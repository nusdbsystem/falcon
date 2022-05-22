package master

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/jobmanager/comms_pattern"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/jobmanager/rpcbase"
	"log"
	"os"
	"sync"
	"time"
)

type Master struct {
	base.RpcBaseClass
	// cond
	beginCountDown       *sync.Cond
	allWorkerReady       *sync.Cond
	BeginCountingWorkers *sync.Cond

	// total party number
	PartyNums uint

	// For Worker discover
	// tmp slice to store registered workers
	workerRegisterChan chan string
	// slice to store valid workers
	workers []*entity.WorkerInfo
	// required worker number
	WorkerNum int
	// check if at least one worker found, decide when to board cast heartbeat
	foundWorker bool

	// common.TrainWorker or common.InferenceWorker
	workerType string

	// jobStatus indicates if job successful, failed, killed running, init
	jobStatus     string
	jobStatusLock sync.Mutex
	// record job status message to db
	jobStatusLog string
	// record runtime status in real time
	runtimeStatus chan *entity.DoTaskReply

	// master heartbeat setting
	lastSendTime     int64
	heartbeatTimeout int

	// logger instance
	Logger  *log.Logger
	LogFile *os.File

	// network configurations
	JobNetCfg comms_pattern.JobNetworkConfig
}

func newMaster(masterAddr string, partyNum uint, jobType string) (ms *Master) {
	ms = new(Master)
	ms.InitRpcBase(masterAddr)
	ms.Name = common.Master
	ms.beginCountDown = sync.NewCond(ms)
	ms.allWorkerReady = sync.NewCond(ms)
	ms.BeginCountingWorkers = sync.NewCond(ms)

	// default cache 100 workers at most,
	// use cache to allow workers to register before master running forwardRegistrations to consume worker's information
	ms.workerRegisterChan = make(chan string, 100)

	//default to successful, update at runtime, record to db after job finishing
	ms.jobStatus = common.JobSuccessful
	ms.runtimeStatus = make(chan *entity.DoTaskReply)

	ms.heartbeatTimeout = common.MasterTimeout
	ms.WorkerNum = 0
	ms.PartyNums = partyNum
	ms.JobNetCfg = comms_pattern.GetJobNetCfg()[jobType]
	return
}

// run master's main logic,
// 1. schedule the job to workers,
// 2. collect the job status, call coordinator's endpoints to update status to db
// 3. close all resources related to this job
func (master *Master) run(
	dispatcher func(),
	updateStatus func(jsonString string),
	finish func(),
) {

	dispatcher()
	master.Logger.Printf("[Master]: Finish job, update to coord, jobStatus: <%s> \n", master.jobStatus)

	updateStatus(master.jobStatusLog)
	master.Logger.Println("[Master]: Finish job, begin to shutdown master")

	finish()
	master.Logger.Printf("[Master] %s: Thread-4 master.finish, job completed\n", master.Addr)

}

// WaitJobComplete master wait until all master's logic finished
func (master *Master) WaitJobComplete() {

loop:
	for {
		select {
		case <-master.Ctx.Done():
			break loop
		}
	}
}

// Shutdown is an RPC method that shuts down the Master's RPC server.
// for rpc method, must be public method, only 2 params, second one must be pointer,return err type
func (master *Master) Shutdown(_, _ *struct{}) error {

	master.Logger.Println("[Master]: Shutdown server")
	// causes the Accept to fail, then break out the accept loop
	_ = master.Listener.Close()
	return nil
}

// KillJob called by coordinator, to shutdown the running job
func (master *Master) KillJob(_, _ *struct{}) error {
	master.killWorkers()
	master.jobStatusLock.Lock()
	master.jobStatus = common.JobKilled
	master.jobStatusLock.Unlock()
	return nil
}

// killWorkers after called killWorker, scheduler in master.run will finish, "jobUpdateStatus" and "finish" will be called
func (master *Master) killWorkers() {
	master.Lock()
	defer master.Unlock()

	for len(master.workers) > 0 {
		worker := master.workers[0]
		master.StopRPCServer(worker.Addr, master.workerType+".Shutdown")
		master.workers = master.workers[1:]
	}
}

func (master *Master) heartBeat() {

loop:
	for {
		select {
		case <-master.Ctx.Done():

			master.Logger.Println("[Master.Heartbeat] Thread-1 heartBeatLoop exit")
			break loop

		default:
			if !master.checkWorker() {

				// if not worker found, wait here
				master.Lock()
				master.beginCountDown.Wait()
				master.Unlock()

			} else {
				// worker found, send heartbeat
				master.Lock()
				elapseTime := time.Now().UnixNano() - master.lastSendTime

				//fmt.Printf("[Master.Heartbeat] CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))

				if int(elapseTime/int64(time.Millisecond)) >= master.heartbeatTimeout {

					master.Unlock()
					//fmt.Println("[Master.Heartbeat] Timeout, server send heart beat to worker")
					master.broadcastHeartbeat()

				} else {
					master.Unlock()
				}
			}
		}
		time.Sleep(time.Millisecond * common.MasterTimeout / 5)
	}
}

// board cast heartbeat to current workers in worker list
func (master *Master) broadcastHeartbeat() {

	// master.Logger.Println("[broadcastHeartbeat]...")
	// update lastSendTime
	master.reset()

	for _, worker := range master.workers {
		ok := client.Call(worker.Addr, master.Network, master.workerType+".ResetTime", new(struct{}), new(struct{}))
		if ok == false {
			master.Logger.Printf("[Master.Heartbeat] RPC %s send heartbeat error\n", worker.Addr)
		}
	}
}

func (master *Master) reset() {
	master.Lock()
	master.lastSendTime = time.Now().UnixNano()
	master.Unlock()

}

func (master *Master) checkWorker() bool {
	// if at least one worker found, master send hearbeat,
	// otherwise, wait until one worker found..
	master.Lock()

	if len(master.workers) > 0 {
		master.foundWorker = true
	} else {
		master.foundWorker = false
	}

	master.Unlock()
	return master.foundWorker
}
