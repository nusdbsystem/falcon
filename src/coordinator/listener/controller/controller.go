package controller

import (
	dist "coordinator/distributed"
)

func SetupWorker(httpHost, masterAddress, taskType string) {

	dist.SetupWorkerHelper(httpHost, masterAddress, taskType)

}
