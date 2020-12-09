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

	logger.Do.Printf("Worker: %s task started \n", wk.Address)

	var dta *entitiy.DoTaskArgs = entitiy.DecodeDoTaskArgs(arg)

	rep.Errs = make(map[string]string)
	rep.ErrLogs = make(map[string]string)
	rep.OutLogs = make(map[string]string)

	// execute task 1: data processing

	logger.Do.Println("Worker:task 1 pre processing start")
	dir := dta.PartyPath.DataInput
	stdIn := ""
	command := "python3"
	//args := []string{dta.TaskInfos.PreProcessing.AlgorithmName, "-a=1", "-b=2"}
	args := []string{"./preprocessing.py", "-a=1", "-b=2"}
	envs := []string{}

	// 2 thread will ready from isStop channel, only one is running at the any time

	killed, e, el, ol := wk.pm.ExecuteSubProc(dir, stdIn, command, args, envs)
	rep.Killed = killed
	if killed {
		wk.taskFinish <- true
		return nil
	}

	rep.Errs[common.PreProcessing] = e
	rep.ErrLogs[common.PreProcessing] = el
	rep.OutLogs[common.PreProcessing] = ol
	logger.Do.Println("Worker:task 1 pre processing done", killed, e, el, ol)

	if e != common.SubProcessNormal {
		// return res is used to control the rpc call status, always return nil, but
		// keep error at rep.Errs
		return nil
	}

	//logger.Do.Println("----------------------------------------")
	//logger.Do.Printf("Errors is %s\n", el)
	//logger.Do.Println("----------------------------------------")
	//logger.Do.Printf("Output is %s\n", ol)
	//logger.Do.Println("----------------------------------------")

	// execute task 2: train
	logger.Do.Println("Worker:task model training start")
	dir = dta.PartyPath.Model
	stdIn = ""
	command = "python3"
	//args = []string{dta.TaskInfos.ModelTraining.AlgorithmName}
	envs = []string{}

	killed, e, el, ol = wk.pm.ExecuteSubProc(dir, stdIn, command, args, envs)

	rep.Killed = killed
	if killed {
		wk.taskFinish <- true
		return nil
	}

	rep.Errs[common.ModelTraining] = e
	rep.ErrLogs[common.ModelTraining] = el
	rep.OutLogs[common.ModelTraining] = ol

	logger.Do.Println("Worker:task 2 train worker done", killed)

	if e != common.SubProcessNormal {
		return nil
	}

	for i := 10; i > 0; i-- {
		logger.Do.Println("Worker: Counting down before job done... ", i)

		time.Sleep(time.Second)
	}

	logger.Do.Printf("Worker: %s: task done\n", wk.Address)

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
