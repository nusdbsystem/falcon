package worker

import (
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
)

type Worker interface {
	// Run worker rpc server
	Run()
	// DoTask rpc call
	DoTask(arg string, rep *entity.DoTaskReply) error
}

// RunWorker run train or inference worker.
func RunWorker(worker Worker) {
	logger.Log.Println("[WorkerBase]: RunWorker called")
	worker.Run()
}
