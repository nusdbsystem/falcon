package worker

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/base"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
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

	go wk.HeartBeatLoop()

	rpcSvc := rpc.NewServer()

	err := rpcSvc.Register(&wk)
	if err != nil {
		logger.Log.Fatalf("%s: start Error \n", wk.Name)
	}

	logger.Log.Printf("%s from PartyID %s: register to masterAddr = %s \n", wk.Name, wk.PartyID, wk.MasterAddr)
	wk.Register(wk.MasterAddr, wk.PartyID)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)
}

func (wk *InferenceWorker) DoTask(taskName string, rep *entity.DoTaskReply) error {

	// 1. decode args

	wk.CreateInference(taskName)
	return nil
}
