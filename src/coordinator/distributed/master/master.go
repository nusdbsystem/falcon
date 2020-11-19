package master

import (
	"coordinator/config"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/utils"
	"fmt"
	"log"
	"net"
	"reflect"
	"sort"
	"strings"
	"sync"
	"time"
)

type Master struct {
	sync.Mutex

	Proxy       string
	Address     string
	doneChannel chan bool

	newCond        *sync.Cond
	beginCountDown *sync.Cond
	allWorkerReady *sync.Cond

	workers   []string
	workerNum int

	l     net.Listener
	stats []int

	lastSendTime     int64
	heartbeatTimeout int

	foundWorker bool

	// if the master is stopped
	isStop bool
}

func newMaster(Proxy, masterAddr string, workerNum int) (ms *Master) {
	ms = new(Master)
	ms.Address = masterAddr
	ms.Proxy = Proxy

	ms.newCond = sync.NewCond(ms)
	ms.beginCountDown = sync.NewCond(ms)
	ms.allWorkerReady = sync.NewCond(ms)

	ms.doneChannel = make(chan bool)
	ms.workerNum = workerNum
	ms.heartbeatTimeout = config.MasterTimeout
	ms.isStop = false
	return
}

func (this *Master) Test(args string, reply *string) error {
	log.Println(args)
	return nil
}

// Register is an RPC method that is called by workers after they have started
// up to report that they are ready to receive tasks.
func (this *Master) Register(args *entitiy.RegisterArgs, _ *struct{}) error {
	this.Lock()
	defer this.Unlock()
	log.Println("Master: Register: worker", args.WorkerAddr)

	if len(this.workers) <= this.workerNum {
		this.workers = append(this.workers, args.WorkerAddr)

		// tell forwardRegistrations() that there's a new workers[] entry.
		this.newCond.Broadcast()
		this.beginCountDown.Broadcast()
	} else {
		log.Println("Master: Register Already got enough worker")
	}
	return nil
}

// sends information of worker to ch. which is used by scheduler
func (this *Master) forwardRegistrations(ch chan string, qItem *config.QItem) {
	indexWorker := 0
	for {
		this.Lock()
		// if current worker number == already checked worker, and also ==  total worker number needed
		if len(this.workers) == indexWorker && len(this.workers) == this.workerNum {
			// if got enough worker, break out loop and tell scheduler to schedule

			// verify if ip match
			var reIps []string
			var qIps = qItem.IPs

			for i := 0; i < len(qIps); i++ {
				addr := this.workers[i]
				ip := strings.Split(addr, ":")[0]
				reIps = append(reIps, ip)
			}

			// compare if 2 slice are the same
			var c = make([]string, len(reIps))
			var d = make([]string, len(qIps))

			copy(c, reIps)
			copy(d, qIps)

			sort.Strings(c)
			sort.Strings(d)

			// if registered ip and job ip not match

			if ok := reflect.DeepEqual(c, d); !ok {
				log.Println("Scheduler: Ip are not match")
				this.Unlock()
				return
			}

			this.allWorkerReady.Broadcast()

			this.Unlock()
			break
		}
		if len(this.workers) > indexWorker {
			log.Println("Master: Found one worker")
			// there's a worker that we haven't told schedule() about.
			w := this.workers[indexWorker]
			go func() {
				ch <- w
			}()
			indexWorker += 1
			log.Println("Master: worker index is ", indexWorker)
		} else {
			// wait for Register() to add an entry to workers[]
			// in response to an RPC from a new worker.
			this.newCond.Wait()
		}
		this.Unlock()
	}
}

func (this *Master) run(schedule func(), finish func()) {

	schedule()
	log.Println("Master: finish job, begin to close all")
	finish()

	log.Printf("Master %s: job completed\n", this.Address)

	this.doneChannel <- true
}

func (this *Master) killWorkers() []int {
	this.Lock()
	defer this.Unlock()

	nStat := make([]int, 0, len(this.workers))

	for _, worker := range this.workers {

		var reply entitiy.ShutdownReply

		log.Println("Master: begin to call Worker.Shutdown")

		ok := utils.Call(worker, this.Proxy, "Worker.Shutdown", new(struct{}), &reply)
		if ok == false {
			log.Printf("Master: RPC %s shutdown error\n", worker)
		} else {
			// save the res of each call
			nStat = append(nStat, reply.Ntasks)
		}
	}
	return nStat
}

func (this *Master) KillJob(_, _ *struct{}) error {
	this.stats = this.killWorkers()
	this.stopRPCServer()
	return nil
}

func (this *Master) Wait() {
	<-this.doneChannel
}

func (this *Master) CheckWorker() bool {

	this.Lock()

	if len(this.workers) > 0 {
		this.foundWorker = true
	} else {
		this.foundWorker = false
	}

	this.Unlock()
	return this.foundWorker
}

func (this *Master) eventLoop() {

	for {

		this.Lock()
		if this.isStop == true {
			log.Printf("Master: isStop=true, server %s quite eventLoop \n", this.Address)
			this.Unlock()
			break
		}
		this.Unlock()

		if !this.CheckWorker() {

			this.Lock()
			this.beginCountDown.Wait()
			this.Unlock()

		} else {

			this.Lock()
			elapseTime := time.Now().UnixNano() - this.lastSendTime
			fmt.Printf("Master: CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))

			if int(elapseTime/int64(time.Millisecond)) >= this.heartbeatTimeout {

				this.Unlock()
				fmt.Println("Master: Timeout, server begin to send hb to worker")
				this.broadcastHeartbeat()

			} else {
				this.Unlock()
			}
		}

		time.Sleep(time.Millisecond * config.MasterTimeout / 5)
	}
}

func (this *Master) broadcastHeartbeat() {
	// update lastSendTime
	this.reset()
	for _, worker := range this.workers {

		ok := utils.Call(worker, this.Proxy, "Worker.ResetTime", new(struct{}), new(struct{}))
		if ok == false {
			log.Printf("Master: RPC %s send heartbeat error\n", worker)
		}
	}
}

func (this *Master) reset() {
	this.Lock()
	this.lastSendTime = time.Now().UnixNano()
	this.Unlock()

}
