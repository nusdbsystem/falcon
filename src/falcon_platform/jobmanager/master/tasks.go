package master

import (
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"sync"
)

/**
 * @Description Dispatch dsl information to each worker, each worker requires different information
 * @Date 下午2:33 25/08/21
 * @Param dslObj dslObj
 * @return
 **/
func (master *Master) dispatchDslObj(wg *sync.WaitGroup, dslObj *cache.DslObj) {

	// for each party's resources, send dslObj
	for partyIndex, LaunchResourceReply := range master.RequiredResource {

		partyID := LaunchResourceReply.PartyID

		dslObj4sp := new(entity.DslObj4SingleWorker)

		// all worker in same party share those vars
		dslObj4sp.JobFlType = dslObj.JobFlType
		dslObj4sp.ExistingKey = dslObj.ExistingKey
		dslObj4sp.PartyNums = dslObj.PartyNums
		dslObj4sp.Tasks = dslObj.Tasks
		dslObj4sp.WorkerPreGroup = dslObj.DistributedTask.WorkerNumber + 1

		// store only this party's information, contains PartyType used in train task
		dslObj4sp.PartyInfo = dslObj.PartyInfoList[partyIndex]
		dslObj4sp.DistributedTask = dslObj.DistributedTask

		// each worker in same party have following individual vars
		for workerID, ResourceSVC := range LaunchResourceReply.ResourceSVCs {
			// get worker address
			workerAddr := ResourceSVC.ToAddr(ResourceSVC.WorkerPort)
			dslObj4sp.ExecutorPairNetworkCfg = master.ExtractedResource.executorPairNetworkCfg[workerID]
			dslObj4sp.MpcPairNetworkCfg = master.ExtractedResource.mpcPairNetworkCfg[workerID]
			dslObj4sp.MpcExecutorNetworkCfg = master.ExtractedResource.mpcExecutorNetworkCfg[workerID][partyIndex]
			dslObj4sp.DistributedExecutorPairNetworkCfg = master.ExtractedResource.distributedNetworkCfg[partyID][ResourceSVC.GroupId]

			// encode object and send to the worker
			args := entity.EncodeDslObj4SingleWorker(dslObj4sp)

			logger.Log.Println("[Master.Dispatch]: task 1. Dispatch registered worker=", workerAddr,
				" worker_id = ", workerID, " group id = ", ResourceSVC.GroupId,
				" with the JobInfo:",
				"\nExecutorPairNetworkCfg= ", dslObj4sp.ExecutorPairNetworkCfg,
				"\nMpcPairNetworkCfg= ", dslObj4sp.MpcPairNetworkCfg,
				"\nMpcExecutorNetworkCfg= ", dslObj4sp.MpcExecutorNetworkCfg,
				"\nDistributedExecutorPairNetworkCfg= ", dslObj4sp.DistributedExecutorPairNetworkCfg)

			common.RetrieveNetworkConfig(dslObj4sp.ExecutorPairNetworkCfg)
			common.RetrieveDistributedNetworkConfig(dslObj4sp.DistributedExecutorPairNetworkCfg)


			wg.Add(1)
			// dispatch dslObj to the worker
			go func(addr string, args []byte) {
				defer logger.HandleErrors()
				master.dispatchTask(addr, string(args), "ReceiveJobInfo", wg)
			}(workerAddr, args)
		}
	}
	wg.Wait()
}

/**
 * @Description dispatch mpc task to each worker in this group, worker decide to accept or not
 * @Date 下午2:49 25/08/21
 * @Param mpcAlgoName, Algorithm name for running mpc, different algorithm use different mpc
 * @return
 **/
func (master *Master) dispatchMpcTask(wg *sync.WaitGroup, mpcAlgoName string, groupID common.GroupIdType) {
	master.Lock()
	for _, worker := range master.workers {
		if worker.GroupID == groupID {
			wg.Add(1)
			go func(addr string) {
				defer logger.HandleErrors()
				master.dispatchTask(addr, mpcAlgoName, "RunMpc", wg)
			}(worker.Addr)
		}
	}
	master.Unlock()
	wg.Wait()
}

/**
 * @Description dispatch PreProcessing Task, worker decide to accept or not
 * @Date 下午2:50 25/08/21
 * @return
 **/
func (master *Master) dispatchPreProcessingTask(wg *sync.WaitGroup) {
	master.Lock()
	for _, worker := range master.workers {
		wg.Add(1)
		go func(addr string) {
			defer logger.HandleErrors()
			master.dispatchTask(addr, common.PreProcSubTask, "DoTask", wg)
		}(worker.Addr)

	}
	master.Unlock()
	wg.Wait()
}

/**
 * @Description dispatch Model TrainingTask, worker decide to accept or not
 * @Date 下午2:51 25/08/21
 * @Param
 * @Param
 * @Param
 * @return
 **/
func (master *Master) dispatchGeneralTask(wg *sync.WaitGroup, taskName string, groupID common.GroupIdType) {
	master.Lock()
	for _, worker := range master.workers {
		if worker.GroupID == groupID {
			wg.Add(1)
			go func(addr string) {
				defer logger.HandleErrors()
				master.dispatchTask(addr, taskName, "DoTask", wg)
			}(worker.Addr)
		}
	}

	master.Unlock()
	wg.Wait()
}

/**
 * @Description dispatch Retrieve Model Report, worker decide to accept or not
 * @Date 下午2:51 25/08/21
 * @Param
 * @Param
 * @Param
 * @return
 **/
func (master *Master) dispatchRetrieveModelReport() string {

	var report string
	master.Lock()
	for _, worker := range master.workers {
		var rep entity.RetrieveModelReportReply
		ok := client.Call(worker.Addr, master.Network, master.workerType+".RetrieveModelReport", "", &rep)
		if !ok {
			logger.Log.Printf("[Master.Dispatch]: Master calling %s.RetrieveModelReport error\n", worker.Addr)
			return ""
		}
		if rep.RuntimeError == true {
			logger.Log.Printf("[Master.Dispatch]: Worker return error msg when calling %s.RetrieveModelReport: %s\n",
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

/**
 * @Description worker decide to accept or not
 * @Date 下午2:51 25/08/21
 * @Param workerAddr worker Address
 * @Param args encoded args to rpc call
 * @Param rpcCallMethod method to call
 * @return
 **/
func (master *Master) dispatchTask(workerAddr string, args string, rpcCallMethod string, wg *sync.WaitGroup) {

	defer wg.Done()

	var rep entity.DoTaskReply
	rep.WorkerAddr = workerAddr
	rep.RpcCallMethod = rpcCallMethod

	ok := client.Call(workerAddr, master.Network, master.workerType+"."+rpcCallMethod, args, &rep)

	if !ok {
		logger.Log.Printf("[Master.Dispatch]: Master calling %s.%s error\n", workerAddr, rpcCallMethod)
		rep.RpcCallError = true
		rep.TaskMsg.RpcCallMsg = "RpcCallError, one worker is probably terminated"

	} else {
		rep.RpcCallError = false
		rep.TaskMsg.RpcCallMsg = ""
	}
	master.runtimeStatus <- &rep
}
