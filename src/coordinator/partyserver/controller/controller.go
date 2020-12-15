package controller

import (
	dist "coordinator/distributed"
)

func SetupWorker(masterAddress, workerType, jobId,dataPath,modelPath,dataOutput string) {

	// todo add dynamic scheduler when running in cluster, to scheduler the pod the specific server where the data are stored.
	// 	 schedule according to dataPath

	dist.SetupWorkerHelper(masterAddress, workerType, jobId, dataPath,modelPath, dataOutput)

}

func CleanWorker(masterAddress, workerType string) {

	dist.CleanWorker()

}

