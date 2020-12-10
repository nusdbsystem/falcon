package controller

import (
	dist "coordinator/distributed"
)

func SetupWorker(masterAddress, taskType string) {

	dist.SetupWorkerHelper(masterAddress, taskType)

}

func CleanWorker(masterAddress, taskType string) {

	dist.CleanWorker()

}

