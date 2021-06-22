package master

import (
	"context"
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"strconv"
	"strings"
	"sync"
	"time"
)

func (master *Master) dispatch(dslOjb *cache.DslObj) {

	// checking if the IP of worker match the dslOjb
	master.Lock()
	master.allWorkerReady.Wait()
	master.Unlock()

	master.Lock()
	logger.Log.Println("[Dispatch]: All worker found: ", master.workers, " required IP: ", dslOjb.PartyAddrList, "Ip matched !!")
	master.Unlock()

	// 1. generate config (MpcIp) for each party-server's mpc task
	// 2. generate config (ports) for each party-server's train task
	// 3. begin do many tasks:
	//		3.1 Combine the config and assign it to each worker
	//		3.2 Run pre_processing and mpc
	//		3.3 Run model_training and mpc
	//		3.4 more tasks later ?

	wg := sync.WaitGroup{}

	// 1. generate MpcIP for each party-server's mpc task
	mpcPort, MpcIP := generateMPCCfg(dslOjb)

	// 2. generate config (ip, ports) for each party-server's train task
	netCfg := generateNetworkCfg(dslOjb)

	ctx, dispatchDone := context.WithCancel(context.Background())
	defer func() {
		//cancel runtimeStatusMonitor threads
		dispatchDone()
	}()

	go master.runtimeStatusMonitor(ctx)

	// 3.1 combine the config to generate dslObj instance, and assign it to each worker
	master.DispatchDslObj(dslOjb, netCfg, MpcIP, mpcPort, &wg)
	if ok := master.keepExecuting(); !ok {
		return
	}

	// 3.2 Run pre_processing, if has this task
	if dslOjb.Tasks.PreProcessing.AlgorithmName != "" {

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.PreProcessing.MpcAlgorithmName)
		if ok := master.keepExecuting(); !ok {
			return
		}

		// Run pre_processing
		master.dispatchPreProcessingTask(&wg)
		if ok := master.keepExecuting(); !ok {
			return
		}
	}

	// 3.3 Run model_training if has this task
	if dslOjb.Tasks.ModelTraining.AlgorithmName != "" {

		// Run mpc
		master.dispatchMpcTask(&wg, dslOjb.Tasks.ModelTraining.MpcAlgorithmName)
		if ok := master.keepExecuting(); !ok {
			return
		}

		// Run model_training
		master.dispatchModelTrainingTask(&wg)
		if ok := master.keepExecuting(); !ok {
			return
		}
	}

	// 3.5 more tasks later? add later

}

func generateMPCCfg(dslOjb *cache.DslObj) (uint, string) {
	mpcPort := client.GetFreePort(common.CoordAddr)
	mpcPint, _ := strconv.Atoi(mpcPort)
	var MpcIP string
	for i, addr := range dslOjb.PartyAddrList {
		// all mpc process use only party-0's (active party) ip.
		if dslOjb.PartyInfoList[i].PartyType == "active" {
			MpcIP = strings.Split(addr, ":")[0]
		}
	}
	return uint(mpcPint), MpcIP
}

func generateNetworkCfg(dslOjb *cache.DslObj) string {
	// generate network config for each party,
	var portArray [][]int32
	for i := 0; i < len(dslOjb.PartyAddrList); i++ {
		var ports []int32
		//generate n ports
		for i := 0; i < len(dslOjb.PartyAddrList); i++ {
			port := client.GetFreePort(common.CoordAddr)
			pint, _ := strconv.Atoi(port)
			ports = append(ports, int32(pint))
		}
		portArray = append(portArray, ports)
	}
	netCfg := common.GenerateNetworkConfig(dslOjb.PartyAddrList, portArray)
	return netCfg
}

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
						"kill other workers\n", status.WorkerUrl, status.RpcCallMethod)
					master.jobStatusLog = entity.MarshalStatus(status)
					master.killWorkers()
				}
			}
		default:
			time.Sleep(10 * time.Millisecond)
		}
	}
}

func (master *Master) keepExecuting() bool {
	// if current task has error, return false which will tell dispatcher to skip the following tasks, and return
	master.jobStatusLock.Lock()
	if master.jobStatus == common.JobKilled || master.jobStatus == common.JobFailed {
		master.jobStatusLock.Unlock()
		return false
	} else {
		master.jobStatusLock.Unlock()
		return true
	}
}
