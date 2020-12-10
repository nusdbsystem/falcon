package controller

import (
	dist "coordinator/distributed"
)

func SetupWorker(masterAddress, taskType, jobId string) {

	dist.SetupWorkerHelper(masterAddress, taskType, jobId)

}

func CleanWorker(masterAddress, taskType string) {

	dist.CleanWorker()

}

