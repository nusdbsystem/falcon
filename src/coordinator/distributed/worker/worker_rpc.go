package worker

import (
	"context"
	"coordinator/common"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"net/rpc"
)

func RunWorker(masterAddress, workerAddress string) {

	wk := new(Worker)
	wk.InitRpcBase(workerAddress)
	wk.Name = common.Worker
	wk.SuicideTimeout = common.WorkerTimeout

	// the lock needs to pass to multi funcs, must create a instance
	wk.pm = taskmanager.InitSubProcessManager()
	wk.taskFinish = make(chan bool)

	// subprocess manager hold a global context
	wk.pm.Ctx, wk.pm.Cancel = context.WithCancel(context.Background())
	wk.reset()

	// 0 thread: start event Loop
	go wk.eventLoop()

	rpcSvc := rpc.NewServer()
	err := rpcSvc.Register(wk)
	if err!= nil{
		logger.Do.Fatalf("%s: start Error \n", wk.Name)
	}

	logger.Do.Println("Worker: register to masterAddress = ", masterAddress)
	wk.register(masterAddress)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)

	// once  worker is killed, clear the resources.
	if common.Env==common.ProdEnv{
		km := taskmanager.InitK8sManager(true,  "")
		km.DeleteService(common.ExecutorCurrentName)
	}

	logger.Do.Println("Worker: ", workerAddress, "runWorker exit")

}
