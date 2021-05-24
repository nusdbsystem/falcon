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
	logger.Log.Println("SetupDist: Launch master Get port", masterPort)

	masterIP := common.CoordIP
	masterAddr := masterIP + ":" + masterPort

	logger.Log.Println("SetupDist: Launch master for Dev Env")

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
	logger.Log.Println("SetupWorkerHelper: Creating parameters:", masterAddr, workerType)

	workerPort := client.GetFreePort(common.CoordAddr)

	workerAddr := common.PartyServerIP + ":" + workerPort
	var serviceName string

	// in dev, use thread

	common.TaskDataPath = dataPath
	common.TaskDataOutput = dataOutput
	common.TaskModelPath = modelPath

	if workerType == common.TrainWorker {

		serviceName = "worker-job" + jobId + "-train-" + common.PartyID
		common.TaskRuntimeLogs = common.PartyServerBasePath + "/" + common.RuntimeLogs + "/" + serviceName
		ee := os.MkdirAll(common.TaskRuntimeLogs, os.ModePerm)
		logger.Log.Println("SetupWorkerHelper: Creating runtimelogsfolder error", ee)

		logger.Log.Println("SetupWorkerHelper: Current in Dev, TrainWorker")

		wk := worker.InitTrainWorker(masterAddr, workerAddr, common.PartyID)
		wk.RunWorker(wk)

	} else if workerType == common.InferenceWorker {

		serviceName = "worker-job" + jobId + "-inference-" + common.PartyID
		common.TaskRuntimeLogs = common.PartyServerBasePath + "/" + common.RuntimeLogs + "/" + serviceName
		ee := os.MkdirAll(common.TaskRuntimeLogs, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("Error in creating runtimelogsfolder", ee)
		}
		logger.Log.Println("SetupWorkerHelper: Created runtimelogsfolder")

		logger.Log.Println("SetupWorkerHelper:  will init InferenceWorker")
		wk := worker.InitInferenceWorker(masterAddr, workerAddr, common.PartyID)
		wk.RunWorker(wk)

	}

	// in prod, use k8s to run train/predict server as a isolate process

	logger.Log.Println("SetupDist: worker is running")

}
