package distributed

import (
	"coordinator/cache"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/master"
	"coordinator/distributed/prediction"
	"coordinator/distributed/taskmanager"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"fmt"
	"math/rand"
	"strings"
	"time"
)

func initSvcName() string{
	r := rand.New(rand.NewSource(time.Now().Unix()))
	res := fmt.Sprintf("%d", r.Int())
	return res
}

func SetupDist(qItem *cache.QItem, taskType string) {
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

	if common.Env == common.DevEnv{
		logger.Do.Println("SetupDist: Launch master DevEnv")

		// use a thread
		SetupMaster(masterAddress, qItem, taskType)

	}else if common.Env == common.ProdEnv{
		logger.Do.Println("SetupDist: Launch master ProdEnv")

		// in prod, use k8s to run train/predict server as a isolate process
		itemKey := "jid"+fmt.Sprintf("%d", qItem.JobId)

		serviceName := "master-" + itemKey

		// put to the queue, assign key to env
		logger.Do.Println("SetupDist: Writing item to redis")

		cache.RedisClient.Set(itemKey, cache.Serialize(qItem))

		logger.Do.Printf("SetupDist: Get key, %s InitK8sManager\n", itemKey)

		km := taskmanager.InitK8sManager(true,  "")

		command := []string{
			common.MasterYamlCreatePath,
			serviceName,
			masterPort,
			itemKey,
			taskType,
			masterAddress,
		}

		//_=taskmanager.ExecuteOthers("ls")
		//_=taskmanager.ExecuteOthers("pwd")
		km.UpdateYaml(strings.Join(command, " "))

		logger.Do.Println("SetupDist: Creating yaml done")

		filename := common.YamlBasePath + serviceName + ".yaml"

		logger.Do.Println("SetupDist: Creating Resources based on file, ", filename)

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

func SetupWorkerHelper(masterAddress, taskType string)  {

	/**
	 * @Author
	 * @Description: this func is only called by listener
	 * @Date 2:14 下午 1/12/20
	 * @Param
	 	httpHost： 		IP of the listener address
		masterAddress： IP of the master address
		masterAddress： train or predictor
	 **/

	workerPort := c.GetFreePort(common.CoordURLGlobal)

	workerAddress := common.ListenAddrGlobal + ":" + workerPort

	// in dev, use thread
	if common.Env == common.DevEnv{

		if taskType == common.TrainExecutor{

			worker.RunWorker(masterAddress, workerAddress)

		}else if taskType == common.PredictExecutor{

			prediction.RunPrediction(masterAddress, workerAddress)
		}

		// in prod, use k8s to run train/predict server as a isolate process
	}else if common.Env == common.ProdEnv{
		itemKey := initSvcName()

		var serviceName string

		if taskType == common.TrainExecutor{
			serviceName = "train-" + itemKey

		}else if taskType == common.PredictExecutor{
			serviceName = "predict-" + itemKey
		}

		km := taskmanager.InitK8sManager(true,  "")
		command := []string{
			common.WorkerYamlCreatePath,
			serviceName,
			workerPort,
			masterAddress,
			taskType,
		}

		km.UpdateYaml(strings.Join(command, " "))

		filename := common.YamlBasePath + serviceName + ".yaml"

		km.CreateResources(filename)

	}

	logger.Do.Println("SetupDist: worker is running")

}


func SetupMaster(masterAddress string, qItem *cache.QItem, taskType string) string {
	/**
	 * @Author
	 * @Description : run train rpc server in a thread, used to test only
	 * @Date 2:26 下午 1/12/20
	 * @Param
	 * @return
	 **/
	logger.Do.Println("SetupDist: Lunching master")

	ms := master.RunMaster(masterAddress, qItem, taskType)


	// update job's master address
	if taskType == common.TrainExecutor{

		c.JobUpdateMaster(common.CoordSvcURLGlobal, masterAddress, qItem.JobId)
	}

	// master will call lister's endpoint to launch worker, to train or predict
	for _, ip := range qItem.IPs {

		// Launch the worker
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		c.SetupWorker(ip+":"+common.ListenerPort, masterAddress, taskType)
	}

	logger.Do.Printf("SetupDist: master is running at %s ... waiting\n", masterAddress)

	ms.Wait()

	return masterAddress
}

