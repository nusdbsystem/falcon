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

func SetupDist(qItem *cache.QItem, workerType string) {
	/**
	 * @Author
	 * @Description run master, and then, master will call lister to run worker
	 * @Date 2:36 下午 5/12/20
	 * @Param
	 * @return
	 **/

	if common.Env == common.DevEnv{
		SetupDistDev(qItem, workerType)
	}else if common.Env == common.ProdEnv{
		SetupDistProd(qItem, workerType)
	}
}

func SetupWorkerHelper(masterUrl, workerType, jobId, dataPath, modelPath, dataOutput string)  {

	/**
	 * @Author
	 * @Description: this func is only called by partyserver
	 * @Date 2:14 下午 1/12/20
	 * @Param
	 	httpHost： 		IP of the partyserver url
		masterUrl： IP of the master url
		masterUrl： train or predictor
	 **/

	// in dev, use thread
	if common.Env == common.DevEnv{
		SetupWorkerHelperDev(masterUrl, workerType, jobId, dataPath, modelPath, dataOutput)
		// in prod, use k8s to run train/predict server as a isolate process
	}else if common.Env == common.ProdEnv{
		SetupWorkerHelperProd(masterUrl, workerType, jobId, dataPath, modelPath, dataOutput)
	}
}


func SetupMaster(masterUrl string, qItem *cache.QItem, workerType string) string {
	/**
	 * @Author
	 * @Description : run train rpc server in a thread, used to test only
	 * @Date 2:26 下午 1/12/20
	 * @Param
	 * @return
	 **/
	logger.Do.Println("SetupDist: Lunching master")

	ms := master.RunMaster(masterUrl, qItem, workerType)


	// update job's master url
	if workerType == common.TrainWorker{

		c.JobUpdateMaster(common.CoordinatorUrl, masterUrl, qItem.JobId)
	}

	// master will call lister's endpoint to launch worker, to train or predict
	for index, ip := range qItem.IPs {

		// Launch the worker
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		//lisPort := c.GetExistPort(common.CoordinatorUrl, ip)

		logger.Do.Printf("SetupDist: master is calling partyserver: %s ...\n", ip)

		// todo, manage partyserver port more wisely eg: c.SetupWorker(ip+lisPort, masterUrl, workerType), such that user dont need
		//  to provide port in job

		dataPath := qItem.PartyInfo[index].PartyPaths.DataInput
		dataOutput := qItem.PartyInfo[index].PartyPaths.DataOutput
		modelPath := qItem.PartyInfo[index].PartyPaths.ModelPath

		c.SetupWorker(ip, masterUrl, workerType, fmt.Sprintf("%d", qItem.JobId), dataPath, modelPath, dataOutput)
	}

	logger.Do.Printf("SetupDist: master is running at %s ... waiting job complete\n", masterUrl)

	ms.Wait()

	logger.Do.Println("SetupDist: master finish all jobs")

	return masterUrl
}

func CleanWorker(){

	// todo delete the svc created for training, master will call this method after ms.Wait()
}


func KillJob(masterUrl, Proxy string) {
	ok := c.Call(masterUrl, Proxy, "Master.KillJob", new(struct{}), new(struct{}))
	if ok == false {
		logger.Do.Println("Master: KillJob error")
		panic("Master: KillJob error")
	} else {
		logger.Do.Println("Master: KillJob Done")
	}
}