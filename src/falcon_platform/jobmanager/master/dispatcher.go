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
func (master *Master) dispatch(dslOjb *cache.DslObj, stageName common.FalconStage) {

	// checking if the IP of worker match the dslOjb
	master.Lock()
	master.allWorkerReady.Wait()
	master.Unlock()

	master.Lock()
	var SerializedWorker []string
	for _, worker := range master.workers {
		tmp := fmt.Sprintf("\n  WorkerAddr=%s, PartyID=%d, GroupID=%d, WorkerID=%d",
			worker.Addr, worker.PartyID, worker.GroupID, worker.WorkerID)
		SerializedWorker = append(SerializedWorker, tmp)
	}
	master.Logger.Println("[Master.Dispatcher]: All worker found:", SerializedWorker)
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
	if ok := master.IsSuccessful(); !ok {
		return
	}

	// 3.2 Run pre_processing if there is the task
	if stageName == common.PreProcStage {
		if dslOjb.Tasks.PreProcessing.AlgorithmName != "" {
			panic("Stage dont have algorithm ERROR")
		}

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.PreProcessing.MpcAlgorithmName, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}

		// Run pre_processing
		master.dispatchPreProcessingTask(&wg)
		if ok := master.IsSuccessful(); !ok {
			return
		}
	}

	// 3.3 Run model_training if there is the task
	if stageName == common.ModelTrainStage {
		if dslOjb.Tasks.ModelTraining.AlgorithmName == "" {
			panic("Stage dont have algorithm ERROR")
		}

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.ModelTraining.MpcAlgorithmName, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.ModelTrainSubTask}, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}
	}

	// 3.4 Run Lime Instance Sampling algorithm is there is the task
	if stageName == common.LimeInstanceSampleStage {
		if dslOjb.Tasks.LimeInsSample.AlgorithmName == "" {
			panic("Stage dont have algorithm ERROR")
		}
		master.Logger.Println("[Master.Dispatcher]: Schedule task=" + stageName)

		// Run lime sampling
		master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.LimeSamplingAlgName}, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}
	}

	// 3.4 Run Lime Instance Sampling algorithm is there is the task
	if stageName == common.LimeInstanceSampleStage {
		if dslOjb.Tasks.LimeInsSample.AlgorithmName == "" {
			panic("Stage dont have algorithm ERROR")
		}
		logger.Log.Println("[Master.Dispatcher]: Schedule task=" + stageName)

		// Run lime sampling
		master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.LimeSamplingAlgName}, common.DefaultWorkerGroupID)
		if ok := master.isSuccessful(); !ok {
			return
		}
	}

	// 3.4 Run LimePred if there is the task
	if stageName == common.LimePredStage {
		if dslOjb.Tasks.LimePred.AlgorithmName == "" {
			panic("Stage dont have algorithm ERROR")
		}

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.LimePred.MpcAlgorithmName, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}

		// Run lime prediction
		master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.LimePredSubTask}, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}
	}

	// 3.5 Run LimeWeight if there is the task
	if stageName == common.LimeWeightStage {
		if dslOjb.Tasks.LimeWeight.AlgorithmName == "" {
			panic("Stage dont have algorithm ERROR")
		}

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeWeight.MpcAlgorithmName, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.LimeWeightSubTask}, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}
	}

	if stageName == common.LimeFeatureSelectionStage {
		if dslOjb.Tasks.LimeFeature.AlgorithmName == "" {
			panic("Stage dont have algorithm ERROR")
		}
		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeFeature.MpcAlgorithmName, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg,
			&entity.GeneralTask{TaskName: common.LimeFeatureSubTask, AlgCfg: dslOjb.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig},
			common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}
	}

	if stageName == common.LimeVFLModelTrainStage {
		if dslOjb.Tasks.LimeInterpret.AlgorithmName == "" {
			panic("Stage dont have algorithm ERROR")
		}
		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeInterpret.MpcAlgorithmName, common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.LimeInterpretSubTask, AlgCfg: dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig},
			common.DefaultWorkerGroupID)
		if ok := master.IsSuccessful(); !ok {
			return
		}

		// Run model_training
		master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.LimeInterpretSubTask, AlgCfg: dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig},
			common.DefaultWorkerGroupID)
		if ok := master.isSuccessful(); !ok {
			return
		}
	}

	// 3.6 Run LimeFeature and LimeInterpret if there is the task
	//if stageName == common.LimeInterpretStage {
	//	if dslOjb.Tasks.LimeFeature.AlgorithmName == "" && dslOjb.Tasks.LimeInterpret.AlgorithmName == "" {
	//		panic("Stage dont have algorithm ERROR")
	//	}
	//
	//	// get current class number
	//	var classNum int32
	//	if dslOjb.Tasks.LimeFeature.AlgorithmName != "" {
	//		classNum = dslOjb.Tasks.LimeFeature.ClassNum
	//	} else if dslOjb.Tasks.LimeInterpret.AlgorithmName != "" {
	//		classNum = dslOjb.Tasks.LimeInterpret.ClassNum
	//	}
	//
	//	// centralized training
	//	if dslOjb.DistributedTask.Enable == 0 {
	//		// for each classID, assign tasks Serially
	//		var selectFeatureFile = ""
	//		var classId int32
	//		for classId = 0; classId < classNum; classId++ {
	//
	//			if dslOjb.Tasks.LimeFeature.AlgorithmName != "" {
	//
	//				// generate SerializedAlgorithmConfig
	//				dslOjb.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig, _, selectFeatureFile =
	//					common.GenerateLimeFeatSelParams(dslOjb.Tasks.LimeFeature.InputConfigs.AlgorithmConfig, classId)
	//
	//				// Run mpc
	//				master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeFeature.MpcAlgorithmName, common.DefaultWorkerGroupID)
	//				if ok := master.IsSuccessful(); !ok {
	//					return
	//				}
	//
	//				// Run model_training
	//				master.dispatchGeneralTask(&wg,
	//					&entity.GeneralTask{TaskName: common.LimeFeatureSubTask, AlgCfg: dslOjb.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig},
	//					common.DefaultWorkerGroupID)
	//
	//				if ok := master.IsSuccessful(); !ok {
	//					return
	//				}
	//			}
	//
	//			if dslOjb.Tasks.LimeInterpret.AlgorithmName != "" {
	//
	//				// generate SerializedAlgorithmConfig
	//				dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig, _ =
	//					common.GenerateLimeInterpretParams(dslOjb.Tasks.LimeInterpret.InputConfigs.AlgorithmConfig, classId, selectFeatureFile, dslOjb.Tasks.LimeInterpret.MpcAlgorithmName)
	//
	//				// Run mpc
	//				master.dispatchMpcTask(&wg, dslOjb.Tasks.LimeInterpret.MpcAlgorithmName,
	//					common.DefaultWorkerGroupID)
	//				if ok := master.IsSuccessful(); !ok {
	//					return
	//				}
	//
	//				// Run model_training
	//				master.dispatchGeneralTask(&wg, &entity.GeneralTask{TaskName: common.LimeInterpretSubTask, AlgCfg: dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig},
	//					common.DefaultWorkerGroupID)
	//
	//				if ok := master.IsSuccessful(); !ok {
	//					return
	//				}
	//			}
	//		}
	//	}
	//
	//	if dslOjb.DistributedTask.Enable == 1 {
	//
	//		// generate availableGroupIds
	//		var availableGroupIds []common.GroupIdType
	//		for i := 0; i < master.SchedulerPolicy.LimeClassParallelism; i++ {
	//			availableGroupIds = append(availableGroupIds, common.GroupIdType(i))
	//		}
	//
	//		var classId int32 = 0
	//
	//		master.Logger.Println("[Master.Dispatcher]: dispatch lime tasks, len(availableGroupIds) = ",
	//			len(availableGroupIds),
	//			" max class parallelism =", master.SchedulerPolicy.LimeClassParallelism,
	//			" classNum = ", classNum)
	//
	//		// for each group, assign a class id
	//		for {
	//			if classId == classNum {
	//				break
	//			}
	//			master.Lock()
	//			if len(availableGroupIds) == 0 {
	//				master.Logger.Println("[Master.Dispatcher]: wait until one worker group is released...")
	//				time.Sleep(1 * time.Second)
	//				master.Unlock()
	//				continue
	//			}
	//			groupId := availableGroupIds[0]
	//			availableGroupIds = availableGroupIds[1:]
	//			master.Logger.Println("[Master.Dispatcher]: dispatch lime tasks, assign classid=", classId,
	//				" to worker group =", groupId,
	//				" current len(availableGroupIds) = ", availableGroupIds)
	//			master.Unlock()
	//
	//			// every time create a new limeWg for different worker, because different worker are running parallelism
	//			limeWg := sync.WaitGroup{}
	//
	//			go func(groupIdParam common.GroupIdType, classIdParam int32, availableGroupIds *[]common.GroupIdType, limeWgParam *sync.WaitGroup) {
	//				var selectFeatureFile = ""
	//				if dslOjb.Tasks.LimeFeature.AlgorithmName != "" {
	//
	//					// generate SerializedAlgorithmConfig
	//					dslOjb.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig, _, selectFeatureFile =
	//						common.GenerateLimeFeatSelParams(dslOjb.Tasks.LimeFeature.InputConfigs.AlgorithmConfig, classIdParam)
	//
	//					// Run mpc
	//					master.dispatchMpcTask(&limeWg, dslOjb.Tasks.LimeFeature.MpcAlgorithmName, groupIdParam)
	//					if ok := master.IsSuccessful(); !ok {
	//						return
	//					}
	//
	//					// Run model_training
	//					master.dispatchGeneralTask(&limeWg,
	//						&entity.GeneralTask{TaskName: common.LimeFeatureSubTask, AlgCfg: dslOjb.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig},
	//						groupIdParam)
	//
	//					if ok := master.IsSuccessful(); !ok {
	//						return
	//					}
	//				}
	//				if dslOjb.Tasks.LimeInterpret.AlgorithmName != "" {
	//
	//					// generate SerializedAlgorithmConfig
	//					dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig, _ =
	//						common.GenerateLimeInterpretParams(dslOjb.Tasks.LimeInterpret.InputConfigs.AlgorithmConfig,
	//							classIdParam, selectFeatureFile, dslOjb.Tasks.LimeInterpret.MpcAlgorithmName)
	//
	//					// Run mpc
	//					master.dispatchMpcTask(&limeWg, dslOjb.Tasks.LimeInterpret.MpcAlgorithmName,
	//						groupIdParam)
	//					if ok := master.IsSuccessful(); !ok {
	//						return
	//					}
	//
	//					// Run model_training
	//					master.dispatchGeneralTask(&limeWg,
	//						&entity.GeneralTask{TaskName: common.LimeInterpretSubTask, AlgCfg: dslOjb.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig},
	//						groupIdParam)
	//
	//					if ok := master.IsSuccessful(); !ok {
	//						return
	//					}
	//				}
	//				master.Lock()
	//				// after finishing the task, append the groupIdParam back to availableGroupIds
	//				*availableGroupIds = append(*availableGroupIds, groupIdParam)
	//				master.Logger.Println("[Master.Dispatcher]: dispatch lime tasks, add group id back to availableGroupIds, current len = ", len(*availableGroupIds))
	//				master.Unlock()
	//			}(groupId, classId, &availableGroupIds, &limeWg)
	//			// process the next task, with new classID
	//			classId++
	//		}
	//
	//		// wait until all task done
	//		master.Logger.Println("[Master.Dispatcher]: dispatch lime tasks done, now waiting for all lime task (for each class) finishing...")
	//		for {
	//			master.jobStatusLock.Lock()
	//			// kill workers may cause to this,
	//			if master.jobStatus == common.JobKilled {
	//				master.jobStatusLock.Unlock()
	//				master.Logger.Println("[Master.Dispatcher]: dispatch lime tasks: job is killed caused by some error")
	//
	//				break
	//			} else {
	//				master.jobStatusLock.Unlock()
	//				master.Lock()
	//				master.Logger.Println("[Master.Dispatcher]: current len(len(availableGroupIds)) = ", len(availableGroupIds))
	//				// all groupId has been released
	//				if len(availableGroupIds) == master.SchedulerPolicy.LimeClassParallelism {
	//					master.Unlock()
	//					break
	//				} else {
	//					master.Unlock()
	//					time.Sleep(5 * time.Second)
	//				}
	//			}
	//		}
	//	}
	//
	//}

	// 3.5 more tasks later? add later
	report := master.dispatchRetrieveModelReport()
	master.Logger.Println("[Master.Dispatcher]: report is", report)
	if report != "" {
		// write to disk
		filename := "/opt/falcon/src/falcon_platform/web/build/static/media/model_report"
		err := utils.WriteFile(report, filename)
		if err != nil {
			master.Logger.Printf("[Master.Dispatcher]: write model report to disk error: %s \n", err.Error())
		}
	}

}

