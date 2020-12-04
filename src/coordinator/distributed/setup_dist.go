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

func SetupDist(httpHost, httpPort string, qItem *cache.QItem, taskType string) {


	httpAddr := httpHost + ":" + httpPort

	ms, masterAddress := SetupMaster(httpHost,httpAddr,qItem, taskType)

	if taskType == common.TrainTaskType{
		// update job's master address
		c.JobUpdateMaster(httpAddr, masterAddress, qItem.JobId)
	}

	for _, ip := range qItem.IPs {

		// Launch the worker

		// todo currently worker port is fixed, not a good design, change to dynamic later
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		c.SetupWorker(ip+":"+common.ListenerPort, masterAddress, taskType)
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
}

func SetupMasterHelper(httpHost, httpAddr string, qItem *cache.QItem, taskType string)  {
	if common.Env == common.DevEnv{

		SetupMaster(httpHost,httpAddr,qItem, taskType)

		// in prod, use k8s to run train/predict server as a isolate process
	}else if common.Env == common.ProdEnv{

		filename := common.PredictorYaml

		km := taskmanager.InitK8sManager(true,  "")
		km.CreateResources(filename)
}



}
func SetupMaster(httpHost,httpAddr string, qItem *cache.QItem, taskType string) (*master.Master,string) {
	/**
	 * @Author
	 * @Description : run train rpc server in a thread, used to test only
	 * @Date 2:26 下午 1/12/20
	 * @Param
	 * @return
	 **/
	logger.Do.Println("SetupDist: Lunching master")

	port, e := utils.GetFreePort()
	if e != nil {
		logger.Do.Println("SetupDist: Launch master Get port Error")
		panic(e)
	}
	masterAddress := httpHost + ":" + fmt.Sprintf("%d", port)
	ms := master.RunMaster("tcp", masterAddress, httpAddr, qItem, taskType)

	return ms, masterAddress

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
