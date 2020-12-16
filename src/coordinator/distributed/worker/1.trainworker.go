package worker

import (
	"coordinator/common"
	"coordinator/distributed/base"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"net/rpc"
)

type TrainWorker struct {
	base.WorkerBase
}

func InitTrainWorker (masterUrl, workerUrl string) *TrainWorker{

	wk := TrainWorker{}
	wk.InitWorkerBase(workerUrl, common.TrainWorker)
	wk.MasterUrl = masterUrl

	return &wk
}


func (wk *TrainWorker) Run(){

	// 0 thread: start event Loop
	go wk.EventLoop()

	rpcSvc := rpc.NewServer()

	err := rpcSvc.Register(wk)
	if err!= nil{
		logger.Do.Fatalf("%s: start Error \n", wk.Name)
	}

	logger.Do.Printf("%s: register to masterUrl = %s \n", wk.Name, wk.MasterUrl)
	wk.Register(wk.MasterUrl)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)

	// once  worker is killed, clear the resources.
	if common.Env==common.ProdEnv{
		km := taskmanager.InitK8sManager(true,  "")
		km.DeleteService(common.WorkerK8sSvcName)
	}

}


func (wk *TrainWorker) DoTask (arg []byte, rep *entitiy.DoTaskReply) error {

	var dta *entitiy.DoTaskArgs = entitiy.DecodeDoTaskArgs(arg)

	TestTaskProcess(dta)
	//wk.TrainTask(dta, rep)
	return nil
}