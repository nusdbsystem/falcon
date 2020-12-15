package base

import (
	"context"
	"coordinator/client"
	"coordinator/common"
	_ "coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"fmt"
	"time"
)

type WorkerBase struct {
	RpcBaseClass

	Pm         		*taskmanager.SubProcessManager
	TaskFinish 		chan bool

	latestHeardTime int64
	SuicideTimeout  int

	// each worker has only one master address
	MasterAddr  	string
}

func(wk *WorkerBase) InitWorkerBase(workerAddress, name string) {

	wk.InitRpcBase(workerAddress)
	wk.Name = name
	wk.SuicideTimeout = common.WorkerTimeout

	// the lock needs to pass to multi funcs, must create a instance
	wk.Pm = taskmanager.InitSubProcessManager()
	wk.TaskFinish = make(chan bool)

	// subprocess manager hold a global context
	wk.Pm.Ctx, wk.Pm.Cancel = context.WithCancel(context.Background())
	wk.reset()
}


// call the master's register method,
func (wk *WorkerBase) Register(master string) {
	args := new(entitiy.RegisterArgs)
	args.WorkerAddr = wk.Address

	logger.Do.Printf("WorkerBase: begin to call Master.Register to register address= %s \n", args.WorkerAddr)
	ok := client.Call(master, wk.Proxy, "Master.Register", args, new(struct{}))
	// if not register successfully, close
	if ok == false {
		logger.Do.Fatalf("WorkerBase: Register RPC %s, register error\n", master)
	}
}

// Shutdown is called by the master when all work has been completed.
// We should respond with the number of tasks we have processed.
func (wk *WorkerBase) Shutdown(_, _ *struct{}) error {
	logger.Do.Println("WorkerBase: Shutdown", wk.Address)

	// shutdown other related thread
	wk.Cancel()

	// check if there are running subprocess
	wk.Pm.Lock()
	if wk.Pm.NumProc > 0 {
		// if there is still running job, kill the job
		wk.Pm.IsKilled = true
		wk.Pm.Unlock()

		// do the kill
		wk.Pm.Cancel()

		logger.Do.Println("WorkerBase: Waiting to finish the killing process of DoTask...", wk.Pm.NumProc)

		// wait until kill successfully
		<-wk.TaskFinish

	} else {
		wk.Pm.Unlock()
	}

	logger.Do.Println("WorkerBase: DoTask returned, Close the partyserver...")

	err := wk.Listener.Close() // close the connection, cause error, and then ,break the WorkerBase
	if err != nil {
		logger.Do.Println("WorkerBase: WorkerBase close error, ", err)
	}
	// this is used to define shut down the WorkerBase servers

	return nil
}

func (wk *WorkerBase) EventLoop() {
	time.Sleep(time.Second * 5)

loop:
	for {
		select {
		case <-wk.Ctx.Done():
			logger.Do.Printf("WorkerBase: server %s quite eventLoop \n", wk.Address)
			break loop
		default:
			elapseTime := time.Now().UnixNano() - wk.latestHeardTime
			if int(elapseTime/int64(time.Millisecond)) >= wk.SuicideTimeout {

				logger.Do.Printf("WorkerBase: Timeout, server %s begin to suicide \n", wk.Address)

				var reply entitiy.ShutdownReply
				ok := client.Call(wk.Address, wk.Proxy, wk.Name+".Shutdown", new(struct{}), &reply)
				if ok == false {
					logger.Do.Printf("WorkerBase: RPC %s shutdown error\n", wk.Address)
				} else {
					logger.Do.Printf("WorkerBase: WorkerBase timeout, RPC %s shutdown successfule\n", wk.Address)
				}
				// quite event loop no matter ok or not
				break
			}

			time.Sleep(time.Millisecond * common.WorkerTimeout / 5)
			fmt.Printf("WorkerBase: CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))
		}
	}
}

func (wk *WorkerBase) ResetTime(_ *struct{}, _ *struct{}) error {
	/**
	 * @Author
	 * @Description which will be called by master
	 * @Date 11:44 上午 14/12/20
	 * @Param
	 * @return
	 **/
	fmt.Println("WorkerBase: reset the countdown")
	wk.reset()
	return nil
}

func (wk *WorkerBase) reset() {
	wk.Lock()
	wk.latestHeardTime = time.Now().UnixNano()
	wk.Unlock()
}
