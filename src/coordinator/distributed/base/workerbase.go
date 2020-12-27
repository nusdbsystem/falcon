package base

import (
	"context"
	"coordinator/client"
	"coordinator/common"
	_ "coordinator/common"
	"coordinator/distributed/entity"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"fmt"
	"time"
)

type Worker interface {
	// run worker rpc server
	Run()
	//  DoTask rpc call
	DoTask(arg []byte, rep *entity.DoTaskReply) error
}

type WorkerBase struct {
	RpcBaseClass

	Pm         *taskmanager.SubProcessManager
	TaskFinish chan bool

	latestHeardTime int64
	SuicideTimeout  int

	// each worker has only one master addr
	MasterAddr string
}

func (w *WorkerBase) RunWorker(worker Worker) {

	worker.Run()

	logger.Do.Printf("%s: runWorker exit", w.Name)

}

func (w *WorkerBase) InitWorkerBase(workerAddr, name string) {

	w.InitRpcBase(workerAddr)
	w.Name = name
	w.SuicideTimeout = common.WorkerTimeout

	// the lock needs to pass to multi funcs, must create a instance
	w.Pm = taskmanager.InitSubProcessManager()
	w.TaskFinish = make(chan bool)

	// subprocess manager hold a global context
	w.Pm.Ctx, w.Pm.Cancel = context.WithCancel(context.Background())
	w.reset()
}

// call the master's register method,
func (w *WorkerBase) Register(master string) {
	args := new(entity.RegisterArgs)
	args.WorkerAddr = w.Addr

	logger.Do.Printf("WorkerBase: begin to call Master.Register to register addr= %s \n", args.WorkerAddr)
	ok := client.Call(master, w.Proxy, "Master.Register", args, new(struct{}))
	// if not register successfully, close
	if ok == false {
		logger.Do.Fatalf("WorkerBase: Register RPC %s, register error\n", master)
	}
}

// Shutdown is called by the master when all work has been completed.
// We should respond with the number of tasks we have processed.
func (w *WorkerBase) Shutdown(_, _ *struct{}) error {
	logger.Do.Printf("%s: Shutdown %s\n", w.Name, w.Addr)

	// shutdown other related thread
	w.Cancel()

	// check if there are running subprocess
	w.Pm.Lock()
	if w.Pm.NumProc > 0 {
		// if there is still running job, kill the job
		w.Pm.IsKilled = true
		w.Pm.Unlock()

		// do the kill
		w.Pm.Cancel()

		logger.Do.Printf("%s: Waiting to finish the killing %d process of DoTask...\n", w.Name, w.Pm.NumProc)

		// wait until kill successfully
		<-w.TaskFinish

	} else {
		w.Pm.Unlock()
	}

	logger.Do.Printf("%s: DoTask returned, Close the party server...\n", w.Name)

	err := w.Listener.Close() // close the connection, cause error, and then ,break the WorkerBase
	if err != nil {
		logger.Do.Printf("%s: close error, %s \n", w.Name, err)
	}
	// this is used to define shut down the WorkerBase servers

	return nil
}

func (w *WorkerBase) EventLoop() {
	time.Sleep(time.Second * 5)

loop:
	for {
		select {
		case <-w.Ctx.Done():
			logger.Do.Printf("%s: server %s quit eventLoop \n", w.Name, w.Addr)
			break loop
		default:
			elapseTime := time.Now().UnixNano() - w.latestHeardTime
			if int(elapseTime/int64(time.Millisecond)) >= w.SuicideTimeout {

				logger.Do.Printf("%s: Timeout, server %s begin to suicide \n", w.Name, w.Addr)

				var reply entity.ShutdownReply
				ok := client.Call(w.Addr, w.Proxy, w.Name+".Shutdown", new(struct{}), &reply)
				if ok == false {
					logger.Do.Printf("%s: RPC %s shutdown error\n", w.Name, w.Addr)
				} else {
					logger.Do.Printf("%s: WorkerBase timeout, RPC %s shutdown successfule\n", w.Name, w.Addr)
				}
				// quit event loop no matter ok or not
				break
			}

			time.Sleep(time.Millisecond * common.WorkerTimeout / 5)
			fmt.Printf("%s: CountDown:....... %d \n", w.Name, int(elapseTime/int64(time.Millisecond)))
		}
	}
}

func (w *WorkerBase) ResetTime(_ *struct{}, _ *struct{}) error {
	/**
	 * @Author
	 * @Description which will be called by master
	 * @Date 11:44 上午 14/12/20
	 * @Param
	 * @return
	 **/
	fmt.Printf("%s: reset the countdown\n", w.Name)
	w.reset()
	return nil
}

func (w *WorkerBase) reset() {
	w.Lock()
	w.latestHeardTime = time.Now().UnixNano()
	w.Unlock()
}
