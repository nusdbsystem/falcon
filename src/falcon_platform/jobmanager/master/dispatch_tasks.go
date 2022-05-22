package master

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/jobmanager/tasks"
	"falcon_platform/logger"
	"sync"
)

// taskName: which task to run
// algName: which algorithm name is used in this task
func (master *Master) dispatchMpcTask(wg *sync.WaitGroup, mpcAlgorithmName string, job *common.TrainJob) {
	master.Lock()
	for _, worker := range master.workers {
		wg.Add(1)
		go func(wk *entity.WorkerInfo) {
			defer logger.HandleErrors()
			task := tasks.GetAllTasks()[common.MpcTaskKey]
			// master send this to each worker, and worker will use it to generate command line.

			generalTask := &entity.TaskContext{TaskName: common.MpcTaskKey,
				Wk:           wk,
				FLNetworkCfg: &master.JobNetCfg,
				Job:          job,
				MpcAlgName:   mpcAlgorithmName,
			}

			args := string(entity.SerializeTask(generalTask))
			master.dispatching(wk.Addr, args, task.GetRpcCallMethodName(), wg)
		}(worker)
	}
	master.Unlock()
	wg.Wait()
}

// dispatchTask
func (master *Master) dispatchTask(wg *sync.WaitGroup, taskName common.FalconTask, job *common.TrainJob) {
	master.Lock()
	for _, worker := range master.workers {
		wg.Add(1)
		go func(wk *entity.WorkerInfo) {
			defer logger.HandleErrors()
			task := tasks.GetAllTasks()[taskName]
			// master send this to each worker, and worker will use it to generate command line.
			generalTask := &entity.TaskContext{TaskName: taskName,
				Wk:           wk,
				FLNetworkCfg: &master.JobNetCfg,
				Job:          job}

			args := string(entity.SerializeTask(generalTask))
			master.dispatching(wk.Addr, args, task.GetRpcCallMethodName(), wg)
		}(worker)
	}
	master.Unlock()
	wg.Wait()
}

func (master *Master) dispatchRetrieveModelReport() string {

	var report string
	master.Lock()
	for _, worker := range master.workers {
		var rep entity.RetrieveModelReportReply
		ok := client.Call(worker.Addr, master.Network, master.workerType+".RetrieveModelReport", "", &rep)
		if !ok {
			master.Logger.Printf("[Master.Dispatch]: Master calling %s.RetrieveModelReport error\n", worker.Addr)
			return ""
		}
		if rep.RuntimeError == true {
			master.Logger.Printf("[Master.Dispatch]: Worker return error msg when calling %s.RetrieveModelReport: %s\n",
				worker.Addr, rep.TaskMsg.RuntimeMsg)
			return ""
		}

		if rep.ContainsModelReport == true {
			report = rep.ModelReport
			return report
		}
	}
	master.Unlock()
	return ""
}

// dispatching a task
func (master *Master) dispatching(workerAddr string, args string, rpcCallMethod string, wg *sync.WaitGroup) {

	defer wg.Done()

	var rep entity.DoTaskReply
	rep.WorkerAddr = workerAddr
	rep.RpcCallMethod = rpcCallMethod

	ok := client.Call(workerAddr, master.Network, master.workerType+"."+rpcCallMethod, args, &rep)

	if !ok {
		master.Logger.Printf("[Master.Dispatch]: Master calling %s.%s error\n", workerAddr, rpcCallMethod)
		rep.RpcCallError = true
		rep.TaskMsg.RpcCallMsg = "RpcCallError, one worker is probably terminated"

	} else {
		rep.RpcCallError = false
		rep.TaskMsg.RpcCallMsg = ""
	}
	master.runtimeStatus <- &rep
}
