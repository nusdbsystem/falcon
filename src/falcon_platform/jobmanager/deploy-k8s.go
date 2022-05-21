package jobmanager

import (
	"falcon_platform/common"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"strings"
)

// DeployMasterK8s run master to call party server to set up worker
func deployJobManagerK8s(job *common.TrainJob, workerType string) {

	masterPort := resourcemanager.GetFreePort(1)[0]
	logger.Log.Printf("[JobManager]: Assign port %d to master\n", masterPort)

	masterIP := common.CoordIP
	masterAddr := masterIP + ":" + fmt.Sprintf("%d", masterPort)

	// use k8s to run train/predict server as a isolate process
	itemKey := "job" + fmt.Sprintf("%d", job.JobId)

	serviceName := "master-" + itemKey + "-" + strings.ToLower(workerType)

	// put to the dslqueue, assign key to env
	logger.Log.Println("[JobManager]: Writing item to redis")

	//cache.InitRedisClient().Set(itemKey, cache.Serialize(job))

	logger.Log.Printf("[JobManager]: Get key, %s InitK8sManager\n", itemKey)

	km := resourcemanager.InitK8sManager(true, "")

	command := []string{
		common.MasterYamlCreatePath,
		serviceName,
		fmt.Sprintf("%d", masterPort),
		itemKey,
		workerType,
		masterAddr,
		common.Master,
		common.CoordK8sSvcName,
		common.Deployment,
	}

	km.UpdateConfig(strings.Join(command, " "))

	logger.Log.Println("[JobManager]: Creating yaml done")

	filename := common.YamlBasePath + serviceName + ".yaml"

	logger.Log.Println("[JobManager]: Creating Resources based on file, ", filename)

	rm := resourcemanager.InitResourceManager()
	rm.CreateResources(km, filename)
	rm.ReleaseResources()

	logger.Log.Println("[JobManager]: setup master done")
}

// DeployWorkerK8s
// * @Description: this func is only called by partyServer, run worker in local thread
// * @Date 2:14 下午 1/12/20
// * @Param
// 	masterAddr： IP of the masterAddr addr
//	workerType： train or inference worker
//	jobId： jobId
//	dataPath： data folder path of this party
//	modelPath： the path to save trained model
//	dataOutput： path to store processed data
//	workerID： id of the worker, mainly used to distinguish workers in distributed training
//	distributedRole： 0: ps, 1: worker
func DeployWorkerK8s(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string,
	workerID common.WorkerIdType) {

	workerPort := resourcemanager.GetFreePort(1)[0]

	workerAddr := common.PartyServerIP + ":" + fmt.Sprintf("%d", workerPort)
	var serviceName string

	if workerType == common.TrainWorker {

		serviceName = "worker-job" + jobId + "-train-" + fmt.Sprintf("%d", common.PartyID)
		logger.Log.Println("[JobManager]: Current in K8s, TrainWorker, svcName", serviceName)
	} else if workerType == common.InferenceWorker {
		serviceName = "worker-job" + jobId + "-predict-" + fmt.Sprintf("%d", common.PartyID)
		logger.Log.Println("[JobManager]: Current in K8s, InferenceWorker, svcName", serviceName)
	}

	km := resourcemanager.InitK8sManager(true, "")
	command := []string{
		common.WorkerYamlCreatePath,
		serviceName,                   // 1. worker service name
		fmt.Sprintf("%d", workerPort), // 2. worker service port
		masterAddr,                    // 3. master addr
		workerType,                    // 4. train or predict job
		workerAddr,                    // 5. worker addr
		workerType,                    // 6. serviceName train or predict
		common.Deployment,             // 7. env or prod
		common.LogPath,                // 8. folder to store logs, the same as partyserver folder currently,
		dataPath,                      // 9. folder to read train data
		modelPath,                     // 10. folder to store models
		dataOutput,                    // 11. folder to store processed data
		fmt.Sprintf("%d", workerID),   // 12. workerID
	}

	//_ = resourcemanager.ExecuteCmd("ls")
	//_ = resourcemanager.ExecuteCmd("pwd")
	km.UpdateConfig(strings.Join(command, " "))

	filename := common.YamlBasePath + serviceName + ".yaml"

	logger.Log.Println("[JobManager]: Creating yaml done", filename)

	rm := resourcemanager.InitResourceManager()

	go func() {
		defer logger.HandleErrors()
		rm.CreateResources(km, filename)
		rm.ReleaseResources()
		logger.Log.Println("[JobManager]: worker is running")
	}()

}
