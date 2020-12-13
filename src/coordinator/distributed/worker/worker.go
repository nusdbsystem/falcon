package worker

import (
	"coordinator/client"
	"coordinator/common"
	_ "coordinator/common"
	"coordinator/distributed/base"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"fmt"
	"time"
)

type Worker struct {

	base.RpcBase

	pm         *taskmanager.SubProcessManager
	taskFinish chan bool

	latestHeardTime int64
	SuicideTimeout  int

}

func (wk *Worker) DoTask(arg []byte, rep *entitiy.DoTaskReply) error {

	wk.MlTaskProcess(arg, rep)
	//go TestTaskProcess()
	return nil

}

// call the master's register method,
func (wk *Worker) register(master string) {
	args := new(entitiy.RegisterArgs)
	args.WorkerAddr = wk.Address

	logger.Do.Printf("Worker: begin to call Master.Register to register address= %s \n", args.WorkerAddr)
	ok := client.Call(master, wk.Proxy, "Master.Register", args, new(struct{}))
	if ok == false {
		logger.Do.Printf("Worker: Register RPC %s, register error\n", master)
	}
}

// Shutdown is called by the master when all work has been completed.
// We should respond with the number of tasks we have processed.
func (wk *Worker) Shutdown(_ *struct{}, res *entitiy.ShutdownReply) error {
	logger.Do.Println("Worker: Shutdown", wk.Address)

	// there are running subprocess
	wk.pm.Lock()

	// shutdown other related thread
	wk.IsStop = true

	if wk.pm.NumProc > 0 {
		wk.pm.Unlock()

		wk.pm.IsStop <- true

		logger.Do.Println("Worker: Waiting to finish DoTask...", wk.pm.NumProc)
		<-wk.taskFinish
		logger.Do.Println("Worker: DoTask returned, Close the listener...")
	} else {
		wk.pm.Unlock()

	}

	err := wk.Listener.Close() // close the connection, cause error, and then ,break the worker
	if err != nil {
		logger.Do.Println("Worker: worker close error, ", err)
	}
	// this is used to define shut down the worker servers

	return nil
}

func (wk *Worker) eventLoop() {
	time.Sleep(time.Second * 5)
	for {

		wk.Lock()
		if wk.IsStop == true {
			logger.Do.Printf("Worker: isStop=true, server %s quite eventLoop \n", wk.Address)

			wk.Unlock()
			break
		}

		elapseTime := time.Now().UnixNano() - wk.latestHeardTime
		if int(elapseTime/int64(time.Millisecond)) >= wk.SuicideTimeout {

			logger.Do.Printf("Worker: Timeout, server %s begin to suicide \n", wk.Address)

			var reply entitiy.ShutdownReply
			ok := client.Call(wk.Address, wk.Proxy, "Worker.Shutdown", new(struct{}), &reply)
			if ok == false {
				logger.Do.Printf("Worker: RPC %s shutdown error\n", wk.Address)
			} else {
				logger.Do.Printf("Worker: worker timeout, RPC %s shutdown successfule\n", wk.Address)
			}
			// quite event loop no matter ok or not
			break
		}
		wk.Unlock()

		time.Sleep(time.Millisecond * common.WorkerTimeout / 5)
		fmt.Printf("Worker: CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))
	}
}

func (wk *Worker) ResetTime(_ *struct{}, _ *struct{}) error {
	fmt.Println("Worker: reset the countdown")
	wk.reset()
	return nil
}

func (wk *Worker) reset() {
	wk.Lock()
	wk.latestHeardTime = time.Now().UnixNano()
	wk.Unlock()

}
