/*
	Job Manager includes one master and multiple workers, It manage the job lifecycle, and call resource manager
    to deploy the job. After job complete, master and worker will stop.
*/

package jobmanager

import (
	"encoding/json"
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/master"
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

// JobManager includes one master and many workers
// it firstly deploy and run master, master will call partyServer to deploy and run worker
func RunJobManager(dslOjb *cache.DslObj, workerType string) {

	if common.Deployment == common.LocalThread {
		logger.Log.Printf("[JobManager]: Deploy master with thread")
		DeployMasterThread(dslOjb, workerType)

	} else if common.Deployment == common.Docker {
		logger.Log.Printf("[JobManager]: Deploy master with docker")
		DeployMasterDocker(dslOjb, workerType)

	} else if common.Deployment == common.K8S {
		logger.Log.Printf("[JobManager]: Deploy master with k8s ")
		DeployMasterK8s(dslOjb, workerType)
	}
}

/**
 * @Description job manager run master and it allocates one master and
				one or more workers to different party server
				Master will call ManageJobLifeCycle
 * @Date 下午8:25 20/08/21
 * @Param
 * @return
 **/
func ManageJobLifeCycle(masterAddr string, dslOjb *cache.DslObj, workerType string) string {

	logger.Log.Printf("[JobManager] begin ManageJobLifeCycle, masterAddr=%s workerType=%s\n", masterAddr, workerType)

	logger.Log.Println("[JobManager] call master.RunMaster with dslOjb:")
	//logger.Log.Println("dslOjb = ", dslOjb)
	ms := master.RunMaster(masterAddr, dslOjb, workerType)

	// update job's master addr
	if workerType == common.TrainWorker {

		client.JobUpdateMaster(common.CoordAddr, masterAddr, dslOjb.JobId)

	} else if workerType == common.InferenceWorker {

		client.InferenceUpdateMaster(common.CoordAddr, masterAddr, dslOjb.JobId)
	}

	// default use only 1 worker in centralized way.
	// in distributed training/inference, each party will launch multiple workers, and default use only 1 ps
	var workerGroupNum = 1
	if dslOjb.DistributedTask.Enable == 1 {
		parameterServerNum := 1
		workerGroupNum = dslOjb.DistributedTask.WorkerNumber + parameterServerNum
	}

	// master will call party server's endpoint to launch worker, to train or predict
	for partyIndex, partyAddr := range dslOjb.PartyAddrList {

		dataPath := dslOjb.PartyInfoList[partyIndex].PartyPaths.DataInput
		dataOutput := dslOjb.PartyInfoList[partyIndex].PartyPaths.DataOutput
		modelPath := dslOjb.PartyInfoList[partyIndex].PartyPaths.ModelPath

		logger.Log.Printf("[JobManager] master call partyserver %s to spawn workers \n", partyAddr)

		// call part server to launch worker to execute the task
		rep := client.RunWorker(partyAddr,
			masterAddr, workerType,
			fmt.Sprintf("%d", dslOjb.JobId),
			dataPath, modelPath, dataOutput,
			workerGroupNum, int(dslOjb.PartyNums),
		)

		reply := new(common.RunWorkerReply)
		err := json.Unmarshal(rep, reply)
		if err != nil {
			panic(err)
		}

		decodedReply := common.DecodeLaunchResourceReply(reply.EncodedStr)
		ms.RequiredResource[partyIndex] = decodedReply
	}

	// extract necessary information
	ms.ExtractResourceInformation()

	logger.Log.Println("[JobManager] master received all partyServer's reply")
	ms.BeginCountingWorkers.Broadcast()

	logger.Log.Println("[JobManager] master wait here until job finish")
	// wait this job finish
	ms.WaitJobComplete()

	logger.Log.Println("[JobManager] master finish all jobs")

	return masterAddr
}

// kill a job
func KillJob(masterAddr, network string) {
	ok := client.Call(masterAddr, network, "Master.KillJob", new(struct{}), new(struct{}))
	if ok == false {
		logger.Log.Println("[JobManager]: KillJob error")
		panic("[JobManager]: KillJob error")
	} else {
		logger.Log.Println("[JobManager]: KillJob Done")
	}
}
