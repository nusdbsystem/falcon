package worker

import (
	"falcon_platform/common"
	"falcon_platform/distributed/base"
	"falcon_platform/distributed/entity"
	"falcon_platform/logger"
	"net/rpc"
)

type TrainWorker struct {
	base.WorkerBase
}

func InitTrainWorker(masterAddr, workerAddr string, PartyID string) *TrainWorker {

	wk := TrainWorker{}
	wk.InitWorkerBase(workerAddr, common.TrainWorker)
	wk.MasterAddr = masterAddr
	wk.PartyID = PartyID

	return &wk
}

func (wk *TrainWorker) Run() {

	// 0 thread: start event Loop
	go wk.HeartBeatLoop()

	rpcSvc := rpc.NewServer()

	err := rpcSvc.Register(wk)
	if err != nil {
		logger.Log.Fatalf("%s: start Error \n", wk.Name)
	}

	logger.Log.Printf("%s from PartyID %s to register with masterAddr(%s)\n", wk.Name, wk.PartyID, wk.MasterAddr)
	wk.Register(wk.MasterAddr, wk.PartyID)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)

}

func (wk *TrainWorker) DoTask(arg []byte, rep *entity.DoTaskReply) error {
	logger.Log.Println("TrainWorker DoTask() called")

	var doTaskArgs *entity.DoTaskArgs = entity.DecodeDoTaskArgs(arg)

	wk.TrainTask(doTaskArgs, rep)

	logger.Log.Printf("Worker: %s: task done\n", wk.Addr)
	return nil
}
