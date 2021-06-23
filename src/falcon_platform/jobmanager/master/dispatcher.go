package master

import (
	"context"
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"falcon_platform/utils"
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

	report := master.dispatchRetrieveModelReport()
	logger.Log.Println("[Dispatch]: report is", report)
	if report != "" {
		// write to disk
		filename := "/opt/falcon/src/falcon_platform/web/build/static/media/model_report"
		err := utils.WriteFile(report, filename)
		if err != nil {
			logger.Log.Printf("[Dispatch]: write model report to disk error: %s \n", err.Error())
		}
	}

}

func generateMPCCfg(dslOjb *cache.DslObj) (uint, string) {
	mpcPort := client.GetFreePort(common.CoordAddr)
	mpcPint, _ := strconv.Atoi(mpcPort)
	var MpcIP string
	for i, addr := range dslOjb.PartyAddrList {
		// all mpc process use only party-0's (active party) ip.
		if dslOjb.PartyInfoList[i].PartyType == common.ActiveParty {
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
