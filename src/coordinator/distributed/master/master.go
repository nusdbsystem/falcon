package master

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/distributed/base"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/distributed/utils"
	"coordinator/logger"
	"strings"
	"sync"
	"time"
)

type Master struct {

	base.RpcBaseClass

	doneChannel chan bool

	beginCountDown *sync.Cond
	allWorkerReady *sync.Cond

	// tmp slice to store registered workers
	tmpWorkers chan string
	// slice to store valid workers
	workers   []string
	workerNum int

	jobStatus uint

	lastSendTime     int64
	heartbeatTimeout int

	foundWorker bool

}

func newMaster(masterAddr string, workerNum int) (ms *Master) {
	ms = new(Master)
	ms.InitRpcBase(masterAddr)
	ms.Name = common.Master
	ms.beginCountDown = sync.NewCond(ms)
	ms.allWorkerReady = sync.NewCond(ms)

	ms.doneChannel = make(chan bool)
	ms.tmpWorkers = make(chan string)
	ms.workerNum = workerNum
	ms.heartbeatTimeout = common.MasterTimeout
	return
}

// Register is an RPC method that is called by workers after they have started
// up to report that they are ready to receive tasks.
func (this *Master) Register(args *entitiy.RegisterArgs, _ *struct{}) error {

	/**
	 * @Author
	 * @Description
			retry 3 times, until put to the tmpWorkers, if failed, it means there is
			no consumer, when the server will be closed, manually call cancel
			client wait 3 seconds max
	 * @Date 9:37 上午 14/12/20
	 * @Param
	 * @return
	 **/
	this.Lock()
	defer this.Unlock()

	NTimes := 3
	for {
		if NTimes<0{
			return nil
		}
		select {
		case this.tmpWorkers <- args.WorkerAddr:
			return nil
		default:
			logger.Do.Println("Master: Register worker, no consumer,retry...", args.WorkerAddr)
			time.Sleep(time.Second*1)
			NTimes--
		}
	}
}

// sends information of worker to ch. which is used by scheduler
func (this *Master) forwardRegistrations(qItem *cache.QItem) {

	logger.Do.Printf("Master: start forwardRegistrations... ")
	var requiredIp []string

	for i := 0; i < len(qItem.IPs); i++ {
		ip := strings.Split(qItem.IPs[i], ":")[0]
		requiredIp = append(requiredIp, ip)
	}

loop:
	for {
		select {
		case <- this.Ctx.Done():
			logger.Do.Printf("Master: %s quite forwardRegistrations \n", this.Port)
			break loop

		case addr := <- this.tmpWorkers:
			// 1. check if this work already exist
			if utils.Contains(addr, this.workers){
				logger.Do.Printf("Master: the worker %s already registered, skip \n", addr)
			}

			// 2. check if this worker is needed
			addrIp := strings.Split(addr, ":")[0]

			for i, ip := range requiredIp{
				if addrIp == ip{
					logger.Do.Println("Master: Found one worker", addr)

					this.Lock()
					this.workers  = append(this.workers, addr)
					this.Unlock()
					this.beginCountDown.Broadcast()

					// remove the i th ip
					requiredIp = append(requiredIp[0:i+1], requiredIp[i+1:]...)
					break
				}
			}

			this.Lock()
			if len(this.workers) == this.workerNum {
				// it is not strictly necessary for you to hold the lock on M sync.Mutex when calling C.Broadcast()
				this.allWorkerReady.Broadcast()
			}
			this.Unlock()
		}
	}
}

func (this *Master) run(schedule func(), finish func()) {

	schedule()
	logger.Do.Println("Master: finish job, begin to close all")
	finish()

	logger.Do.Printf("Master %s: job completed\n", this.Address)

	this.doneChannel <- true
}


func (this *Master) Wait() {
	<-this.doneChannel

	if common.Env==common.ProdEnv{
		km := taskmanager.InitK8sManager(true,  "")
		km.DeleteService(common.ExecutorCurrentName)
	}
}

// Shutdown is an RPC method that shuts down the Master's RPC server.
// for rpc method, must be public method, only 2 params, second one must be pointer,return err type
func (this *Master) Shutdown(_, _ *struct{}) error {
	logger.Do.Println("Master: Shutdown server")
	_ = this.Listener.Close() // causes the Accept to fail, then break out the accetp loop
	return nil
}


func (this *Master) killWorkers() {

	this.Lock()
	defer this.Unlock()

	for _, worker := range this.workers {

		logger.Do.Println("Master: begin to call Worker.Shutdown")

		this.StopRPCServer(worker,"Worker.Shutdown")
	}
}

func (this *Master) KillJob(_, _ *struct{}) error {
	/**
	 * @Author
	 * @Description called by coordinator, to shutdown the running job
	 * @Date 9:47 上午 14/12/20
	 * @Param
	 * @return
	 **/
	this.killWorkers()

	this.Cancel()
	this.StopRPCServer(this.Address,"Master.Shutdown")
	return nil
}