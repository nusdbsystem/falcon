package controller

import (
	dist "falcon_platform/distributed"
)

func SetupWorker(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string) {

	// todo add dynamic scheduler when running in cluster, to scheduler the pod the specific server where the data are stored.
	// 	 schedule according to dataPath

	dist.SetupWorkerHelper(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput)

}

func CleanWorker(masterAddr, workerType string) {
}
