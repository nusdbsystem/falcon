package distributed

import (
	"coordinator/cache"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"fmt"
	"strings"
)


func SetupDistProd(qItem *cache.QItem, taskType string) {
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

	logger.Do.Println("SetupDist: Launch master ProdEnv")

	// in prod, use k8s to run train/predict server as a isolate process
	itemKey := "jid"+fmt.Sprintf("%d", qItem.JobId)

	serviceName := "master-" + itemKey + "-" + taskType

	// put to the queue, assign key to env
	logger.Do.Println("SetupDist: Writing item to redis")

	cache.InitRedisClient().Set(itemKey, cache.Serialize(qItem))

	logger.Do.Printf("SetupDist: Get key, %s InitK8sManager\n", itemKey)

	km := taskmanager.InitK8sManager(true,  "")

	command := []string{
		common.MasterYamlCreatePath,
		serviceName,
		masterPort,
		itemKey,
		taskType,
		masterAddress,
		common.MasterExecutor,
		common.CoordSvcName,
		common.Env,
	}

	//_=taskmanager.ExecuteOthers("ls")
	//_=taskmanager.ExecuteOthers("pwd")
	km.UpdateYaml(strings.Join(command, " "))

	logger.Do.Println("SetupDist: Creating yaml done")

	filename := common.YamlBasePath + serviceName + ".yaml"

	logger.Do.Println("SetupDist: Creating Resources based on file, ", filename)

	km.CreateResources(filename)
	logger.Do.Println("SetupDist: setup master done")
}


func SetupWorkerHelperProd(masterAddress, taskType, jobId, dataPath, modelPath, dataOutput string)  {

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

	if taskType == common.TrainExecutor{

		serviceName = "worker-jid" + jobId + "-train-" + common.PartyServerId

		logger.Do.Println("SetupWorkerHelper: Current in Prod, TrainExecutor, svcName", serviceName)
	}else if taskType == common.PredictExecutor{

		serviceName = "worker-jid" + jobId + "-predict-" + common.PartyServerId

		logger.Do.Println("SetupWorkerHelper: Current in Prod, PredictExecutor, svcName", serviceName)
	}

	km := taskmanager.InitK8sManager(true,  "")
	command := []string{
		common.WorkerYamlCreatePath,
		serviceName, 	// 1. worker service name
		workerPort,  	// 2. worker service port
		masterAddress,  // 3. master url
		taskType,		// 4. train or predict job
		workerAddress, 	// 5. worker url
		taskType,   	// 6. serviceName train or predict
		common.Env,  	// 7. env or prod
		common.ListenBasePath,  // 8. folder to store logs, the same as partyserver folder currently,
		dataPath, 		// 9. folder to read train data
		modelPath, 		// 10. folder to store models
		dataOutput, 	// 11. folder to store processed data
	}

	_=taskmanager.ExecuteOthers("ls")
	_=taskmanager.ExecuteOthers("pwd")
	km.UpdateYaml(strings.Join(command, " "))

	filename := common.YamlBasePath + serviceName + ".yaml"

	logger.Do.Println("SetupDist: Creating yaml done", filename)

	km.CreateResources(filename)


	logger.Do.Println("SetupDist: worker is running")

}