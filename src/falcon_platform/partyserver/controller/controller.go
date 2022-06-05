package controller

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager"
	"falcon_platform/jobmanager/comms_pattern"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"strconv"
)

/**
 * @Description partyServer schedule worker to one node of cluster using hash partition method
 * @Date 下午2:44 24/08/21
 * @Param
 * @return
 **/
func schedule(workerId common.WorkerIdType) (nodeId int) {
	var clusterSize = len(common.PartyServerClusterIPs)
	nodeId = int(workerId) % clusterSize
	logger.Log.Printf("[PartyServer]: PartyServer Schedule worker %d to node %d \n", workerId, nodeId)
	return nodeId
}

// RunWorker party Run workers to do tasks, partyServer can deploy works in 3 ways according to user's setting,
// namely subprocess, docker, or k8s
func RunWorker(masterAddr, workerType,
	jobId,
	dataPath, modelPath, dataOutput string, // some vars used in FLengine
	workerNum, // how many worker to spawn
	partyNum int, //  how many party in this execution
	TaskClassIDName string, // used to generate MPC port
	jobNetCfg comms_pattern.PartyNetworkConfig,
) *comms_pattern.PartyRunWorkerReply {

	reply := new(comms_pattern.PartyRunWorkerReply)
	reply.PartyID = common.PartyID
	reply.ResourceNum = workerNum
	reply.ResourceSVCs = make(map[common.WorkerIdType]*comms_pattern.ResourceSVC)

	logger.Log.Println(
		"[PartyServer]: PartyServer setup workers,  = ",
		" workerNum = ", workerNum, "TaskClassIDName=", TaskClassIDName)

	// centralized way
	if workerNum <= 2 {
		logger.Log.Println("[PartyServer]: PartyServer setup one worker for centralized training")

		// in centralized way, to avoid schedule worker to fixed server every time, use jobId
		jobIdInt, _ := strconv.Atoi(jobId)
		workerId := common.WorkerIdType(jobIdInt)

		nodeID := schedule(workerId)
		nodeIP := common.PartyServerClusterIPs[nodeID]

		// init resourceSVC information
		resourceSVC := new(comms_pattern.ResourceSVC)
		resourceSVC.WorkerId = workerId
		resourceSVC.ResourceIP = nodeIP
		resourceSVC.WorkerPort = resourcemanager.GetOneFreePort()

		resourceSVC.JobNetCfg = jobNetCfg.Constructor(partyNum, workerId, TaskClassIDName, common.CentralizedWorker, workerNum)

		reply.ResourceSVCs[workerId] = resourceSVC

		if common.Deployment == common.LocalThread {
			// in thread model, don't support cross machine deployment. all worker share the same ip address as partyServer
			resourceSVC.ResourceIP = common.PartyServerIP
			jobmanager.DeployWorkerThread(masterAddr, workerType, jobId, dataPath, modelPath,
				dataOutput, resourceSVC)

		} else if common.Deployment == common.Docker {
			nodeLabel := common.PartyServerClusterLabels[nodeID]
			jobmanager.DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath,
				dataOutput, resourceSVC, nodeLabel, TaskClassIDName)

		} else if common.Deployment == common.K8S {
			jobmanager.DeployWorkerK8s(masterAddr, workerType, jobId, dataPath, modelPath,
				dataOutput, workerId)

		} else {
			logger.Log.Printf("[PartyServer]: Deployment %s not supported\n", common.Deployment)
		}
	}

	// in distributed way, currently, only support using docker, others could be added later
	if workerNum > 2 {

		// ps is always at workerID = 0
		var workerId common.WorkerIdType = 0

		// for each party, deploy one ps and many workers
		logger.Log.Println("[PartyServer]: PartyServer deploy worker and ps for group ...")

		// 1 worker is for serving parameter server
		logger.Log.Println("[PartyServer]: PartyServer setup one worker as parameter " +
			"server to conduct distributed training")
		nodeID := schedule(workerId)
		nodeIP := common.PartyServerClusterIPs[nodeID]
		// init resourceSVC information
		resourceSVC := new(comms_pattern.ResourceSVC)
		resourceSVC.WorkerId = workerId
		resourceSVC.ResourceIP = nodeIP
		resourceSVC.WorkerPort = resourcemanager.GetOneFreePort()

		resourceSVC.JobNetCfg = jobNetCfg.Constructor(partyNum, workerId, TaskClassIDName, common.DistributedParameterServer, workerNum)

		reply.ResourceSVCs[workerId] = resourceSVC
		if common.Deployment == common.Docker {
			nodeLabel := common.PartyServerClusterLabels[nodeID]
			jobmanager.DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath,
				dataOutput, resourceSVC, nodeLabel, TaskClassIDName)

		} else {
			if common.IsDebug == common.DebugOn {
				resourceSVC.ResourceIP = common.PartyServerIP
				jobmanager.DeployWorkerThread(masterAddr, workerType, jobId, dataPath, modelPath,
					dataOutput, resourceSVC)

			} else {
				logger.Log.Printf("[PartyServer]: Deployment %s not supported in distributed training\n", common.Deployment)
			}
		}
		workerId++

		// other workers are for serving train worker
		for ii := 1; ii < workerNum; ii++ {

			logger.Log.Printf("[PartyServer]: PartyServer setup one worker %d as train worker "+
				"to conduct distributed training\n", workerId)

			nodeID := schedule(workerId)
			nodeIP := common.PartyServerClusterIPs[nodeID]

			// init resourceSVC information
			resourceSVC := new(comms_pattern.ResourceSVC)
			resourceSVC.WorkerId = workerId
			resourceSVC.ResourceIP = nodeIP
			resourceSVC.WorkerPort = resourcemanager.GetOneFreePort()

			resourceSVC.JobNetCfg = jobNetCfg.Constructor(partyNum, workerId, TaskClassIDName, common.DistributedWorker, workerNum)

			reply.ResourceSVCs[workerId] = resourceSVC

			logger.Log.Printf("[PartyServer]: PartyServer setup one worker as train worker with workerID=%d to "+
				"conduct distributed training\n", workerId)

			if common.Deployment == common.Docker {
				nodeLabel := common.PartyServerClusterLabels[nodeID]
				jobmanager.DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath,
					dataOutput, resourceSVC, nodeLabel, TaskClassIDName)

			} else {

				if common.IsDebug == common.DebugOn {
					resourceSVC.ResourceIP = common.PartyServerIP
					jobmanager.DeployWorkerThread(masterAddr, workerType, jobId, dataPath, modelPath,
						dataOutput, resourceSVC)

				} else {
					logger.Log.Printf("[PartyServer]: Deployment %s not supported in distributed training\n",
						common.Deployment)
				}
			}
			workerId++
		}
	}
	return reply
}

func printAllocatedResources(reply *comms_pattern.PartyRunWorkerReply) {
	logger.Log.Printf("[PartyServer]: Deployment %s not supported in distributed training\n", common.Deployment)

}

func CleanWorker(masterAddr, workerType string) {}
