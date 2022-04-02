package jobmanager

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/jobmanager/worker"
	"falcon_platform/logger"
)

/**
 * @Description run master in a local thread, master will call partyServer to set up worker
 * @Date 下午6:12 20/08/21
 * @Param workerType: trainWorker or inferenceWorker
 * @return
 **/
func DeployMasterThread(dslOjb *cache.DslObj, workerType string) {

	// call party server to launch workers
	ManageJobLifeCycle(dslOjb, workerType)
}

/**
 * @Description: this func is only called by partyServer, run worker in local thread
 * @Date 2:14 下午 1/12/20
 * @Param
 	masterAddr： IP of the masterAddr addr
	workerType： train or inference worker
	jobId： jobId
	dataPath： data folder path of this party
	modelPath： the path to save trained model
	dataOutput： path to store processed data
	workerID： id of the worker, mainly used to distinguish workers in distributed training
	distributedRole： 0: ps, 1: worker
 **/
func DeployWorkerThread(masterAddr, workerType, jobId,
	dataPath, modelPath, dataOutput string,
	resourceSVCs *common.ResourceSVC, distributedRole uint) {

	workerAddr := resourceSVCs.ToAddr(resourceSVCs.WorkerPort)

	var serviceName string

	// in dev, use thread

	common.TaskDataPath = dataPath
	common.TaskDataOutput = dataOutput
	common.TaskModelPath = modelPath

	// one parameter server and multiple workers
	if workerType == common.TrainWorker {

		serviceName = "job-" + jobId + "-train"
		common.TaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName
		wk := worker.InitTrainWorker(masterAddr, workerAddr,
			common.PartyID, resourceSVCs.WorkerId, resourceSVCs.GroupId,
			distributedRole)

		go func() {
			defer logger.HandleErrors()
			wk.RunWorker(wk)
		}()

	} else if workerType == common.InferenceWorker {

		serviceName = "job-" + jobId + "-inference"
		common.TaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName
		wk := worker.InitInferenceWorker(masterAddr, workerAddr, common.PartyID, resourceSVCs.WorkerId, distributedRole)
		go func() {
			defer logger.HandleErrors()
			wk.RunWorker(wk)
		}()
	}
}
