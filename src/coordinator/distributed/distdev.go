package distributed

import (
	"coordinator/cache"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"os"
)



func SetupDistDev(qItem *cache.QItem, workerType string) {
	/**
	 * @Author
	 * @Description run master, and then, master will call lister to run worker
	 * @Date 2:36 下午 5/12/20
	 * @Param
	 * @return
	 **/

	masterPort := c.GetFreePort(common.CoordAddr)
	logger.Do.Println("SetupDist: Launch master Get port", masterPort)

	masterIp := common.CoordIP
	masterAddr := masterIp + ":" + masterPort

	logger.Do.Println("SetupDist: Launch master DevEnv")

	// use a thread
	SetupMaster(masterAddr, qItem, workerType)

	logger.Do.Println("SetupDist: setup master done")
}


func SetupWorkerHelperDev(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string)  {

	/**
	 * @Author
	 * @Description: this func is only called by partyserver
	 * @Date 2:14 下午 1/12/20
	 * @Param
	 	httpHost： 		IP of the partyserver addr
		masterAddr： IP of the master addr
		masterAddr： train or predictor
	 **/
	logger.Do.Println("SetupWorkerHelper: Creating parameters:", masterAddr, workerType)

	workerPort := c.GetFreePort(common.CoordAddr)

	workerAddr := common.PartyServerIP + ":" + workerPort
	var serviceName string

	// in dev, use thread

	common.TaskDataPath = dataPath
	common.TaskDataOutput = dataOutput
	common.TaskModelPath = modelPath

	if workerType == common.TrainWorker{

		serviceName = "worker-jid" + jobId + "-train-" + common.PartyServerId
		common.TaskRuntimeLogs = common.PartyServeBasePath+"/"+common.RuneTimeLogs +"/"+serviceName
		ee := os.Mkdir(common.TaskRuntimeLogs, os.ModePerm)
		logger.Do.Println("SetupWorkerHelper: Creating runtimelogsfolder error", ee)

		logger.Do.Println("SetupWorkerHelper: Current in Dev, TrainWorker")

		wk := worker.InitTrainWorker(masterAddr, workerAddr)
		wk.RunWorker(wk)

	}else if workerType == common.InferenceWorker{

		serviceName = "worker-jid" + jobId + "-inference-" + common.PartyServerId
		common.TaskRuntimeLogs = common.PartyServeBasePath+"/" + common.RuneTimeLogs + "/"+serviceName
		ee := os.Mkdir(common.TaskRuntimeLogs, os.ModePerm)
		logger.Do.Println("SetupWorkerHelper: Creating runtimelogsfolder error", ee)
		logger.Do.Println("SetupWorkerHelper: Current in Dev, InferenceWorker")

		wk := worker.InitInferenceWorker(masterAddr, workerAddr)
		wk.RunWorker(wk)

	}

		// in prod, use k8s to run train/predict server as a isolate process

	logger.Do.Println("SetupDist: worker is running")

}