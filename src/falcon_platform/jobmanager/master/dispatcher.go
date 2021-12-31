package master

import (
	"context"
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"falcon_platform/utils"
	"fmt"
	"sync"
	"time"
)

// master dispatch job to multiple workers, and wait until worker finish
func (master *Master) dispatch(dslOjb *cache.DslObj) {

	// checking if the IP of worker match the dslOjb
	master.Lock()
	master.allWorkerReady.Wait()
	master.Unlock()

	master.Lock()
	var SerializedWorker []string
	for _, worker := range master.workers {
		tmp := fmt.Sprintf("WorkerAddr=%s, PartyID=%d, WorkerID=%d", worker.Addr, worker.PartyID, worker.WorkerID)
		SerializedWorker = append(SerializedWorker, tmp)
	}
	logger.Log.Println("[Master.Dispatcher]: All worker found:", SerializedWorker)
	master.Unlock()

	// 1. generate config (MpcIp) for each party-server's mpc task
	// 2. generate config (ports) for each party-server's train task
	// 3. begin do many tasks:
	//		3.1 Combine the config and assign it to each worker
	//		3.2 Run pre_processing and mpc
	//		3.3 Run model_training and mpc
	//		3.4 more tasks later ?

	wg := sync.WaitGroup{}

	ctx, dispatchDone := context.WithCancel(context.Background())
	defer func() {
		//cancel runtimeStatusMonitor threads
		dispatchDone()
	}()

	go func() {
		defer logger.HandleErrors()
		master.runtimeStatusMonitor(ctx)
	}()

	// 3.1 generate dslObj instance, and assign it to each worker
	master.dispatchDslObj(&wg, dslOjb)
	if ok := master.isSuccessful(); !ok {
		return
	}

	// 3.2 Run pre_processing if there is the task
	if dslOjb.Tasks.PreProcessing.AlgorithmName != "" {

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.PreProcessing.MpcAlgorithmName)
		if ok := master.isSuccessful(); !ok {
			return
		}

		// Run pre_processing
		master.dispatchPreProcessingTask(&wg)
		if ok := master.isSuccessful(); !ok {
			return
		}
	}

	// 3.3 Run model_training if there is the task
	if dslOjb.Tasks.ModelTraining.AlgorithmName != "" {

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.ModelTraining.MpcAlgorithmName)
		if ok := master.isSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg, common.ModelTrainSubTask)
		if ok := master.isSuccessful(); !ok {
			return
		}
	}

	// 3.4 Run LimePred if there is the task
	if dslOjb.Tasks.LimePred.AlgorithmName != "" {

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.LimePred.MpcAlgorithmName)
		if ok := master.isSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg, common.LimePredSubTask)
		if ok := master.isSuccessful(); !ok {
			return
		}
	}

	// 3.5 Run LimeWeight if there is the task
	if dslOjb.Tasks.LimeWeight.AlgorithmName != "" {

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeWeight.MpcAlgorithmName)
		if ok := master.isSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg, common.LimeWeightSubTask)
		if ok := master.isSuccessful(); !ok {
			return
		}
	}

	// 3.6 Run LimeFeature and LimeInterpret if there is the task
	if dslOjb.Tasks.LimeFeature.AlgorithmName != "" || dslOjb.Tasks.LimeInterpret.AlgorithmName != "" {

		// get current class number
		var classNum int32
		if dslOjb.Tasks.LimeFeature.AlgorithmName != "" {
			classNum = dslOjb.Tasks.LimeFeature.ClassNum
		} else if dslOjb.Tasks.LimeInterpret.AlgorithmName != "" {
			classNum = dslOjb.Tasks.LimeInterpret.ClassNum
		}

		// for each classID, assign tasks Serially
		var selectFeatureFile = ""
		var classId int32
		for classId = 0; classId < classNum; classId++ {

			if dslOjb.Tasks.LimeFeature.AlgorithmName != "" {

				// generate SerializedAlgorithmConfig
				dslOjb.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig, _, selectFeatureFile =
					common.GenerateLimeFeatSelParams(dslOjb.Tasks.LimeFeature.InputConfigs.AlgorithmConfig, classId)

				// Run mpc
				master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeFeature.MpcAlgorithmName)
				if ok := master.isSuccessful(); !ok {
					return
				}

				// Run model_training
				master.dispatchGeneralTask(&wg, common.LimeFeatureSubTask+dslOjb.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig)
				if ok := master.isSuccessful(); !ok {
					return
				}
			}

			if dslOjb.Tasks.LimeInterpret.AlgorithmName != "" {

				// generate SerializedAlgorithmConfig
				dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig, _ =
					common.GenerateLimeInterpretParams(dslOjb.Tasks.LimeInterpret.InputConfigs.AlgorithmConfig, classId, selectFeatureFile)

				// Run mpc
				master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeInterpret.MpcAlgorithmName)
				if ok := master.isSuccessful(); !ok {
					return
				}

				// Run model_training
				master.dispatchGeneralTask(&wg, common.LimeInterpretSubTask+dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig)
				if ok := master.isSuccessful(); !ok {
					return
				}
			}
		}

		//todo: for each classID, assign tasks Parallelly?

	}

	// 3.5 more tasks later? add later

	report := master.dispatchRetrieveModelReport()
	logger.Log.Println("[Master.Dispatcher]: report is", report)
	if report != "" {
		// write to disk
		filename := "/opt/falcon/src/falcon_platform/web/build/static/media/model_report"
		err := utils.WriteFile(report, filename)
		if err != nil {
			logger.Log.Printf("[Master.Dispatcher]: write model report to disk error: %s \n", err.Error())
		}
	}

}

func (master *Master) runtimeStatusMonitor(ctx context.Context) {
	// monitor each worker's status, update the final status accordingly
	// if one task got error, kill all workers.

	for {
		select {
		case <-ctx.Done():
			return
		case status := <-master.runtimeStatus:
			// read runtime status,
			if status.RuntimeError == true || status.RpcCallError == true {
				master.jobStatusLock.Lock()
				// kill workers may cause to this,
				if master.jobStatus == common.JobKilled {
					master.jobStatusLock.Unlock()
					// kill all workers.
					logger.Log.Println("[Scheduler]: Master killed all workers")
				} else {
					master.jobStatus = common.JobFailed
					master.jobStatusLock.Unlock()
					// kill all workers.
					logger.Log.Printf("[Scheduler]: One worker failed %s in calling %s, "+
						"kill other workers\n", status.WorkerAddr, status.RpcCallMethod)
					master.jobStatusLog = entity.MarshalStatus(status)
					master.killWorkers()
				}
			}
		default:
			time.Sleep(10 * time.Millisecond)
		}
	}
}

// if current task has error, return false which will tell dispatcher to skip the following tasks, and return
func (master *Master) isSuccessful() bool {
	master.jobStatusLock.Lock()
	if master.jobStatus == common.JobKilled || master.jobStatus == common.JobFailed {
		master.jobStatusLock.Unlock()
		return false
	} else {
		master.jobStatusLock.Unlock()
		return true
	}
}
