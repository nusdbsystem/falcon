package resourcemanager

import (
	"context"
	"falcon_platform/common"
	"falcon_platform/logger"
	"io/ioutil"
	"os/exec"
	"sync"
	"syscall"
	"time"
)

type SubProcessManager struct{}

func InitSubProcessManager() *SubProcessManager {
	subpMngr := new(SubProcessManager)
	return subpMngr
}

func (mngr *SubProcessManager) ResourceMonitor(
	resource interface{},
	isClearCtx context.Context,
	isRealeaseCtx context.Context,
) {
	// the ctx is shared by all threads created by worker,
	// once cancel, all threads stopped
	pid := resource.(int)
loop:
	for {
		select {
		case <-isRealeaseCtx.Done():
			logger.Log.Println("[subProcessMngr]: resources released")
			break
		case <-isClearCtx.Done():
			_, err := syscall.Getpgid(pid)
			if err == nil {
				mngr.DeleteResource(pid)
			} else {
				logger.Log.Printf("[subProcessMngr]: Clear resources, PID %d is not running, skip!\n", pid)
			}
			break loop
		default:
			time.Sleep(time.Second * 1)
		}
	}
}

func (mngr *SubProcessManager) UpdateConfig(config interface{}) {

}

/**
   * @Description
   * @Date 11:54 上午 10/12/20
   * @Param
        // Each entry is of the form "key=value".
        // If Env contains duplicate environment keys, only the last
        // value in the slice for each duplicate key is used.
        // As a special case on Windows, SYSTEMROOT is always added if
        // missing and not explicitly set to the empty string.

  	Dir specifies the working directory of the command.
    	    If Dir is the empty string, Run runs the command in the
    	    calling process's current directory.

   * @return
   **/
func (mngr *SubProcessManager) CreateResource(
	incmd interface{}, // command line to be executed
	isClearCtx context.Context, // control if clear resource or not
	isRealeaseCtx context.Context, // control if release resource or not
	mux *sync.Mutex, // lock
	TotResources *int, // store number of sub-processes
	TaskStatus *string, // store tasks status
	runTimeErrorLog *string, // store tasks error logs
) {
	var cmd *exec.Cmd = incmd.(*exec.Cmd)

	defer func() {
		//logger.Log.Println("[subProcessMngr]: lock")
		mux.Lock()
		*TotResources -= 1
		mux.Unlock()
		//logger.Log.Println("[subProcessMngr]: Unlock ")
	}()

	//1. start pip to record standard error
	stderr, err := cmd.StderrPipe()
	if err != nil {
		logger.Log.Println(err)
		mux.Lock()
		*runTimeErrorLog = err.Error()
		mux.Unlock()
		return
	}
	var out OutStream
	cmd.Stdout = out

	// 2. start with a short statement to execute before the condition
	if err := cmd.Start(); err != nil {
		logger.Log.Println("cmd.Start() Error:", err)
		mux.Lock()
		*runTimeErrorLog = err.Error()
		mux.Unlock()
		return
	}

	// 3. if start not successfully, record error to TaskStatus
	if cmd.Process == nil {
		mux.Lock()
		*TaskStatus = common.TaskFailed
		mux.Unlock()
		logger.Log.Printf("[subProcessMngr]: running %s\n, cmd.Process is Nil, start error\n", cmd.String())

		// if start successfully
	} else if cmd.Process != nil {
		// record as running
		mux.Lock()
		*TaskStatus = common.TaskRunning
		mux.Unlock()

		logger.Log.Println("[subProcessMngr]: open subProcess, PID=", cmd.Process.Pid)

		// monitor the process
		go func() {
			defer logger.HandleErrors()
			mngr.ResourceMonitor(cmd.Process.Pid, isClearCtx, isRealeaseCtx)
		}()

		// if there is a running SubProc, nTasks add 1
		mux.Lock()
		*TotResources += 1
		mux.Unlock()

		errLog, _ := ioutil.ReadAll(stderr)
		executeError := cmd.Wait()

		// this means subprocess exit with Non zero code
		if executeError != nil {
			mux.Lock()
			*TaskStatus = common.TaskFailed
			mux.Unlock()
			logger.Log.Printf("[subProcessMngr]: running cmd = %s\n subprocess exit status: << %s >> \n", cmd.String(), executeError.Error())
			//logger.Log.Printf("[subProcessMngr]: subprocess error logs: \n"+
			//	"<<<<<<<<<<\n "+
			//	"%s \n"+
			//	"<<<<<<<<<<\n",
			//	string(errLog))
			mux.Lock()
			*runTimeErrorLog = string(errLog)
			mux.Unlock()
		} else {
			// this means subprocess running successful
			mux.Lock()
			*TaskStatus = common.TaskSuccessful
			mux.Unlock()
		}
	}
}

func (mngr *SubProcessManager) DeleteResource(resource interface{}) {
	pid := resource.(int)
	err := syscall.Kill(pid, syscall.SIGQUIT)
	if err != nil {
		logger.Log.Println("[subProcessMngr]: Manually Killed PID=cmd.Process.Pid Error", err)
	} else {
		logger.Log.Println("[subProcessMngr]: Manually Killed PID=cmd.Process.Pid", pid)
	}
}
