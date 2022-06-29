package master

import (
	"context"
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"fmt"
	"reflect"
	"sync"
	"time"
)

// master dispatch job to multiple workers, and wait until worker finish
func (master *Master) dispatch(job *common.TrainJob, taskName common.FalconTask) {

	// checking if the IP of worker match the job
	master.Lock()
	master.allWorkerReady.Wait()
	master.Unlock()

	master.Lock()
	var SerializedWorker []string
	for _, worker := range master.workers {
		tmp := fmt.Sprintf("\n  WorkerAddr=%s, PartyID=%d, WorkerID=%d",
			worker.Addr, worker.PartyID, worker.WorkerID)
		SerializedWorker = append(SerializedWorker, tmp)
	}
	master.Logger.Println("[Master.Dispatcher]: All worker found:", SerializedWorker)
	master.Unlock()

	// 1. generate config (MpcIp) for each party-server's mpc tasks
	// 2. generate config (ports) for each party-server's train tasks
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

	// run monitor
	go func() {
		defer logger.HandleErrors()
		master.runtimeStatusMonitor(ctx)
	}()

	master.Logger.Println("[Scheduler]: get taskName by reflect" + taskName)
	// the o position should be always the 'MpcAlgorithmName'
	mpcAlgorithmName := reflect.ValueOf(job.ExecutedTasks[taskName]).Elem().Field(0).String()

	// Run mpc
	master.dispatchMpcTask(&wg, mpcAlgorithmName, job)
	if ok := master.IsSuccessful(); !ok {
		return
	}

	// Run tasks
	master.dispatchTask(&wg, taskName, job)
	if ok := master.IsSuccessful(); !ok {
		return
	}

}

// runtimeStatusMonitor  monitor each worker's status, update the final status accordingly
// if one tasks got error, kill all workers.
func (master *Master) runtimeStatusMonitor(ctx context.Context) {

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
						"kill other workers, runTimeError=%s, RpcCallError=%s\n", status.WorkerAddr, status.RuntimeError, status.RpcCallMethod)
					master.jobStatusLog = entity.MarshalStatus(status)
					master.killWorkers()
				}
			}
		default:
			time.Sleep(10 * time.Millisecond)
		}
	}
}

// IsSuccessful if current tasks has error, return false which will tell dispatcher to skip the following tasks, and return
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
