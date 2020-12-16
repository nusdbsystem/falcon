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

func SetupWorkerHelper(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string)  {

	/**
	 * @Author
	 * @Description: this func is only called by partyserver
	 * @Date 2:14 下午 1/12/20
	 * @Param
	 	httpHost： 		IP of the partyserver addr
		masterAddr： IP of the master addr
		masterAddr： train or predictor
	 **/

	// in dev, use thread
	if common.Env == common.DevEnv{
		SetupWorkerHelperDev(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput)
		// in prod, use k8s to run train/predict server as a isolate process
	}else if common.Env == common.ProdEnv{
		SetupWorkerHelperProd(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput)
	}
}


func SetupMaster(masterAddr string, qItem *cache.QItem, workerType string) string {
	/**
	 * @Author
	 * @Description : run train rpc server in a thread, used to test only
	 * @Date 2:26 下午 1/12/20
	 * @Param
	 * @return
	 **/
	logger.Do.Println("SetupDist: Launching master")

	ms := master.RunMaster(masterAddr, qItem, workerType)


	// update job's master addr
	if workerType == common.TrainWorker{

		c.JobUpdateMaster(common.CoordAddr, masterAddr, qItem.JobId)
	}

	// master will call lister's endpoint to launch worker, to train or predict
	for index, addr := range qItem.AddrList {

		// Launch the worker
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		//lisPort := c.GetExistPort(common.CoordAddr, ip)

		logger.Do.Printf("SetupDist: master is calling partyserver: %s ...\n", addr)

		// todo, manage partyserver port more wisely eg: c.SetupWorker(ip+lisPort, masterAddr, workerType), such that user dont need
		//  to provide port in job

		dataPath := qItem.PartyInfo[index].PartyPaths.DataInput
		dataOutput := qItem.PartyInfo[index].PartyPaths.DataOutput
		modelPath := qItem.PartyInfo[index].PartyPaths.ModelPath

		c.SetupWorker(addr, masterAddr, workerType, fmt.Sprintf("%d", qItem.JobId), dataPath, modelPath, dataOutput)
	}

	logger.Do.Printf("SetupDist: master is running at %s ... waiting job complete\n", masterAddr)

	ms.Wait()

	logger.Do.Println("SetupDist: master finish all jobs")

	return masterAddr
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