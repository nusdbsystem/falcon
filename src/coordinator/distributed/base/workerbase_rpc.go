package base

import (
	"coordinator/distributed/entitiy"
	"coordinator/logger"
)

type Worker interface {
	// run worker rpc server
	Run()
	//  DoTask rpc call
	DoTask(arg []byte, rep *entitiy.DoTaskReply) error
}

func (w *WorkerBase) RunWorker(worker Worker) {

	worker.Run()

	logger.Do.Printf("%s: runWorker exit", w.Name)

}
