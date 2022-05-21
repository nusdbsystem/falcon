package worker

import (
	"falcon_platform/client"
	"falcon_platform/common"
	_ "falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/jobmanager/rpcbase"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"time"
)

// WorkerBase worker server will do 2 things:
// 1. call c++ code to train /predict
// 2. response to master's request to either report heartbeat or shutdown the training subprocess
type WorkerBase struct {
	base.RpcBaseClass

	// resource manager to manage resource of tasks like preprocess, model training,
	// init at beginning of each doTask call, delete when dotask finish
	Tm *resourcemanager.ResourceManager

	// global mpcTM, manage multi mpc, record global mpc status in multi MPCs
	MpcTm *resourcemanager.ResourceManager

	// used for heartbeat
	latestHeardTime int64
	SuicideTimeout  int

	// each worker has only one master addr
	MasterAddr string

	// each worker has onw ID
	WorkerID common.WorkerIdType

	// each worker is linked to the PartyID
	PartyID common.PartyIdType
}

// InitWorkerBase Addr is the worker ip+port
// name is the worker name
// manager: each worker have a tasks manager, which can be either docker, or native subprocess
func (w *WorkerBase) InitWorkerBase(workerAddr, name string) {

	w.InitRpcBase(workerAddr)
	w.Name = name
	w.SuicideTimeout = common.WorkerTimeout
	w.MpcTm = resourcemanager.InitResourceManager()
	w.reset()
}

// RegisterToMaster call the master's register method to tell master I'm(worker) ready,
func (w *WorkerBase) RegisterToMaster(MasterAddr string) {
	worker := new(entity.WorkerInfo)
	worker.Addr = w.Addr
	worker.PartyID = w.PartyID
	worker.WorkerID = w.WorkerID

	logger.Log.Printf("[WorkerBase]: call Master.RegisterWorker to register WorkerAddr: %s \n", worker.Addr)
	ok := client.Call(MasterAddr, w.Network, "Master.RegisterWorker", worker, new(struct{}))
	// if not register successfully, close
	if ok == false {
		logger.Log.Fatalf("[WorkerBase]: Register RPC %s, register error\n", MasterAddr)
	}
}

func (w *WorkerBase) Shutdown(_, _ *struct{}) error {
	// Shutdown is called by the master when all work has been completed.
	//todo respond with the number of tasks we have processed later.?

	logger.Log.Printf("[WorkerBase]: Shutdown called, %s: Shutdown %s\n", w.Name, w.Addr)

	// when worker is about exit, shutdown mpc if any.
	w.MpcTm.DeleteResources()

	// shutdown worker's hear-beat
	w.Clear()
	if w.Tm != nil {
		w.Tm.DeleteResources()
	}

	err := w.Listener.Close() // close the connection, cause error, and then, break the WorkerBase
	if err != nil {
		logger.Log.Printf("[WorkerBase] %s: close error, %s \n", w.Name, err)
	}
	return nil
}

// HeartBeatLoop loop
func (w *WorkerBase) HeartBeatLoop() {

	// wait for 5 second before counting down
	time.Sleep(time.Second * 5)

	//define a label and break to that label
loop:
	for {
		select {
		case <-w.Ctx.Done():
			logger.Log.Printf("[%s]: server %s quit HeartBeat eventLoop \n", w.Name, w.Addr)
			break loop
		default:
			elapseTime := time.Now().UnixNano() - w.latestHeardTime
			if int(elapseTime/int64(time.Millisecond)) >= w.SuicideTimeout {

				logger.Log.Printf("[%s]: Timeout, server %s suicide \n", w.Name, w.Addr)

				var reply entity.ShutdownReply
				ok := client.Call(w.Addr, w.Network, w.Name+".Shutdown", new(struct{}), &reply)
				if ok == false {
					logger.Log.Printf("[%s]: RPC %s shutdown error\n", w.Name, w.Addr)
				} else {
					logger.Log.Printf("[%s]: WorkerBase timeout, RPC %s shutdown successfully\n", w.Name, w.Addr)
				}
				// quit event loop no matter ok or not
				break
			}

			time.Sleep(time.Millisecond * common.WorkerTimeout / 5)
		}
	}
}

// ResetTime called by master to reset worker rpcbase time
func (w *WorkerBase) ResetTime(_ *struct{}, _ *struct{}) error {
	w.reset()
	return nil
}

func (w *WorkerBase) reset() {
	logger.Log.Println("[WorkerBase]: reset called")
	w.Lock()
	w.latestHeardTime = time.Now().UnixNano()
	w.Unlock()
}
