package taskmanager

import (
	"context"
	"errors"
	"falcon_platform/common"
	"falcon_platform/distributed/utils"
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
					logger.Log.Println("[SubProcessManager]: Manually Killed PID=cmd.Process.Pid Error", err)
				} else {
					logger.Log.Println("[SubProcessManager]: Manually Killed PID=cmd.Process.Pid", pid)
				}
			} else {
				logger.Log.Printf("[SubProcessManager]: PID %d is not running\n", pid)
			}
			break loop
		default:
			time.Sleep(time.Second * 1)
		}
	}
}

func CreateProc(
	cmd *exec.Cmd,
	ctx context.Context,
	mux *sync.Mutex,
	NumProc *int,

) (string, string) {
	/**
	   * @Author
	   * @Description
	   * @Date 11:54 上午 10/12/20
	   * @Param
	        // Each entry is of the form "key=value".
	        // If Env is nil, the new process uses the current process's
	        // environment.
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
		//logger.Log.Println("[SubProcessManager]: lock")
		mux.Lock()
		*NumProc -= 1
		mux.Unlock()
		//logger.Log.Println("[SubProcessManager]: Unlock ")
	}()

	stderr, err := cmd.StderrPipe()
	if err != nil {
		logger.Log.Println(err)
		return err.Error(), ""
	}
	var out utils.OutStream
	cmd.Stdout = out

	// start with a short statement to execute before the condition
	if err := cmd.Start(); err != nil {
		logger.Log.Println("cmd.Start() Error:")
		logger.Log.Println(err)
		return err.Error(), ""
	}

	// if start successfully
	if cmd.Process != nil {
		logger.Log.Println("[SubProcessManager]: open subProcess, PID=", cmd.Process.Pid)

		go ProcMonitor(cmd.Process.Pid, ctx)
		// if there is a running SubProc, nTasks add 1
		mux.Lock()
		*NumProc += 1
		mux.Unlock()

		errLog, _ := ioutil.ReadAll(stderr)
		executeStatus := cmd.Wait()

		// oe is
		var executeStr string
		if executeStatus != nil {
			executeStr = executeStatus.Error()
		} else {
			executeStr = common.SubProcessNormal
		}

		logger.Log.Printf("[SubProcessManager]: subprocess exit status: << %s >> \n", executeStr)
		logger.Log.Printf("[SubProcessManager]: subprocess error logs: \n"+
			"<<<<<\n "+
			"%s \n"+
			"<<<<<\n",
			string(errLog))

		return executeStr, string(errLog)

	} else {
		return errors.New("[SubProcessManager]: cmd.Process is Nil, start error").Error(), ""
	}
}
