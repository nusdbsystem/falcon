package distributed

import (
	"coordinator/cache"
	"coordinator/client"
	"coordinator/common"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"os"
)

func SetupDistDev(qItem *cache.QItem, workerType string) {
	// run master to call partyserver to set up worker

	masterPort := client.GetFreePort(common.CoordAddr)
	logger.Do.Println("SetupDist: Launch master Get port", masterPort)

	masterIp := common.CoordIP
	masterAddr := masterIp + ":" + masterPort

	logger.Do.Println("SetupDist: Launch master for Dev Env")

	// use a thread
	SetupMaster(masterAddr, qItem, workerType)
}

func SetupWorkerHelperDev(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string) {

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

	workerPort := client.GetFreePort(common.CoordAddr)

	workerAddr := common.PartyServerIP + ":" + workerPort
	var serviceName string

	// in dev, use thread

	common.TaskDataPath = dataPath
	common.TaskDataOutput = dataOutput
	common.TaskModelPath = modelPath

	if workerType == common.TrainWorker {

		serviceName = "worker-job" + jobId + "-train-" + common.PartyServerId
		common.TaskRuntimeLogs = common.PartyServerBasePath + "/" + common.RuntimeLogs + "/" + serviceName
		ee := os.Mkdir(common.TaskRuntimeLogs, os.ModePerm)
		logger.Do.Println("SetupWorkerHelper: Creating runtimelogsfolder error", ee)

		logger.Do.Println("SetupWorkerHelper: Current in Dev, TrainWorker")

		wk := worker.InitTrainWorker(masterAddr, workerAddr)
		wk.RunWorker(wk)

	} else if workerType == common.InferenceWorker {

		serviceName = "worker-job" + jobId + "-inference-" + common.PartyServerId
		common.TaskRuntimeLogs = common.PartyServerBasePath + "/" + common.RuntimeLogs + "/" + serviceName
		ee := os.Mkdir(common.TaskRuntimeLogs, os.ModePerm)
		if ee != nil {
			logger.Do.Fatalln("Error in creating runtimelogsfolder", ee)
		}
		logger.Do.Println("SetupWorkerHelper: Created runtimelogsfolder")

		logger.Do.Println("SetupWorkerHelper:  will init InferenceWorker")
		wk := worker.InitInferenceWorker(masterAddr, workerAddr)
		wk.RunWorker(wk)

	}

	// in prod, use k8s to run train/predict server as a isolate process

	logger.Do.Println("SetupDist: worker is running")

}
