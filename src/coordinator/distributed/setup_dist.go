package distributed

import (
	"coordinator/cache"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/master"
	"coordinator/logger"
	"fmt"
	"math/rand"
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

	if common.Env == common.DevEnv{
		SetupDistDev(qItem, taskType)
	}else if common.Env == common.ProdEnv{
		SetupDistProd(qItem, taskType)
	}
}

func SetupWorkerHelper(masterAddress, taskType, jobId, dataPath, modelPath, dataOutput string)  {

	/**
	 * @Author
	 * @Description: this func is only called by partyserver
	 * @Date 2:14 下午 1/12/20
	 * @Param
	 	httpHost： 		IP of the partyserver address
		masterAddress： IP of the master address
		masterAddress： train or predictor
	 **/

	// in dev, use thread
	if common.Env == common.DevEnv{
		SetupWorkerHelperDev(masterAddress, taskType, jobId, dataPath, modelPath, dataOutput)
		// in prod, use k8s to run train/predict server as a isolate process
	}else if common.Env == common.ProdEnv{
		SetupWorkerHelperProd(masterAddress, taskType, jobId, dataPath, modelPath, dataOutput)
	}
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
	for index, ip := range qItem.IPs {

		// Launch the worker
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		//lisPort := c.GetExistPort(common.CoordSvcURLGlobal, ip)

		logger.Do.Printf("SetupDist: master is calling partyserver: %s ...\n", ip)

		// todo, manage partyserver port more wisely eg: c.SetupWorker(ip+lisPort, masterAddress, taskType), such that user dont need
		//  to provide port in job

		dataPath := qItem.PartyInfos[index].PartyPaths.DataInput
		dataOutput := qItem.PartyInfos[index].PartyPaths.DataOutput
		modelPath := qItem.PartyInfos[index].PartyPaths.ModelPath

		c.SetupWorker(ip, masterAddress, taskType, fmt.Sprintf("%d", qItem.JobId), dataPath, modelPath, dataOutput)
	}

	logger.Do.Printf("SetupDist: master is running at %s ... waiting job complete\n", masterAddress)

	ms.Wait()

	logger.Do.Println("SetupDist: master finish all jobs")

	return masterAddress
}

func CleanWorker(){

	// todo delete the svc created for training, master will call this method after ms.Wait()
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