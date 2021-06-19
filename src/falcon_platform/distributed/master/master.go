package master

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/distributed/base"
	"falcon_platform/distributed/entity"
	"falcon_platform/logger"
	"strconv"
	"strings"
	"sync"
)

type WorkerInfo struct {
	Addr string
	ID   uint
	IP   string
}

func Contains(str string, l []WorkerInfo) bool {
	for _, ls := range l {
		if ls.Addr+":"+strconv.Itoa(int(ls.ID)) == str {
			return true
		}
	}
	return false
}

type Master struct {
	base.RpcBaseClass

	// cond
	beginCountDown *sync.Cond
	allWorkerReady *sync.Cond

	// tmp slice to store registered workers
	tmpWorkers chan string

	// slice to store valid workers
	workers   []WorkerInfo
	workerNum int

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

	// check if at least one worker found, decide when to board cast heartbeat
	foundWorker bool

	// common.TrainWorker or common.InferenceWorker
	workerType string
}

func newMaster(masterAddr string, workerNum int) (ms *Master) {
	ms = new(Master)
	ms.InitRpcBase(masterAddr)
	ms.Name = common.Master
	ms.beginCountDown = sync.NewCond(ms)
	ms.allWorkerReady = sync.NewCond(ms)

	ms.tmpWorkers = make(chan string)
	ms.workerNum = workerNum

	//default to successful, update at runtime, record to db after job finishing
	ms.jobStatus = common.JobSuccessful
	ms.runtimeStatus = make(chan *entity.DoTaskReply)

	ms.heartbeatTimeout = common.MasterTimeout
	return
}

// RegisterWorker is an RPC method that is called by workers after they have started
// up to report that they are ready to receive tasks.
func (master *Master) RegisterWorker(args *entity.RegisterArgs, _ *struct{}) error {

	logger.Log.Println("[master/RegisterWorker] one Worker registered!")
	// Pass WorkerAddrId (addr:partyID) into tmpWorkers for pre-processing
	master.tmpWorkers <- args.WorkerAddrId
	return nil
}

// sends information of worker to ch. which is used by scheduler
func (master *Master) forwardRegistrations(dslOjb *cache.DslObj) {

	logger.Log.Printf("Master: start forwardRegistrations... ")
	var requiredIP []string

	for i := 0; i < len(dslOjb.PartyAddrList); i++ {
		IP := strings.Split(dslOjb.PartyAddrList[i], ":")[0]
		requiredIP = append(requiredIP, IP)
	}

loop:
	for {
		select {
		case <-master.Ctx.Done():

			logger.Log.Println("Master: Thread-2 forwardRegistrations: exit")
			break loop

		// a list of master.workers:
		// eg: [127.0.0.1:30009:0 127.0.0.1:30010:1 127.0.0.1:30011:2]
		case tmpWorker := <-master.tmpWorkers:
			// 1. check if this work already exist
			master.Lock()
			if Contains(tmpWorker, master.workers) {
				logger.Log.Printf("Master: the worker %s already registered, skip \n", tmpWorker)
			}
			master.Unlock()
			// 2. check if this worker is needed
			// worker is IP:Port:PartyID
			workerTmpList := strings.Split(tmpWorker, ":")
			tmpUrl := strings.Join(workerTmpList[:2], ":")
			tmpIP := workerTmpList[0]
			tmpID, _ := strconv.Atoi(workerTmpList[2])

			for i, IP := range requiredIP {
				if tmpIP == IP {
					logger.Log.Println("Master: Found one worker", tmpWorker)

					master.Lock()
					master.workers = append(master.workers, WorkerInfo{Addr: tmpUrl, ID: uint(tmpID), IP: tmpIP})
					master.Unlock()
					master.beginCountDown.Broadcast()

					// remove the i-th IP
					requiredIP = append(requiredIP[0:i+1], requiredIP[i+1:]...)
					break
				}
			}

			master.Lock()
			if len(master.workers) == master.workerNum {
				// it is not strictly necessary for you to hold the lock on M sync.Mutex when calling C.Broadcast()
				master.allWorkerReady.Broadcast()
			}
			master.Unlock()
		}
	}
}

func (master *Master) run(
	dispatcher func(),
	updateStatus func(jsonString string),
	finish func(),
) {
	// master's main logic,
	// 1. schedule the job to workers,
	// 2. collect the job status, call coordinator's endpoints to update status to db
	// 3. close all resources related to this job

	dispatcher()
	logger.Log.Printf("Master: Finish job, update to coord, jobStatus: <%s> \n", master.jobStatus)

	updateStatus(master.jobStatusLog)
	logger.Log.Println("Master: Finish job, begin to shutdown master")

	finish()
	logger.Log.Printf("Master %s: Thread-4 master.finish, job completed\n", master.Addr)

}

func (master *Master) Wait() {
	// master wait until all master's logic finished

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

	logger.Log.Println("Master: Shutdown server")
	// causes the Accept to fail, then break out the accept loop
	_ = master.Listener.Close()
	return nil
}

// called by coordinator, to shutdown the running job
func (master *Master) KillJob(_, _ *struct{}) error {
	master.killWorkers()
	master.jobStatusLock.Lock()
	master.jobStatus = common.JobKilled
	master.jobStatusLock.Unlock()
	return nil
}

func (master *Master) killWorkers() {
	// after called killWorker, scheduler in master.run will finish, "jobUpdateStatus" and "finish" will be called
	master.Lock()
	defer master.Unlock()

	for len(master.workers) > 0 {
		worker := master.workers[0]
		master.StopRPCServer(worker.Addr, master.workerType+".Shutdown")
		master.workers = master.workers[1:]
	}
}
