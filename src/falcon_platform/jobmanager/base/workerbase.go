package base

import (
	"falcon_platform/client"
	"falcon_platform/common"
	_ "falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"time"
)

type Worker interface {
	// run worker rpc server
	Run()
	// DoTask rpc call
	DoTask(arg string, rep *entity.DoTaskReply) error
	// receive job information
	ReceiveJobInfo(arg string, rep *entity.DoTaskReply) error
}

// worker server will do 2 things:
// 1. call c++ code to train /predict
// 2. response to master's request to either report heartbeat or shutdown the training subprocess
type WorkerBase struct {
	RpcBaseClass

	// resource manager to manage resource of tasks like preprocess, model training,
	// init at beginning of each doTask call, delete when dotask finish
	Tm *resourcemanager.ResourceManager

	// global mpcTM, manage multi mpc, record global mpc status in multi mpcs
	MpcTm *resourcemanager.ResourceManager

	// used for heartbeat
	latestHeardTime int64
	SuicideTimeout  int

	// each worker has only one master addr
	MasterAddr string

	// each worker is linked to the PartyID
	PartyID string

	// each worker store full job information,
	DslObj entity.DslObj4SingleParty
}

func (w *WorkerBase) RunWorker(worker Worker) {
	logger.Log.Println("[WorkerBase]: RunWorker called")

	worker.Run()

	logger.Log.Printf("%s: RunWorker exit", w.Name)

}

func (w *WorkerBase) InitWorkerBase(workerAddr, name string) {

	// work addr Addr is the worker ip+port
	// name is the worker name
	// manager: each worker have a task manager, which can be either docker, or native subprocess

	w.InitRpcBase(workerAddr)
	w.Name = name
	w.SuicideTimeout = common.WorkerTimeout
	w.MpcTm = resourcemanager.InitResourceManager()
	w.reset()
}

func (w *WorkerBase) ReceiveJobInfo(arg string, rep *entity.DoTaskReply) error {
	// receive job information, called by master
	dslObj, err := entity.DecodeDslObj4SingleParty([]byte(arg))
	if err != nil {
		rep.RuntimeError = true
		rep.TaskMsg.RuntimeMsg = err.Error()
	} else {
		w.DslObj = *dslObj
	}
	return nil
}

func (w *WorkerBase) Register(MasterAddr string, PartyID string) {
	// call the master's register method to tell master i'm(worker) ready,
	args := new(entity.RegisterArgs)
	args.WorkerAddr = w.Addr
	args.PartyID = PartyID
	args.WorkerAddrId = w.Addr + ":" + PartyID // IP:Port:PartyID

	logger.Log.Printf("[WorkerBase]: call Master.RegisterWorker to register WorkerAddr: %s \n", args.WorkerAddr)
	ok := client.Call(MasterAddr, w.Network, "Master.RegisterWorker", args, new(struct{}))
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
	w.MpcTm.ResourceClear()

	// shutdown worker's hear-beat
	w.Clear()
	if w.Tm == nil {
		logger.Log.Println("[WorkerBase] Worker.ResourceManager must be init")
	} else {
		w.Tm.DeleteResources()
	}

	err := w.Listener.Close() // close the connection, cause error, and then, break the WorkerBase
	if err != nil {
		logger.Log.Printf("[WorkerBase] %s: close error, %s \n", w.Name, err)
	}
	return nil
}

func (w *WorkerBase) HeartBeatLoop() {
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

			//fmt.Printf("%s: CountDown:....... %d \n", w.Name, int(elapseTime/int64(time.Millisecond)))
		}
	}
}

func (w *WorkerBase) ResetTime(_ *struct{}, _ *struct{}) error {

	// called by master to reset worker base time
	//logger.Log.Println("[WorkerBase]: ResetTime called")
	w.reset()
	return nil
}

func (w *WorkerBase) reset() {
	logger.Log.Println("[WorkerBase]: reset called")
	w.Lock()
	w.latestHeardTime = time.Now().UnixNano()
	w.Unlock()
}
