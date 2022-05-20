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

func InitInferenceWorker(masterAddr, workerAddr string, PartyID common.PartyIdType, WorkerID common.WorkerIdType, DistributedRole uint) *InferenceWorker {
	wk := InferenceWorker{}
	wk.InitWorkerBase(workerAddr, common.InferenceWorker)
	wk.MasterAddr = masterAddr
	wk.PartyID = PartyID

	return &wk
}

func (wk *InferenceWorker) Run() {

	go func() {
		defer logger.HandleErrors()
		wk.HeartBeatLoop()
	}()

	rpcSvc := rpc.NewServer()

	err := rpcSvc.Register(&wk)
	if err != nil {
		logger.Log.Fatalf("[%s]: start Error \n", wk.Name)
	}

	logger.Log.Printf("[%s] from PartyID %d: register to masterAddr = %s \n", wk.Name, wk.PartyID, wk.MasterAddr)
	wk.RegisterToMaster(wk.MasterAddr)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)
}

func (wk *InferenceWorker) DoTask(taskName string, rep *entity.DoTaskReply) error {

	// 1. decode args

	wk.CreateInference(taskName)
	return nil
}

func (wk *InferenceWorker) CreateInference(_ string) {
	// todo gobuild.sh sub process to run prediction job

	logger.Log.Println("InferenceWorker: CreateService")

	//cmd := exec.Command("python3", "/go/preprocessing.py", "-a=1", "-b=2")

	// 2 thread will ready from isStop channel, only one is running at the any time

	//wk.Tm.CreateResources(cmd)

}

func (wk *InferenceWorker) UpdateInference() error {
	return nil
}

func (wk *InferenceWorker) QueryInference() error {
	return nil
}
