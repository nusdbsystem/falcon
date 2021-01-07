package worker

import (
	"coordinator/common"
	"coordinator/distributed/base"
	"coordinator/distributed/entity"
	"coordinator/logger"
	"net/rpc"
)

type InferenceWorker struct {
	base.WorkerBase
}

func InitInferenceWorker(masterAddr, workerAddr string, PartyID string) *InferenceWorker {
	wk := InferenceWorker{}
	wk.InitWorkerBase(workerAddr, common.InferenceWorker)
	wk.MasterAddr = masterAddr
	wk.PartyID = PartyID

	return &wk
}

func (wk *InferenceWorker) Run() {

	// 0 thread: start event Loop
	go wk.EventLoop()

	rpcSvc := rpc.NewServer()

	err := rpcSvc.Register(&wk)
	if err != nil {
		logger.Do.Fatalf("%s: start Error \n", wk.Name)
	}

	logger.Do.Printf("%s from PartyID %s: register to masterAddr = %s \n", wk.Name, wk.PartyID, wk.MasterAddr)
	wk.Register(wk.MasterAddr, wk.PartyID)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)
}

func (wk *InferenceWorker) DoTask(arg []byte, rep *entity.DoTaskReply) error {

	var doTaskArgs *entity.DoTaskArgs = entity.DecodeDoTaskArgs(arg)

	go wk.CreateInference(doTaskArgs)
	return nil
}
