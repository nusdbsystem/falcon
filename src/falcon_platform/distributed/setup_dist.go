package distributed

import (
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/distributed/master"
	"falcon_platform/logger"
	"fmt"
	"math/rand"
	"time"
)

func initSvcName() string {
	r := rand.New(rand.NewSource(time.Now().Unix()))
	res := fmt.Sprintf("%d", r.Int())
	return res
}

func SetupDist(qItem *cache.QItem, workerType string) {
	// run master to call partyserver to set up worker

	if common.Env == common.DevEnv {
		SetupDistDev(qItem, workerType)
	} else if common.Env == common.ProdEnv {
		SetupDistProd(qItem, workerType)
	}
}

func SetupWorkerHelper(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string) {
	// this func is only called by partyserver

	// in dev, use thread
	if common.Env == common.DevEnv {
		SetupWorkerHelperDev(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput)
		// in prod, use k8s to run train/predict server as a isolate process
	} else if common.Env == common.ProdEnv {
		SetupWorkerHelperProd(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput)
	}
}

func SetupMaster(masterAddr string, qItem *cache.QItem, workerType string) string {
	// run train rpc server in a thread

	logger.Log.Printf("[SetupMaster] masterAddr=%s workerType=%s\n", masterAddr, workerType)

	logger.Log.Println("[SetupMaster] call master.RunMaster with qItem:")
	logger.Log.Println("qItem = ", qItem)
	ms := master.RunMaster(masterAddr, qItem, workerType)

	// update job's master addr
	if workerType == common.TrainWorker {
		logger.Log.Printf(
			"[SetupMaster] TrainWorker => call client.JobUpdateMaster at CoordArrd=%s, masterAddr=%s, JobId=%d\n",
			common.CoordAddr, masterAddr, qItem.JobId)
		client.JobUpdateMaster(common.CoordAddr, masterAddr, qItem.JobId)

	} else if workerType == common.InferenceWorker {
		logger.Log.Printf(
			"[SetupMaster] InferenceWorker => call client.InferenceUpdateMaster at CoordArrd=%s, masterAddr=%s, JobId=%d\n",
			common.CoordAddr, masterAddr, qItem.JobId)
		client.InferenceUpdateMaster(common.CoordAddr, masterAddr, qItem.JobId)
	}

	// master will call partyserver's endpoint to launch worker, to train or predict
	for index, partyAddr := range qItem.AddrList {

		// Launch the worker
		// maybe check table wit IP, and + port got from table also

		// send a request to http
		//lisPort := client.GetExistPort(common.CoordAddr, IP)

		// todo, manage partyserver port more wisely eg: client.SetupWorker(IP+lisPort, masterAddr, workerType), such that user dont need
		//  to provide port in job

		dataPath := qItem.PartyInfo[index].PartyPaths.DataInput
		dataOutput := qItem.PartyInfo[index].PartyPaths.DataOutput
		modelPath := qItem.PartyInfo[index].PartyPaths.ModelPath

		logger.Log.Printf("[SetupMaster] master register/dispatch job to partyserver: %s ...\n", partyAddr)
		logger.Log.Printf("[SetupMaster] JobId=%d\n", qItem.JobId)
		logger.Log.Printf("[SetupMaster] dataPath=%s\n", dataPath)
		logger.Log.Printf("[SetupMaster] modelPath=%s\n", modelPath)
		logger.Log.Printf("[SetupMaster] dataOutput=%s\n", dataOutput)

		client.SetupWorker(partyAddr, masterAddr, workerType, fmt.Sprintf("%d", qItem.JobId), dataPath, modelPath, dataOutput)
	}

	ms.Wait()

	logger.Log.Println("[SetupMaster] master finish all jobs")

	return masterAddr
}

func KillJob(masterAddr, network string) {
	ok := client.Call(masterAddr, network, "Master.KillJob", new(struct{}), new(struct{}))
	if ok == false {
		logger.Log.Println("Master: KillJob error")
		panic("Master: KillJob error")
	} else {
		logger.Log.Println("Master: KillJob Done")
	}
}
