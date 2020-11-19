package worker

import (
	"coordinator/config"
	_ "coordinator/config"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/distributed/utils"
	"fmt"
	"net"
	"sync"
	"time"
)

type Worker struct {

	sync.Mutex

	Proxy string
	name  string //  which is the ip+port address of worker

	l net.Listener

	pm         *taskmanager.SubProcessManager
	taskFinish chan bool

	latestHeardTime	int64
	SuicideTimeout	int

	// if the master is stopped
	isStop 			bool
}

func (wk *Worker) DoTask(arg []byte, rep *entitiy.DoTaskReply) error {

	fmt.Printf("Worker: %s task started \n", wk.name)

	var dta *entitiy.DoTaskArgs = entitiy.DecodeDoTaskArgs(arg)

	rep.Errs = make(map[string]string)
	rep.ErrLogs = make(map[string]string)
	rep.OutLogs = make(map[string]string)

	// execute task 1: data processing

	fmt.Println("Worker:task 1 pre processing start")
	dir := dta.PartyPath.DataInput
	stdIn := ""
	command := "/Users/nailixing/.virtualenvs/test_pip/bin/python"
	args := []string{dta.TaskInfos.PreProcessing.AlgorithmName, "-a=1", "-b=2"}
	envs := []string{}

	// 2 thread will ready from isStop channel, only one is running at the any time

	killed, e, el, ol := wk.pm.ExecuteSubProc(dir, stdIn, command, args, envs)
	rep.Killed = killed
	if killed {
		wk.taskFinish <- true
		return nil
	}

	rep.Errs[config.PreProcessing] = e
	rep.ErrLogs[config.PreProcessing] = el
	rep.OutLogs[config.PreProcessing] = ol
	fmt.Println("Worker:task 1 pre processing done", killed, e, el, ol)

	if e != config.SubProcessNormal {
		// return res is used to control the rpc call status, always return nil, but
		// keep error at rep.Errs
		return nil
	}

	//fmt.Println("----------------------------------------")
	//fmt.Printf("Errors is %s\n", el)
	//fmt.Println("----------------------------------------")
	//fmt.Printf("Output is %s\n", ol)
	//fmt.Println("----------------------------------------")

	// execute task 2: train
	fmt.Println("Worker:task model training start")
	dir = dta.PartyPath.Model
	stdIn = ""
	command = "/Users/nailixing/.virtualenvs/test_pip/bin/python"
	args = []string{dta.TaskInfos.ModelTraining.AlgorithmName}
	envs = []string{}

	killed, e, el, ol = wk.pm.ExecuteSubProc(dir, stdIn, command, args, envs)

	rep.Killed = killed
	if killed {
		wk.taskFinish <- true
		return nil
	}

	rep.Errs[config.ModelTraining] = e
	rep.ErrLogs[config.ModelTraining] = el
	rep.OutLogs[config.ModelTraining] = ol

	fmt.Println("Worker:task 2 train worker done", killed)

	if e != config.SubProcessNormal {
		return nil
	}

	for i := 10; i > 0; i-- {
		fmt.Println("Worker: Counting down before job done... ", i)

		time.Sleep(time.Second)
	}

	fmt.Printf("Worker: %s: task done\n", wk.name)

	return nil
}

// call the master's register method,
func (wk *Worker) register(master string) {
	args := new(entitiy.RegisterArgs)
	args.WorkerAddr = wk.name

	fmt.Printf("Worker: begin to call Master.Register to register address= %s \n", args.WorkerAddr)
	ok := utils.Call(master, wk.Proxy, "Master.Register", args, new(struct{}))
	if ok == false {
		fmt.Printf("Worker: Register RPC %s, register error\n", master)
	}
}

// Shutdown is called by the master when all work has been completed.
// We should respond with the number of tasks we have processed.
func (wk *Worker) Shutdown(_ *struct{}, res *entitiy.ShutdownReply) error {
	fmt.Println("Worker: Shutdown", wk.name)

	// there are running subprocess
	wk.pm.Lock()

	// shutdown other related thread
	wk.isStop = true

	if wk.pm.NumProc > 0 {
		wk.pm.Unlock()

		wk.pm.IsStop <- true

		fmt.Println("Worker: Waiting to finish DoTask...", wk.pm.NumProc)
		<-wk.taskFinish
		fmt.Println("Worker: DoTask returned, Close the listener...")
	} else {
		wk.pm.Unlock()

	}

	err := wk.l.Close() // close the connection, cause error, and then ,break the worker
	if err != nil {
		fmt.Println("Worker: worker close error, ", err)
	}
	// this is used to define shut down the worker servers

	return nil
}


func (wk *Worker) eventLoop() {
	time.Sleep(time.Second*5)
	for {

		wk.Lock()
		if wk.isStop == true{
			fmt.Printf("Worker: isStop=true, server %s quite eventLoop \n", wk.name)

			wk.Unlock()
			break
		}

		elapseTime := time.Now().UnixNano() - wk.latestHeardTime
		if int(elapseTime/int64(time.Millisecond)) >= wk.SuicideTimeout{

			fmt.Printf("Worker: Timeout, server %s begin to suicide \n", wk.name)

			var reply entitiy.ShutdownReply
			ok := utils.Call(wk.name, wk.Proxy, "Worker.Shutdown", new(struct{}), &reply)
			if ok == false {
				fmt.Printf("Worker: RPC %s shutdown error\n", wk.name)
			}else{
				fmt.Printf("Worker: worker timeout, RPC %s shutdown successfule\n", wk.name)
			}
			// quite event loop no matter ok or not
			break
		}
		wk.Unlock()

		time.Sleep(time.Millisecond*config.WorkerTimeout/5)
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
