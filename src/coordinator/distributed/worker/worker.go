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
	"sync"
	"time"
)

type Worker struct {

	base.RpcBase

	pm         		*taskmanager.SubProcessManager
	taskFinish 		chan bool

	latestHeardTime int64
	SuicideTimeout  int

}

func (wk *Worker) DoTask(arg []byte, rep *entitiy.DoTaskReply) error {

	var dta *entitiy.DoTaskArgs = entitiy.DecodeDoTaskArgs(arg)

	wg := sync.WaitGroup{}

	wg.Add(2)

	go wk.MpcTaskProcess(dta, "algName")
	go wk.MlTaskProcess(dta, rep, &wg)

	// wait until all the task done
	wg.Wait()

	// kill all the monitors
	wk.pm.Cancel()

	wk.pm.Lock()
	rep.Killed = wk.pm.IsKilled
	if wk.pm.IsKilled == true{
		wk.pm.Unlock()
		wk.taskFinish <- true
	}else{
		wk.pm.Unlock()
	}

	//go TestTaskProcess()
	return nil

}

// call the master's register method,
func (wk *Worker) register(master string) {
	args := new(entitiy.RegisterArgs)
	args.WorkerAddr = wk.Address

	logger.Do.Printf("Worker: begin to call Master.Register to register address= %s \n", args.WorkerAddr)
	ok := client.Call(master, wk.Proxy, "Master.Register", args, new(struct{}))
	// if not register successfully, close
	if ok == false {
		logger.Do.Fatalf("Worker: Register RPC %s, register error\n", master)
	}
}

// Shutdown is called by the master when all work has been completed.
// We should respond with the number of tasks we have processed.
func (wk *Worker) Shutdown(_, _ *struct{}) error {
	logger.Do.Println("Worker: Shutdown", wk.Address)

	// shutdown other related thread
	wk.Cancel()

	// check if there are running subprocess
	wk.pm.Lock()
	if wk.pm.NumProc > 0 {
		// if there is still running job, kill the job
		wk.pm.IsKilled = true
		wk.pm.Unlock()

		// do the kill
		wk.pm.Cancel()

		logger.Do.Println("Worker: Waiting to finish the killing process of DoTask...", wk.pm.NumProc)

		// wait until kill successfully
		<-wk.taskFinish

	} else {
		wk.pm.Unlock()
	}

	logger.Do.Println("Worker: DoTask returned, Close the listener...")

	err := wk.Listener.Close() // close the connection, cause error, and then ,break the worker
	if err != nil {
		logger.Do.Println("Worker: worker close error, ", err)
	}
	// this is used to define shut down the worker servers

	return nil
}

func (wk *Worker) eventLoop() {
	time.Sleep(time.Second * 5)

loop:
	for {
		select {
		case <-wk.Ctx.Done():
			logger.Do.Printf("Worker: isStop=true, server %s quite eventLoop \n", wk.Address)
			break loop
		default:
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
}

func (wk *Worker) ResetTime(_ *struct{}, _ *struct{}) error {
	/**
	 * @Author
	 * @Description which will be called by master
	 * @Date 11:44 上午 14/12/20
	 * @Param
	 * @return
	 **/
	fmt.Println("Worker: reset the countdown")
	wk.reset()
	return nil
}

func (wk *Worker) reset() {
	wk.Lock()
	wk.latestHeardTime = time.Now().UnixNano()
	wk.Unlock()
}
