package jobmanager

import (
	"falcon_platform/common"
	"falcon_platform/logger"
)

// RunJobManager called by coordinator to run JobManager.
// The JobManager includes one master and many workers for each tasks， workerType is train or inference worker
// it firstly deploys and run master, master will call partyServer to deploy and run worker
func RunJobManager(job *common.TrainJob, workerType string) {

	if common.Deployment == common.LocalThread {
		logger.Log.Printf("[JobManager]: Deploy master with thread")
		deployJobManagerThread(job, workerType)

	} else if common.Deployment == common.Docker {
		logger.Log.Printf("[JobManager]: Deploy master with docker")
		deployJobManagerDocker(job, workerType)

	} else if common.Deployment == common.K8S {
		logger.Log.Printf("[JobManager]: Deploy master with k8s ")
		deployJobManagerK8s(job, workerType)
	}
}
