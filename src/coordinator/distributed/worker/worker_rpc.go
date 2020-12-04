package worker

import (
	"coordinator/common"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"net/rpc"
)

func RunWorker(masterAddress, workerProxy, workerHost, workerPort string) {

	workerAddress := workerHost + ":" + workerPort

	wk := new(Worker)
	wk.InitRpc(workerProxy, workerAddress)
	wk.Name = common.Worker
	wk.SuicideTimeout = common.WorkerTimeout

	// the lock needs to pass to multi funcs, must create a instance
	wk.pm = taskmanager.InitSubProcessManager()
	wk.taskFinish = make(chan bool)

	wk.reset()
	go wk.eventLoop()


	rpcSvc := rpc.NewServer()
	err := rpcSvc.Register(wk)
	if err!= nil{
		logger.Do.Printf("%s: start Error \n", wk.Name)
		return
	}

	logger.Do.Println("Worker: register to masterAddress= ", masterAddress)
	wk.register(masterAddress)

	wk.StartRPCServer(rpcSvc, true)

	logger.Do.Println("Worker: ", workerAddress, "runWorker exit")
}
