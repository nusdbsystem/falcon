package distributed

import (
	"coordinator/cache"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/master"
	"coordinator/distributed/prediction"
	"coordinator/distributed/taskmanager"
	"coordinator/distributed/utils"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"fmt"
)

func SetupDist(qItem *cache.QItem, taskType string) {

	var masterIp string
	if common.Env == common.DevEnv{
		masterIp = common.CoordAddrGlobal
		// use a thread
		// in dev model, the master ip is the same as coordinator ip
		SetupMaster(masterIp, qItem, taskType)

	}else if common.Env == common.ProdEnv{
		// in prod, use k8s to run train/predict server as a isolate process
		filename := common.MasterYaml

		km := taskmanager.InitK8sManager(true,  "")
		km.CreateResources(filename)
	}
	logger.Do.Println("SetupDist: setup master done")
}


func KillJob(masterAddr, Proxy string) {
	ok := c.Call(masterAddr, Proxy, "Master.KillJob", new(struct{}), new(struct{}))
	if ok == false {
		logger.Do.Println("Master: KillJob error")
		panic("Master: KillJob error")
	} else {
		logger.Do.Println("Master: KillJob Done")
	}
}

func SetupWorkerHelper(httpHost string, masterAddress, taskType string)  {

	/**
	 * @Author
	 * @Description
	 * @Date 2:14 下午 1/12/20
	 * @Param
	 	httpHost： 		IP of the listener address
		masterAddress： IP of the master address
		masterAddress： train or predictor
	 **/

	// in dev, use thread
	if common.Env == common.DevEnv{

		if taskType == common.TrainTaskType{

			SetupTrain(httpHost, masterAddress)

		}else if taskType == common.PredictTaskType{

			SetupPrediction(httpHost, masterAddress)
		}

		// in prod, use k8s to run train/predict server as a isolate process
	}else if common.Env == common.ProdEnv{

		var filename string

		if taskType == common.TrainTaskType{
			filename = common.TrainYaml

		}else if taskType == common.PredictTaskType{
			filename = common.PredictorYaml
		}

		km := taskmanager.InitK8sManager(true,  "")
		km.CreateResources(filename)

	}

	logger.Do.Println("SetupDist: worker is running")

}

func SetupMaster(masterIp string, qItem *cache.QItem, taskType string) string {
	/**
	 * @Author
	 * @Description : run train rpc server in a thread, used to test only
	 * @Date 2:26 下午 1/12/20
	 * @Param
	 * @return
	 **/
	logger.Do.Println("SetupDist: Lunching master")

	port, e := utils.GetFreePort4K8s()
	if e != nil {
		logger.Do.Println("SetupDist: Launch master Get port Error")
		panic(e)
	}
	masterPort := fmt.Sprintf("%d", port)
	masterAddress := masterIp + ":" + masterPort

	master.RunMaster(masterAddress, qItem, taskType)


	// update job's master address
	if taskType == common.TrainTaskType{

		c.JobUpdateMaster(common.CoordAddrPortGlobal, masterAddress, qItem.JobId)
	}

	// master will call lister's endpoint to launch worker, to train or predict
	for _, ip := range qItem.IPs {

		// Launch the worker
		// todo currently worker port is fixed, not a good design, change to dynamic later
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		c.SetupWorker(ip+":"+common.ListenerPort, masterAddress, taskType)
	}

	logger.Do.Println("SetupDist: master is running at ", masterAddress)

	return masterAddress
}


func SetupTrain(httpHost string, masterAddress string)  {
	/**
	 * @Author
	 * @Description : run train rpc server in a thread, used to test only
	 * @Date 2:26 下午 1/12/20
	 * @Param
	 * @return
	 **/
	logger.Do.Println("SetupDist: Launch worker threads")

	port, e := utils.GetFreePort()
	if e != nil {
		logger.Do.Println("SetupDist: Launch worker Get port Error")
		panic(e)
	}

	// worker address share the same host with listener server
	worker.RunWorker(masterAddress, httpHost, fmt.Sprintf("%d", port))

}

func SetupPrediction(httpHost, masterAddress string)  {
	/**
	 * @Author
	 * @Description : run prediction rpc server in a thread, used to test only
	 * @Date 2:26 下午 1/12/20
	 * @Param
	 * @return
	 **/
	// the masterAddress is the master thread address

	logger.Do.Println("SetupDist: Lunching prediction svc")

	port, e := utils.GetFreePort()
	if e != nil {
		logger.Do.Println("SetupDist: Lunching worker Get port Error")
		panic(e)
	}
	// worker address share the same host with listener server
	prediction.RunPrediction(masterAddress, httpHost, fmt.Sprintf("%d", port),"tcp")

}


//pm := taskmanager.InitSubProcessManager()
//dir:=""
//stdIn := ""
//var envs []string
//
//var args []string
//
//commend := "./coordinator_server"
//
//if taskType == common.TrainTaskType{
//	args = []string{
//		fmt.Sprintf("-svc %s -wip %s -master_addr %s",
//		common.TrainTaskType,
//		httpHost,
//		masterAddress)}
//
//}else if taskType == common.PredictTaskType{
//	args = []string{
//		fmt.Sprintf("-svc %s -wip %s -master_addr %s",
//			common.PredictTaskType,
//			httpHost,
//			masterAddress)}
//	pm.IsWait = false
//
//}
//
//killed, e, el, ol := pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
//logger.Do.Println(killed, e, el, ol)
