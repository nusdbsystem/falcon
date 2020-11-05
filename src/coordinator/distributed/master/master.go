package master

import (
	"coordinator/distributed/entitiy"
	"coordinator/distributed/utils"
	"fmt"
	"net"
	"sync"
)

type Master struct {
	sync.Mutex

	Proxy       string
	Address     string
	doneChannel chan bool

	newCond   *sync.Cond
	workers   []string
	workerNum int

	l     net.Listener
	stats []int
}

func newMaster(Proxy, masterAddr string, workerNum int) (ms *Master) {
	ms = new(Master)
	ms.Address = masterAddr
	ms.Proxy = Proxy
	ms.newCond = sync.NewCond(ms)
	ms.doneChannel = make(chan bool)
	ms.workerNum = workerNum
	return
}

func (this *Master) Test(args string, reply *string) error {
	fmt.Println(args)
	return nil
}

// Register is an RPC method that is called by workers after they have started
// up to report that they are ready to receive tasks.
func (this *Master) Register(args *entitiy.RegisterArgs, _ *struct{}) error {
	this.Lock()
	defer this.Unlock()
	fmt.Println("Master: Register: worker", args.WorkerAddr)

	if len(this.workers) < this.workerNum {
		this.workers = append(this.workers, args.WorkerAddr)

		// tell forwardRegistrations() that there's a new workers[] entry.
		this.newCond.Broadcast()
	} else {
		fmt.Println("Master: Register Already got enough worker")
	}
	return nil
}

// sends information of worker to ch. which is used by scheduler
func (this *Master) forwardRegistrations(ch chan string) {
	indexWorker := 0
	for {
		this.Lock()
		if len(this.workers) > this.workerNum {
			// if got enough worker, break out loop
			this.Unlock()
			break
		}
		if len(this.workers) > indexWorker {
			fmt.Println("Master: Found one worker")
			// there's a worker that we haven't told schedule() about.
			w := this.workers[indexWorker]
			go func() {
				ch <- w
			}()
			indexWorker += 1
			fmt.Println("Master: worker index is ", indexWorker)
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
	fmt.Println("Master: finish job, begin to close all")
	finish()

	fmt.Printf("Master %s: job completed\n", this.Address)

	this.doneChannel <- true
}

func (this *Master) killWorkers() []int {
	this.Lock()
	defer this.Unlock()

	nStat := make([]int, 0, len(this.workers))

	for _, worker := range this.workers {

		var reply entitiy.ShutdownReply

		fmt.Println("Master: begin to call Worker.Shutdown")

		ok := utils.Call(worker, this.Proxy, "Worker.Shutdown", new(struct{}), &reply)
		if ok == false {
			fmt.Printf("Master: RPC %s shutdown error\n", worker)
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
