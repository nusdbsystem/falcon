/*
	Job Manager includes one master and multiple workers, It manages the job lifecycle, and call resource manager
    to deploy the job. After job complete, master and worker will stop.
*/

package jobmanager

import (
	"encoding/json"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/DAGscheduler"
	"falcon_platform/jobmanager/comms_pattern"
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

// ManageJobLifeCycle
// it will run one master and many workers for each tasks.
// It should schedule whole tasks as DAG
func manageJobLifeCycle(job *common.TrainJob, workerType string) {

	var jobStatus string
// 	if job.DistributedTask.WorkerNumber >= 3+2*int(job.ClassNum) {
	if job.DistributedTask.WorkerNumber >= 3 {
		jobStatus = parallellySchedule(job, workerType)
	} else {
		jobStatus = sequenceSchedule(job, workerType)
	}

	// call coordinator to update status
	client.JobUpdateResInfo(common.CoordAddr, "", jobStatus, "", job.JobId)
	client.JobUpdateStatus(common.CoordAddr, jobStatus, job.JobId)
	client.ModelUpdate(common.CoordAddr, 1, job.JobId)
}

func sequenceSchedule(job *common.TrainJob, workerType string) (jobStatus string) {
	status := true
	jobStatus = common.JobSuccessful

	// 1. init DAG scheduler
	dagScheduler := DAGscheduler.NewDagScheduler(job)
	if dagScheduler.ParallelismPolicy.IsValid == false {
		logger.Log.Printf("[JobManager]: Cannot find a valid schedule policy according to current worker_num=%d, deadline=10k(default), class_number=%d\n", job.DistributedTask.WorkerNumber, job.ClassNum)
		jobStatus = common.JobFailed
		return
	}
	dagScheduler.SplitTaskIntoStage(job)

	if stage, ok := dagScheduler.DagTasks[common.PreProcTaskKey]; ok {
		// 1. generate master address
		status = manageTaskLifeCycle(*job, workerType, stage.Name, 1, 0)

		logger.Log.Printf("[JobManager]: stage PreProcStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	if stage, ok := dagScheduler.DagTasks[common.ModelTrainTaskKey]; ok {
		// 1. generate master address
		status = manageTaskLifeCycle(*job, workerType, stage.Name, stage.AssignedWorker, 0)

		logger.Log.Printf("[JobManager]: stage ModelTrainStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	if stage, ok := dagScheduler.DagTasks[common.LimeInstanceSampleTask]; ok {
		// 1. generate master address
		status = manageTaskLifeCycle(*job, workerType, stage.Name, 1, 0)
		logger.Log.Printf("[JobManager]: stage LimeInstanceSampleStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	if stage, ok := dagScheduler.DagTasks[common.LimePredTaskKey]; ok {
		status = manageTaskLifeCycle(*job, workerType, stage.Name, 1, 0)
		logger.Log.Printf("[JobManager]: stage LimePredStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	if stage, ok := dagScheduler.DagTasks[common.LimeWeightTaskKey]; ok {
		status = manageTaskLifeCycle(*job, workerType, stage.Name, 1, 0)
		logger.Log.Printf("[JobManager]: stage LimePredStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	var classNum int
	if job.Tasks.LimeFeature.AlgorithmName != "" {
		classNum = int(job.Tasks.LimeFeature.ClassNum)
	} else if job.Tasks.LimeInterpret.AlgorithmName != "" {
		classNum = int(job.Tasks.LimeInterpret.ClassNum)
	}

	for classId := 0; classId < classNum; classId++ {
		// assign one parallelism to this class

		logger.Log.Printf("[JobManager]: One thread start for class %d", classId)

		var selectFeatureFile = ""
		var status45 bool = true
		// feature selection
		if stage, ok := dagScheduler.DagTasks[common.LimeFeatureTaskKey]; ok {
			job.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig, _, selectFeatureFile =
				common.GenerateLimeFeatSelParams(job.Tasks.LimeFeature.InputConfigs.AlgorithmConfig, int32(classId), job.Tasks.LimeFeature.MpcAlgorithmName)
			status45 = manageTaskLifeCycle(*job, workerType, stage.Name, 1, classId)
			if !status {
				jobStatus = common.JobFailed
				return
			} else {
				jobStatus = common.JobSuccessful
			}
		}

		// model training, if there is this task, and previous stage is correct.
		if stage, ok := dagScheduler.DagTasks[common.LimeInterpretTaskKey]; ok && status45 {
			// generate SerializedAlgorithmConfig
			job.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig, _ =
				common.GenerateLimeInterpretParams(job.Tasks.LimeInterpret.InputConfigs.AlgorithmConfig,
					int32(classId), selectFeatureFile, job.Tasks.LimeInterpret.MpcAlgorithmName)

			status45 = manageTaskLifeCycle(*job, workerType, stage.Name, 1, classId)
			if !status {
				jobStatus = common.JobFailed
				return
			} else {
				jobStatus = common.JobSuccessful
			}
		}

		// after finishing the tasks, append the groupIdParam back to availableGroupIds
		logger.Log.Printf("[JobManager]: one thread done for class %d", classId)
	}

	return
}

func parallellySchedule(job *common.TrainJob, workerType string) (jobStatus string) {

	status := true
	jobStatus = common.JobSuccessful

	// 1. init DAG scheduler
	dagScheduler := DAGscheduler.NewDagScheduler(job)
	if dagScheduler.ParallelismPolicy.IsValid == false {
		logger.Log.Printf("[JobManager]: Cannot find a valid schedule policy according to current worker_num=%d, deadline=10k(default), class_number=%d\n", job.DistributedTask.WorkerNumber, job.ClassNum)
		jobStatus = common.JobFailed
		return
	}
	dagScheduler.SplitTaskIntoStage(job)

	if stage, ok := dagScheduler.DagTasks[common.PreProcTaskKey]; ok {
		// 1. generate master address
		status = manageTaskLifeCycle(*job, workerType, stage.Name, stage.AssignedWorker, 0)

		logger.Log.Printf("[JobManager]: stage PreProcStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	if stage, ok := dagScheduler.DagTasks[common.ModelTrainTaskKey]; ok {
		// 1. generate master address
		status = manageTaskLifeCycle(*job, workerType, stage.Name, stage.AssignedWorker, 0)

		logger.Log.Printf("[JobManager]: stage ModelTrainStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}

	}

	if stage, ok := dagScheduler.DagTasks[common.LimeInstanceSampleTask]; ok {
		// 1. generate master address
		status = manageTaskLifeCycle(*job, workerType, stage.Name, stage.AssignedWorker, 0)
		logger.Log.Printf("[JobManager]: stage LimeInstanceSampleStage status = %d\n", status)
		if !status {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	// parallel run prediction + weighting
	_, predExist := dagScheduler.DagTasks[common.LimePredTaskKey]
	_, weightExist := dagScheduler.DagTasks[common.LimeWeightTaskKey]
	if predExist || weightExist {
		statusPrediction := true
		statusWeight := true
		wg23 := sync.WaitGroup{}
		if stage, ok := dagScheduler.DagTasks[common.LimePredTaskKey]; ok {
			wg23.Add(1)
			go func(job common.TrainJob, workerType string, stage *DAGscheduler.TaskStage, wg23 *sync.WaitGroup, status *bool) {
				*status = manageTaskLifeCycle(job, workerType, stage.Name, stage.AssignedWorker, 0)
				wg23.Done()
			}(*job, workerType, &stage, &wg23, &statusPrediction)
		}

		if stage, ok := dagScheduler.DagTasks[common.LimeWeightTaskKey]; ok {
			wg23.Add(1)
			go func(job common.TrainJob, workerType string, stage *DAGscheduler.TaskStage, wg23 *sync.WaitGroup, status *bool) {
				*status = manageTaskLifeCycle(job, workerType, stage.Name, stage.AssignedWorker, 0)
				wg23.Done()
			}(*job, workerType, &stage, &wg23, &statusWeight)
		}
		wg23.Wait()
		logger.Log.Printf("[JobManager]: stage LimeWeightStage or LimePredStage status = %d\n", statusPrediction && statusWeight)
		if !(statusPrediction && statusWeight) {
			jobStatus = common.JobFailed
			return
		} else {
			jobStatus = common.JobSuccessful
		}
	}

	_, featureExist := dagScheduler.DagTasks[common.LimeFeatureTaskKey]
	_, InterpretExist := dagScheduler.DagTasks[common.LimeInterpretTaskKey]
	if featureExist || InterpretExist {
		var classNum int
		if job.Tasks.LimeFeature.AlgorithmName != "" {
			classNum = int(job.Tasks.LimeFeature.ClassNum)
		} else if job.Tasks.LimeInterpret.AlgorithmName != "" {
			classNum = int(job.Tasks.LimeInterpret.ClassNum)
		}

		// parallel run feature selection  + training across many classes.
		wg45 := sync.WaitGroup{}
		wg45.Add(classNum)

		var classParallelismLock sync.Mutex

		classParallelism := dagScheduler.ParallelismPolicy.LimeClassParallelism

		var status45list []string

		// for each class, execute the stage
		for classId := 0; classId < classNum; classId++ {
			// assign one parallelism to this class
			classParallelismLock.Lock()
			classParallelism -= 1
			classParallelismLock.Unlock()
			// schedule this class
			go func(classIdParam int, classParallelismParam *int, wgParam *sync.WaitGroup, job common.TrainJob, status45Para *[]string) {

				logger.Log.Printf("[JobManager]: One thread start for class %d", classIdParam)

				var selectFeatureFile = ""
				var status45 bool = true
				// feature selection
				if stage, ok := dagScheduler.DagTasks[common.LimeFeatureTaskKey]; ok {
					job.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig, _, selectFeatureFile =
						common.GenerateLimeFeatSelParams(job.Tasks.LimeFeature.InputConfigs.AlgorithmConfig, int32(classIdParam), job.Tasks.LimeFeature.MpcAlgorithmName)
					status45 = manageTaskLifeCycle(job, workerType, stage.Name, stage.AssignedWorker, classIdParam)
					if !status45 {
						*status45Para = append(*status45Para, common.JobFailed)
						return
					} else {
						*status45Para = append(*status45Para, common.JobSuccessful)
					}
				}

				// model training, if there is this task, and previous stage is correct.
				if stage, ok := dagScheduler.DagTasks[common.LimeInterpretTaskKey]; ok && status45 {
					// generate SerializedAlgorithmConfig
					job.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig, _ =
						common.GenerateLimeInterpretParams(job.Tasks.LimeInterpret.InputConfigs.AlgorithmConfig,
							int32(classIdParam), selectFeatureFile, job.Tasks.LimeInterpret.MpcAlgorithmName)

					status45 = manageTaskLifeCycle(job, workerType, stage.Name, stage.AssignedWorker, classIdParam)
					if !status45 {
						// mark as done.
						*status45Para = append(*status45Para, common.JobFailed)
						return
					} else {
						*status45Para = append(*status45Para, common.JobSuccessful)
					}
				}

				// after finishing the tasks, append the groupIdParam back to availableGroupIds
				classParallelismLock.Lock()
				*classParallelismParam = *classParallelismParam + 1
				classParallelismLock.Unlock()
				logger.Log.Printf("[JobManager]: one thread done for class %d", classIdParam)
				// mark as done.
				wgParam.Done()

			}(classId, &classParallelism, &wg45, *job, &status45list)

			// wait until previous class finish
			for {
				classParallelismLock.Lock()
				if classParallelism > 0 {
					classParallelismLock.Unlock()
					break
				} else {
					classParallelismLock.Unlock()
					time.Sleep(1 * time.Second)
				}
			}

		}

		logger.Log.Printf("[JobManager]: waiting for task 45 finish classNum = %d", classNum)
		wg45.Wait()

		for _, status45 := range status45list {
			if status45 == common.JobFailed {
				jobStatus = common.JobFailed
				return
			} else {
				jobStatus = common.JobSuccessful
			}
		}
	}
	return
}

// job: train or inference
// workerType: train or inference
// current taskName
func manageTaskLifeCycle(job common.TrainJob, workerType string, taskName common.FalconTask, assignedWorker int, classID int) bool {

	masterPort := resourcemanager.GetFreePort(1)[0]
	masterAddr := common.CoordIP + ":" + fmt.Sprintf("%d", masterPort)

	// update distributed information
	if assignedWorker <= 2 {
		// if assignedWorker = 1 disable distributed
		job.DistributedTask.Enable = 0
	} else {
		// if assignedWorker > 1 disable distributed
		job.DistributedTask.Enable = 1
		job.DistributedTask.WorkerNumber = assignedWorker
	}

	// generate log file name
	taskClassID := string(taskName) + "-" + fmt.Sprintf("%d", classID)

	masterIns := master.RunMaster(masterAddr, &job, workerType, taskName, taskClassID)
	masterIns.Logger.Printf("[JobManager]: Assign port %d to master, run tasks=%s\n", masterPort, taskName)
	masterIns.Logger.Printf("[JobManager] begin ManageJobLifeCycle, masterAddr=%s, taskName=%s \n", masterAddr, taskClassID)
	masterIns.Logger.Println("[JobManager] call master.RunMaster with job")

	// update job's master addr
	if workerType == common.TrainWorker {
		client.JobUpdateMaster(common.CoordAddr, masterAddr, job.JobId)
	} else if workerType == common.InferenceWorker {
		client.InferenceUpdateMaster(common.CoordAddr, masterAddr, job.JobId)
	}

	// partyServer reply to jobManager-master after deploying resources.
	var requiredResource = make([][]byte, len(job.PartyAddrList))
	// default use only 1 worker in centralized way.
	// in distributed training/inference, each party will launch multiple workers, and default use only 1 ps
	// master will call party server's endpoint to launch worker, to train or predict
	for partyIndex, partyAddr := range job.PartyAddrList {

		dataPath := job.PartyInfoList[partyIndex].PartyPaths.DataInput
		dataOutput := job.PartyInfoList[partyIndex].PartyPaths.DataOutput
		modelPath := job.PartyInfoList[partyIndex].PartyPaths.ModelPath

		masterIns.Logger.Printf("[JobManager] master call party-server %s to spawn workers \n", partyAddr)

		// call part server to launch worker to execute the tasks
		rep := client.RunWorker(partyAddr,
			masterAddr, workerType,
			fmt.Sprintf("%d", job.JobId),
			dataPath, modelPath, dataOutput,
			assignedWorker, int(job.PartyNums),
			taskClassID, job.JobFlType,
		)

		reply := new(comms_pattern.RunWorkerReply)
		err := json.Unmarshal(rep, reply)
		if err != nil {
			panic(err)
		}

		requiredResource[partyIndex] = reply.EncodedStr
	}

	// extract necessary information
	masterIns.JobNetCfg.Constructor(requiredResource, masterIns.PartyNums, masterIns.Logger)
	masterIns.WorkerNum = assignedWorker * int(masterIns.PartyNums)

	masterIns.Logger.Printf("[JobManager] master received all partyServer's reply, expected workerNum = %d\n", masterIns.WorkerNum)
	masterIns.BeginCountingWorkers.Broadcast()

	masterIns.Logger.Println("[JobManager] master wait here until stage finish")
	// wait this job finish
	masterIns.WaitJobComplete()

	masterIns.Logger.Println("[JobManager] master finish this stage")

	return masterIns.IsSuccessful()
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