func (master *Master) runtimeStatusMonitor(ctx context.Context) {
	// monitor each worker's status, update the final status accordingly
	// if one task got error, kill all workers.

	for {
		select {
		case <-ctx.Done():
			master.Logger.Println("[Scheduler]: Master finish dispatching tasks.")
			return
		case status := <-master.runtimeStatus:
			// read runtime status,
			if status.RuntimeError == true || status.RpcCallError == true {
				master.jobStatusLock.Lock()
				// kill workers may cause to this,
				if master.jobStatus == common.JobKilled {
					master.jobStatusLock.Unlock()
					// kill all workers.
					master.Logger.Println("[Scheduler]: Master killed all workers")
				} else {
					master.jobStatus = common.JobFailed
					master.jobStatusLock.Unlock()
					// kill all workers.
					master.Logger.Printf("[Scheduler]: One worker failed %s in calling %s, "+
						"kill other workers, runTImeError=%s, RpcCallError=%s\n", status.WorkerAddr, status.RuntimeError, status.RpcCallMethod)
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
func (master *Master) IsSuccessful() bool {
	master.jobStatusLock.Lock()
	if master.jobStatus == common.JobKilled || master.jobStatus == common.JobFailed {
		master.jobStatusLock.Unlock()
		return false
	} else {
		master.jobStatusLock.Unlock()
		return true
	}
}
