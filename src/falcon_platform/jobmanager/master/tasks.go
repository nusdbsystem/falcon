package master

import (
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"strings"
	"sync"
)

func (master *Master) DispatchDslObj(
	dslOjb *cache.DslObj,
	netCfg string,
	MpcIP string,
	mpcPort uint,
	wg *sync.WaitGroup,
) {

	for i, addr := range dslOjb.PartyAddrList {
		dslObj := new(entity.DslObj4SingleParty)

		// get ip of one party-server, url = ip:port
		partyIP := strings.Split(addr, ":")[0]

		// those are the same as TranJob object or DslObj
		dslObj.JobFlType = dslOjb.JobFlType
		dslObj.ExistingKey = dslOjb.ExistingKey
		dslObj.PartyNums = dslOjb.PartyNums
		dslObj.Tasks = dslOjb.Tasks

		// store only this party's information, contains PartyType used in traintask
		dslObj.PartyInfo = dslOjb.PartyInfoList[i]
		// Proto file for falconMl task communication
		dslObj.NetWorkFile = netCfg

		// used to launch semi-party
		dslObj.MpcIP = MpcIP
		dslObj.MpcPort = mpcPort

		// a list of master.workers, eg: [127.0.0.1:30009:0 127.0.0.1:30010:1 127.0.0.1:30011:2]
		master.Lock()
		for _, worker := range master.workers {

			// match using IP (if the IPs are from different devices)
			// practically for single-machine and multiple terminals, the IP will be the same
			// thus, needs to match the wd worker with the PartyServer info
			// check for wd worker's partyID == dslObj.partyID
			if worker.IP == partyIP && worker.ID == dslObj.PartyInfo.ID {
				logger.Log.Println("[Dispatch]: task 1. Dispatch registered worker=", worker.Addr,
					"with the JobInfo - dslObj.PartyInfo = ", dslObj.PartyInfo)

				wg.Add(1)
				// execute the task
				// append will allocate new memory inside the func stack
				// so must pass addr of slice to func. such that multi goroutines can update the original slices.
				args := entity.EncodeDslObj4SingleParty(dslObj)
				go master.dispatchTask(worker.Addr, string(args), "ReceiveJobInfo", wg)
			}
		}
		master.Unlock()
	}
	wg.Wait()
}

func (master *Master) dispatchMpcTask(wg *sync.WaitGroup, mpcAlgoName string) {
	master.Lock()
	for _, worker := range master.workers {
		wg.Add(1)
		go master.dispatchTask(worker.Addr, mpcAlgoName, "RunMpc", wg)
	}
	master.Unlock()
	wg.Wait()
}

func (master *Master) dispatchPreProcessingTask(wg *sync.WaitGroup) {
	master.Lock()
	for _, worker := range master.workers {
		wg.Add(1)
		go master.dispatchTask(worker.Addr, common.PreProcSubTask, "DoTask", wg)
	}
	master.Unlock()
	wg.Wait()
}

func (master *Master) dispatchModelTrainingTask(wg *sync.WaitGroup) {
	master.Lock()
	for _, worker := range master.workers {
		wg.Add(1)
		go master.dispatchTask(worker.Addr, common.ModelTrainSubTask, "DoTask", wg)
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
			logger.Log.Printf("[Dispatch]: Master calling %s.RetrieveModelReport error\n", worker.Addr)
			return ""
		}
		if rep.RuntimeError == true {
			logger.Log.Printf("[Dispatch]: Worker return error msg when calling %s.RetrieveModelReport: %s\n",
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

func (master *Master) dispatchTask(workerUrl string, args string, rpcCallMethod string, wg *sync.WaitGroup) {

	defer wg.Done()

	var rep entity.DoTaskReply
	rep.WorkerUrl = workerUrl
	rep.RpcCallMethod = rpcCallMethod

	ok := client.Call(workerUrl, master.Network, master.workerType+"."+rpcCallMethod, args, &rep)

	if !ok {
		logger.Log.Printf("[Dispatch]: Master calling %s.%s error\n", workerUrl, rpcCallMethod)
		rep.RpcCallError = true
		rep.TaskMsg.RpcCallMsg = "RpcCallError, one worker is probably terminated"

	} else {
		rep.RpcCallError = false
		rep.TaskMsg.RpcCallMsg = ""
	}
	master.runtimeStatus <- &rep
}
