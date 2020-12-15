package distributed

import (
	"coordinator/cache"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/prediction"
	"coordinator/distributed/worker"
	"coordinator/logger"
)



func SetupDistDev(qItem *cache.QItem, taskType string) {
	/**
	 * @Author
	 * @Description run master, and then, master will call lister to run worker
	 * @Date 2:36 下午 5/12/20
	 * @Param
	 * @return
	 **/

	masterPort := c.GetFreePort(common.CoordSvcURLGlobal)
	logger.Do.Println("SetupDist: Launch master Get port", masterPort)

	masterIp := common.CoordAddrGlobal
	masterAddress := masterIp + ":" + masterPort

	logger.Do.Println("SetupDist: Launch master DevEnv")

	// use a thread
	SetupMaster(masterAddress, qItem, taskType)

	logger.Do.Println("SetupDist: setup master done")
}


func SetupWorkerHelperDev(masterAddress, taskType, jobId, dataPath, modelPath, dataOutput string)  {

	/**
	 * @Author
	 * @Description: this func is only called by partyserver
	 * @Date 2:14 下午 1/12/20
	 * @Param
	 	httpHost： 		IP of the partyserver address
		masterAddress： IP of the master address
		masterAddress： train or predictor
	 **/
	logger.Do.Println("SetupWorkerHelper: Creating parameters:", masterAddress, taskType)

	workerPort := c.GetFreePort(common.CoordSvcURLGlobal)

	workerAddress := common.ListenAddrGlobal + ":" + workerPort
	var serviceName string

	// in dev, use thread

	common.TaskDataPath = dataPath
	common.TaskDataOutput = dataOutput
	common.TaskModelPath = modelPath

	if taskType == common.TrainExecutor{

		serviceName = "worker-jid" + jobId + "-train-" + common.PartyServerId
		common.TaskRuntimeLogs = common.ListenBasePath+"/"+"run_time_logs/"+serviceName

		logger.Do.Println("SetupWorkerHelper: Current in Dev, TrainExecutor")
		worker.RunWorker(masterAddress, workerAddress)

	}else if taskType == common.PredictExecutor{

		serviceName = "worker-jid" + jobId + "-predict-" + common.PartyServerId
		common.TaskRuntimeLogs = common.ListenBasePath+"/"+"run_time_logs/"+serviceName

		logger.Do.Println("SetupWorkerHelper: Current in Dev, PredictExecutor")
		prediction.RunPrediction(masterAddress, workerAddress)
	}

		// in prod, use k8s to run train/predict server as a isolate process

	logger.Do.Println("SetupDist: worker is running")

}