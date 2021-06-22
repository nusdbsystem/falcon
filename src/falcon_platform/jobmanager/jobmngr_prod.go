package jobmanager

import (
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"strings"
)

func SetupJobManagerProd(dslOjb *cache.DslObj, workerType string) {
	// run master to call party server to set up worker

	masterPort := client.GetFreePort(common.CoordAddr)
	logger.Log.Println("SetupJobManager: Launch master Get port", masterPort)

	masterIP := common.CoordIP
	masterAddr := masterIP + ":" + masterPort

	logger.Log.Println("SetupJobManager: Launch master K8sEnv")

	// in prod, use k8s to run train/predict server as a isolate process
	itemKey := "job" + fmt.Sprintf("%d", dslOjb.JobId)

	serviceName := "master-" + itemKey + "-" + strings.ToLower(workerType)

	// put to the dslqueue, assign key to env
	logger.Log.Println("SetupJobManager: Writing item to redis")

	cache.InitRedisClient().Set(itemKey, cache.Serialize(dslOjb))

	logger.Log.Printf("SetupJobManager: Get key, %s InitK8sManager\n", itemKey)

	km := resourcemanager.InitK8sManager(true, "")

	command := []string{
		common.MasterYamlCreatePath,
		serviceName,
		masterPort,
		itemKey,
		workerType,
		masterAddr,
		common.Master,
		common.CoordK8sSvcName,
		common.Env,
	}

	//_=resourcemanager.ExecuteOthers("ls")
	//_=resourcemanager.ExecuteOthers("pwd")
	km.UpdateYaml(strings.Join(command, " "))

	logger.Log.Println("SetupJobManager: Creating yaml done")

	filename := common.YamlBasePath + serviceName + ".yaml"

	logger.Log.Println("SetupJobManager: Creating Resources based on file, ", filename)

	km.CreateResources(filename)
	logger.Log.Println("SetupJobManager: setup master done")
}

func SetupWorkerHelperProd(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string) {

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

	if workerType == common.TrainWorker {

		serviceName = "worker-job" + jobId + "-train-" + common.PartyID

		logger.Log.Println("SetupWorkerHelper: Current in Prod, TrainWorker, svcName", serviceName)
	} else if workerType == common.InferenceWorker {

		serviceName = "worker-job" + jobId + "-predict-" + common.PartyID

		logger.Log.Println("SetupWorkerHelper: Current in Prod, InferenceWorker, svcName", serviceName)
	}

	km := resourcemanager.InitK8sManager(true, "")
	command := []string{
		common.WorkerYamlCreatePath,
		serviceName,                // 1. worker service name
		workerPort,                 // 2. worker service port
		masterAddr,                 // 3. master addr
		workerType,                 // 4. train or predict job
		workerAddr,                 // 5. worker addr
		workerType,                 // 6. serviceName train or predict
		common.Env,                 // 7. env or prod
		common.PartyServerBasePath, // 8. folder to store logs, the same as partyserver folder currently,
		dataPath,                   // 9. folder to read train data
		modelPath,                  // 10. folder to store models
		dataOutput,                 // 11. folder to store processed data
	}

	//_ = resourcemanager.ExecuteCmd("ls")
	//_ = resourcemanager.ExecuteCmd("pwd")
	km.UpdateYaml(strings.Join(command, " "))

	filename := common.YamlBasePath + serviceName + ".yaml"

	logger.Log.Println("SetupJobManager: Creating yaml done", filename)

	km.CreateResources(filename)

	logger.Log.Println("SetupJobManager: worker is running")

}
