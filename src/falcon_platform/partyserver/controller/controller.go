package controller

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager"
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

// party Run workers to do tasks, partyServer can deploy works in 3 ways according to user's setting,
// namely subprocess, docker, or k8s
func RunWorker(masterAddr, workerType,
	jobId,
	dataPath, modelPath, dataOutput string,
	enableDistributedTrain int,
	workerPreGroup, partyNum, workerGroupNum int) *common.LaunchResourceReply {

	reply := new(common.LaunchResourceReply)
	reply.ResourceNum = workerGroupNum * workerPreGroup
	reply.ResourceNumPreGroup = workerPreGroup
	reply.GroupNum = workerGroupNum
	reply.PartyID = common.PartyID
	reply.ResourceSVCs = make(map[common.WorkerIdType]*common.ResourceSVC)

	var totalWorkerNum common.WorkerIdType = common.WorkerIdType(workerGroupNum * workerPreGroup)

	logger.Log.Println(
		"[PartyServer]: PartyServer setup workers, workerGroupNum = ", workerGroupNum,
		" workerPreGroup = ", workerPreGroup, "totalWorkers=", totalWorkerNum)

	// centralized way
	if enableDistributedTrain == 0 {
		logger.Log.Println("[PartyServer]: PartyServer setup one worker for centralized training")

		// in centralized way, to avoid schedule worker to fixed server every time, use jobId
		jobIdInt, _ := strconv.Atoi(jobId)
		workerId := common.WorkerIdType(jobIdInt)

		nodeID := schedule(workerId)
		nodeIP := common.PartyServerClusterIPs[nodeID]

		// init resourceSVC information
		resourceSVC := new(common.ResourceSVC)
		resourceSVC.WorkerId = workerId
		resourceSVC.ResourceIP = nodeIP
		resourceSVC.GroupId = common.DefaultWorkerGroupID
		resourceSVC.WorkerPort = resourcemanager.GetFreePort(1)[0]
		resourceSVC.ExecutorExecutorPort = resourcemanager.GetFreePort(partyNum)
		resourceSVC.MpcMpcPort = resourcemanager.GetFreePort(1)[0]
		resourceSVC.MpcExecutorPort = resourcemanager.GetMpcExecutorPort(int(resourceSVC.WorkerId))
		resourceSVC.ExecutorPSPort = 0
		resourceSVC.PsExecutorPorts = []common.PortType{}
		//resourceSVC.PsPsPorts = []common.PortType{}
		resourceSVC.DistributedRole = common.CentralizedWorker
		reply.ResourceSVCs[workerId] = resourceSVC

		if common.Deployment == common.LocalThread {
			// in thread model, dont support cross machine deployment. all worker share the same ip address as partyServer
			resourceSVC.ResourceIP = common.PartyServerIP
			jobmanager.DeployWorkerThread(masterAddr, workerType, jobId, dataPath, modelPath,
				dataOutput, resourceSVC, common.CentralizedWorker)

		} else if common.Deployment == common.Docker {
			nodeLabel := common.PartyServerClusterLabels[nodeID]
			jobmanager.DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath,
				dataOutput, resourceSVC, common.CentralizedWorker, nodeLabel)

		} else if common.Deployment == common.K8S {
			jobmanager.DeployWorkerK8s(masterAddr, workerType, jobId, dataPath, modelPath,
				dataOutput, workerId, common.CentralizedWorker)

		} else {
			logger.Log.Printf("[PartyServer]: Deployment %s not supported\n", common.Deployment)
		}
	}

	// in distributed way, currently, only support using docker, others could be added later
	if enableDistributedTrain == 1 {

		var workerId common.WorkerIdType = 0
		var groupId common.GroupIdType = 0

		// for each worker group, deploy one ps and many workers
		for groupId < common.GroupIdType(workerGroupNum) {

			logger.Log.Println("[PartyServer]: PartyServer deploy worker and ps for group", groupId, "...")

			// 1 worker is for serving parameter server

			logger.Log.Println("[PartyServer]: PartyServer setup one worker as parameter " +
				"server to conduct distributed training")
			nodeID := schedule(workerId)
			nodeIP := common.PartyServerClusterIPs[nodeID]
			// init resourceSVC information
			resourceSVC := new(common.ResourceSVC)
			resourceSVC.WorkerId = workerId
			resourceSVC.GroupId = groupId
			resourceSVC.ResourceIP = nodeIP
			resourceSVC.WorkerPort = resourcemanager.GetFreePort(1)[0]
			resourceSVC.ExecutorExecutorPort = resourcemanager.GetFreePort(partyNum)
			resourceSVC.MpcMpcPort = resourcemanager.GetFreePort(1)[0]
			resourceSVC.MpcExecutorPort = resourcemanager.GetMpcExecutorPort(0)
			resourceSVC.ExecutorPSPort = 0
			resourceSVC.PsExecutorPorts = resourcemanager.GetFreePort(workerPreGroup - 1)
			resourceSVC.DistributedRole = common.DistributedParameterServer
			reply.ResourceSVCs[workerId] = resourceSVC
			if common.Deployment == common.Docker {
				nodeLabel := common.PartyServerClusterLabels[nodeID]
				jobmanager.DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath,
					dataOutput, resourceSVC, common.DistributedParameterServer, nodeLabel)

			} else {
				if common.IsDebug == common.DebugOn {
					resourceSVC.ResourceIP = common.PartyServerIP
					jobmanager.DeployWorkerThread(masterAddr, workerType, jobId, dataPath, modelPath,
						dataOutput, resourceSVC, common.DistributedParameterServer)

				} else {
					logger.Log.Printf("[PartyServer]: Deployment %s not supported in distributed training\n", common.Deployment)
				}
			}
			workerId++

			// other workers are for serving train worker
			for ii := 1; ii < workerPreGroup; ii++ {

				logger.Log.Printf("[PartyServer]: PartyServer setup one worker %d as train worker "+
					"to conduct distributed training\n", workerId)

				nodeID := schedule(workerId)
				nodeIP := common.PartyServerClusterIPs[nodeID]

				// init resourceSVC information
				resourceSVC := new(common.ResourceSVC)
				resourceSVC.WorkerId = workerId
				resourceSVC.GroupId = groupId
				resourceSVC.ResourceIP = nodeIP
				resourceSVC.WorkerPort = resourcemanager.GetFreePort(1)[0]
				resourceSVC.ExecutorExecutorPort = resourcemanager.GetFreePort(partyNum)
				resourceSVC.MpcMpcPort = resourcemanager.GetFreePort(1)[0]
				resourceSVC.MpcExecutorPort = resourcemanager.GetMpcExecutorPort(int(resourceSVC.WorkerId))
				resourceSVC.ExecutorPSPort = resourcemanager.GetFreePort(1)[0]
				resourceSVC.PsExecutorPorts = []common.PortType{}
				resourceSVC.DistributedRole = common.DistributedWorker
				reply.ResourceSVCs[workerId] = resourceSVC

				logger.Log.Printf("[PartyServer]: PartyServer setup one worker as train worker with workerID=%d to "+
					"conduct distributed training\n", workerId)

				if common.Deployment == common.Docker {
					nodeLabel := common.PartyServerClusterLabels[nodeID]
					jobmanager.DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath,
						dataOutput, resourceSVC, common.DistributedWorker, nodeLabel)

				} else {

					if common.IsDebug == common.DebugOn {
						resourceSVC.ResourceIP = common.PartyServerIP
						jobmanager.DeployWorkerThread(masterAddr, workerType, jobId, dataPath, modelPath,
							dataOutput, resourceSVC, common.DistributedWorker)

					} else {
						logger.Log.Printf("[PartyServer]: Deployment %s not supported in distributed training\n",
							common.Deployment)
					}
				}

				workerId++
			}
			groupId++
		}
	}

	return reply
}

func printAllocatedResources(reply *common.LaunchResourceReply) {
	logger.Log.Printf("[PartyServer]: Deployment %s not supported in distributed training\n", common.Deployment)

}

func CleanWorker(masterAddr, workerType string) {}
