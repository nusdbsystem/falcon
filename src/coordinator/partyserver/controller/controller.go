package controller

import (
	dist "coordinator/distributed"
)

func SetupWorker(masterUrl, workerType, jobId,dataPath,modelPath,dataOutput string) {

	// todo add dynamic scheduler when running in cluster, to scheduler the pod the specific server where the data are stored.
	// 	 schedule according to dataPath

	dist.SetupWorkerHelper(masterUrl, workerType, jobId, dataPath,modelPath, dataOutput)

}

func CleanWorker(masterUrl, workerType string) {

	dist.CleanWorker()

}

