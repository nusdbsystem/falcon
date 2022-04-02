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
	"falcon_platform/jobmanager/DAGscheduler"
	"falcon_platform/jobmanager/master"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"math/rand"
	"sync"
	"time"
)

func initSvcName() string {
	r := rand.New(rand.NewSource(time.Now().Unix()))
	res := fmt.Sprintf("%d", r.Int())
	return res
}

// RunJobManager
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

// ManageJobLifeCycle /**
// * @Description job manager run master and it allocates one master and
//				one or more workers to different party server
//				Master will call ManageJobLifeCycle
// * @Date 下午8:25 20/08/21
// * @Param
// * @return
// **/
func ManageJobLifeCycle(dslOjb *cache.DslObj, workerType string) {

	// 1. init DAG scheduler
	dagScheduler := DAGscheduler.NewDagScheduler(dslOjb)

	if stage, ok := dagScheduler.Stages[common.PreProcStage]; ok {
		// 1. generate master address
		ManageTaskLifeCycle(dslOjb, workerType, stage.Name, stage.TasksParallelism, len(stage.TasksParallelism), stage.AssignedWorker)
	}

	if stage, ok := dagScheduler.Stages[common.ModelTrainStage]; ok {
		// 1. generate master address
		ManageTaskLifeCycle(dslOjb, workerType, stage.Name, stage.TasksParallelism, len(stage.TasksParallelism), stage.AssignedWorker)
	}

	if stage, ok := dagScheduler.Stages[common.LimeInstanceSampleStage]; ok {
		// 1. generate master address
		ManageTaskLifeCycle(dslOjb, workerType, stage.Name, stage.TasksParallelism, len(stage.TasksParallelism), stage.AssignedWorker)
	}

	wg := sync.WaitGroup{}

	if stage, ok := dagScheduler.Stages[common.LimePredStage]; ok {
		wg.Add(1)
		go func(dslOjb *cache.DslObj, workerType string, stage *DAGscheduler.TaskStage, wg *sync.WaitGroup) {
			ManageTaskLifeCycle(dslOjb, workerType, stage.Name, stage.TasksParallelism, len(stage.TasksParallelism), stage.AssignedWorker)
			wg.Done()
		}(dslOjb, workerType, &stage, &wg)
	}

	if stage, ok := dagScheduler.Stages[common.LimeWeightStage]; ok {
		wg.Add(1)
		go func(dslOjb *cache.DslObj, workerType string, stage *DAGscheduler.TaskStage, wg *sync.WaitGroup) {
			ManageTaskLifeCycle(dslOjb, workerType, stage.Name, stage.TasksParallelism, len(stage.TasksParallelism), stage.AssignedWorker)
			wg.Done()
		}(dslOjb, workerType, &stage, &wg)
	}

	wg.Wait()

	if stage, ok := dagScheduler.Stages[common.LimeInterpretStage]; ok {
		// 1. generate master address
		ManageTaskLifeCycle(dslOjb, workerType, stage.Name, stage.TasksParallelism, len(stage.TasksParallelism), stage.AssignedWorker)
	}
}

// KillJob kill a job
func KillJob(masterAddr, network string) {
	ok := client.Call(masterAddr, network, "Master.KillJob", new(struct{}), new(struct{}))
	if ok == false {
		logger.Log.Println("[JobManager]: KillJob error")
		panic("[JobManager]: KillJob error")
	} else {
		logger.Log.Println("[JobManager]: KillJob Done")
	}
}

func ManageTaskLifeCycle(dslOjb *cache.DslObj, workerType string, stageName common.FalconStage, tasksParallelism map[common.FalconTask]int, groupNum int, assignedWorker int) string {

	masterPort := resourcemanager.GetFreePort(1)[0]
	masterAddr := common.CoordIP + ":" + fmt.Sprintf("%d", masterPort)
	//logger.Log.Println("dslOjb = ", dslOjb)
	masterIns := master.RunMaster(masterAddr, dslOjb, workerType, stageName)
	masterIns.Logger.Printf("[JobManager]: Assign port %d to master, run task=%s\n", masterPort, stageName)
	masterIns.Logger.Printf("[JobManager] begin ManageJobLifeCycle, masterAddr=%s, stageName=%s, groupNum=%s, tasksParallelism=%d\n", masterAddr, stageName, groupNum, tasksParallelism)
	masterIns.Logger.Println("[JobManager] call master.RunMaster with dslOjb")

	// update job's master addr
	if workerType == common.TrainWorker {
		client.JobUpdateMaster(common.CoordAddr, masterAddr, dslOjb.JobId)
	} else if workerType == common.InferenceWorker {
		client.InferenceUpdateMaster(common.CoordAddr, masterAddr, dslOjb.JobId)
	}

	isDistributed := 0
	if assignedWorker == 1 {
		// if assignedWorker = 1 disable distributed
		isDistributed = 0
	} else {
		// if assignedWorker > 1 disable distributed
		isDistributed = 1
	}

	// default use only 1 worker in centralized way.
	// in distributed training/inference, each party will launch multiple workers, and default use only 1 ps
	var workerPreGroup = assignedWorker
	// master will call party server's endpoint to launch worker, to train or predict
	for partyIndex, partyAddr := range dslOjb.PartyAddrList {

		dataPath := dslOjb.PartyInfoList[partyIndex].PartyPaths.DataInput
		dataOutput := dslOjb.PartyInfoList[partyIndex].PartyPaths.DataOutput
		modelPath := dslOjb.PartyInfoList[partyIndex].PartyPaths.ModelPath

		masterIns.Logger.Printf("[JobManager] master call partyserver %s to spawn workers \n", partyAddr)

		// call part server to launch worker to execute the task
		rep := client.RunWorker(partyAddr,
			masterAddr, workerType,
			fmt.Sprintf("%d", dslOjb.JobId),
			dataPath, modelPath, dataOutput,
			isDistributed,
			workerPreGroup, int(dslOjb.PartyNums),
			groupNum, string(stageName),
		)

		reply := new(common.RunWorkerReply)
		err := json.Unmarshal(rep, reply)
		if err != nil {
			panic(err)
		}

		decodedReply := common.DecodeLaunchResourceReply(reply.EncodedStr)
		masterIns.RequiredResource[partyIndex] = decodedReply
	}

	// extract necessary information
	masterIns.ExtractResourceInformation()

	masterIns.Logger.Println("[JobManager] master received all partyServer's reply")
	masterIns.BeginCountingWorkers.Broadcast()

	masterIns.Logger.Println("[JobManager] master wait here until stage finish")
	// wait this job finish
	masterIns.WaitJobComplete()

	masterIns.Logger.Println("[JobManager] master finish this stage")

	return masterAddr
}
