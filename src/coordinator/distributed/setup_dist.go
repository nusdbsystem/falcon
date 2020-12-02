package distributed

import (
	c "coordinator/client"
	"coordinator/config"
	"coordinator/distributed/master"
	"coordinator/distributed/prediction"
	"coordinator/distributed/taskmanager"
	"coordinator/distributed/utils"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"fmt"
)

func SetupDist(httpHost, httpPort string, qItem *config.QItem, taskType string) {
	logger.Do.Println("SetupDist: Lunching master")


	httpAddr := httpHost + ":" + httpPort
	port, e := utils.GetFreePort()

	if e != nil {
		logger.Do.Println("Get port Error")
		return
	}
	masterAddress := httpHost + ":" + fmt.Sprintf("%d", port)
	ms := master.RunMaster("tcp", masterAddress, httpAddr, qItem, taskType)

	if taskType == config.TrainTaskType{
		// update job's master address
		c.JobUpdateMaster(httpAddr, masterAddress, qItem.JobId)
	}

	for _, ip := range qItem.IPs {

		// Launch the worker

		// todo currently worker port is fixed, not a good design, change to dynamic later
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		c.SetupWorker(ip+":"+config.ListenerPort, masterAddress, taskType)
	}

	// wait until job done
	ms.Wait()
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
	if config.Env == config.DevEnv{

		if taskType == config.TrainTaskType{

			SetupTrain(httpHost, masterAddress)

		}else if taskType == config.PredictTaskType{

			SetupPrediction(httpHost, masterAddress)
		}

		// in prod, use k8s to run train/predict server as a isolate process
	}else if config.Env == config.ProdEnv{

		var filename string

		if taskType == config.TrainTaskType{
			filename = config.TrainYaml

		}else if taskType == config.PredictTaskType{
			filename = config.PredictorYaml
		}

		km := taskmanager.InitK8sManager(true,  "")
		km.CreateResources(filename)

	}
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
	worker.RunWorker(masterAddress, "tcp", httpHost, fmt.Sprintf("%d", port))

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
//if taskType == config.TrainTaskType{
//	args = []string{
//		fmt.Sprintf("-svc %s -wip %s -master_addr %s",
//		config.TrainTaskType,
//		httpHost,
//		masterAddress)}
//
//}else if taskType == config.PredictTaskType{
//	args = []string{
//		fmt.Sprintf("-svc %s -wip %s -master_addr %s",
//			config.PredictTaskType,
//			httpHost,
//			masterAddress)}
//	pm.IsWait = false
//
//}
//
//killed, e, el, ol := pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
//logger.Do.Println(killed, e, el, ol)
