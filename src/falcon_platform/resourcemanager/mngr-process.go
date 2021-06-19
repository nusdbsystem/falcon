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

func ProcMonitor(pid int, Ctx context.Context) {
	// the ctx is shared by all threads created by worker,
	// once cancel, all threads stopped
loop:
	for {
		select {
		case <-Ctx.Done():

			_, err := syscall.Getpgid(pid)
			if err == nil {
				err = syscall.Kill(pid, syscall.SIGQUIT)
				if err != nil {
					logger.Log.Println("[subProcessMngr]: Manually Killed PID=cmd.Process.Pid Error", err)
				} else {
					logger.Log.Println("[subProcessMngr]: Manually Killed PID=cmd.Process.Pid", pid)
				}
			} else {
				logger.Log.Printf("[subProcessMngr]: Clear resources, PID %d is not running, skip!\n", pid)
			}
			break loop
		default:
			time.Sleep(time.Second * 1)
		}
	}
}

func CreateProc(
	cmd *exec.Cmd, // command line to be executed
	ctx context.Context, // control subprocess groups exist or not
	mux *sync.Mutex, // lock
	TotResources *int, // store number of sub-processes
	TaskStatus *string, // store task status
	runTimeErrorLog *string, // store task error logs
) {
	/**
	   * @Author
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

	defer func() {
		//logger.Log.Println("[subProcessMngr]: lock")
		mux.Lock()
		*TotResources -= 1
		mux.Unlock()
		//logger.Log.Println("[subProcessMngr]: Unlock ")
	}()

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

	// start with a short statement to execute before the condition
	if err := cmd.Start(); err != nil {
		logger.Log.Println("cmd.Start() Error:")
		logger.Log.Println(err)
		mux.Lock()
		*runTimeErrorLog = err.Error()
		mux.Unlock()
		return
	}

	// if start successfully
	if cmd.Process != nil {
		mux.Lock()
		*TaskStatus = common.TaskRunning
		mux.Unlock()

		logger.Log.Println("[subProcessMngr]: open subProcess, PID=", cmd.Process.Pid)

		go ProcMonitor(cmd.Process.Pid, ctx)
		// if there is a running SubProc, nTasks add 1
		mux.Lock()
		*TotResources += 1
		mux.Unlock()

		errLog, _ := ioutil.ReadAll(stderr)
		executeStatus := cmd.Wait()

		if executeStatus != nil {
			// this means subprocess exit with Non zero code
			mux.Lock()
			*TaskStatus = common.TaskFailed
			mux.Unlock()
			logger.Log.Printf("[subProcessMngr]: subprocess exit status: << %s >> \n", executeStatus.Error())
			logger.Log.Printf("[subProcessMngr]: subprocess error logs: \n"+
				"<<<<<<<<<<\n "+
				"%s \n"+
				"<<<<<<<<<<\n",
				string(errLog))
			mux.Lock()
			*runTimeErrorLog = string(errLog)
			mux.Unlock()
		} else {
			// this means subprocess running successful
			mux.Lock()
			*TaskStatus = common.TaskSuccessful
			mux.Unlock()
		}

	} else {
		mux.Lock()
		*TaskStatus = common.TaskFailed
		mux.Unlock()
		logger.Log.Printf("[subProcessMngr]: cmd.Process is Nil, start error")
	}
}
