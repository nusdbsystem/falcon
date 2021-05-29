package worker

import (
	"falcon_platform/common"
	"falcon_platform/distributed/base"
	"falcon_platform/distributed/entity"
	"falcon_platform/logger"
	"net/rpc"
	"time"
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
	logger.Log.Println("TrainWorker Run() called")
	// 0 thread: start heartbeat
	go wk.HeartBeat()

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

	for i := 10; i > 0; i-- {
		logger.Log.Println("Worker: Counting down before DoTask done... ", i)
		time.Sleep(time.Second)
	}
	logger.Log.Printf("Worker: %s: DoTask done\n", wk.Addr)
	return nil
}
