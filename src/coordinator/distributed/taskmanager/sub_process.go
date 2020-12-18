package taskmanager

import (
	"context"
	"coordinator/common"
	"coordinator/logger"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"strings"
	"sync"
	"syscall"
	"time"
)

type SubProcessManager struct {
	sync.Mutex

	// if the subprocess be killed
	IsKilled 	bool
	// number of subprocess
	NumProc 	int
	// context
	Ctx 		context.Context
	Cancel		context.CancelFunc

}

func InitSubProcessManager() *SubProcessManager {
	pm := new(SubProcessManager)
	pm.NumProc = 0
	return pm
}


func (pm *SubProcessManager) SubProcessMonitor(pid int) {

loop:
	for {
		select {
		case <- pm.Ctx.Done():

			_, err := syscall.Getpgid(pid)
			if err == nil {
				err = syscall.Kill(pid, syscall.SIGQUIT)
				if err != nil {
					logger.Do.Println("SubProcessManager: Manually Killed PID=cmd.Process.Pid Error",err)
				}else{
					logger.Do.Println("SubProcessManager: Manually Killed PID=cmd.Process.Pid", pid)
				}
			}else{
				logger.Do.Printf("SubProcessManager: PID %d is not running\n", pid)
			}
			break loop
		default:
			time.Sleep(time.Second*1)
		}
	}
}


func (pm *SubProcessManager) CreateResources(
	cmd *exec.Cmd,
	envs []string,

) (string, string) {
/**
 * @Author
 * @Description
 * @Date 11:54 上午 10/12/20
 * @Param
	// Env specifies the environment of the process.
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
		logger.Do.Println("SubProcessManager: lock")
		pm.Lock()
		pm.NumProc -= 1
		pm.Unlock()
		logger.Do.Println("SubProcessManager: Unlock ")
	}()

	if len(envs) > 0 {
		cmd.Env = append(os.Environ(), envs...)
	}

	stderr, err := cmd.StderrPipe()
	if err != nil {
		logger.Do.Println(err)
		return err.Error(), ""
	}

	if err := cmd.Start(); err != nil {
		logger.Do.Println(err)
		return err.Error(), ""
	}

	// if start successfully
	if cmd.Process != nil {
		logger.Do.Println("SubProcessManager: Open subProcess, PID=", cmd.Process.Pid)

		go pm.SubProcessMonitor(cmd.Process.Pid)
		// if there is a running KillSubProc, nTasks add 1
		pm.Lock()
		pm.NumProc += 1
		pm.Unlock()

		exitStatus := cmd.Wait()
		errLog, _ := ioutil.ReadAll(stderr)

		// oe is
		var exitStr string
		if exitStatus != nil {
			exitStr = exitStatus.Error()
		} else {
			exitStr = common.SubProcessNormal
		}

		logger.Do.Println("SubProcessManager: subprocess exitStr:", exitStr)
		logger.Do.Println("SubProcessManager: subprocess errorLogs:", string(errLog))
		return exitStr, string(errLog)

	} else {
		return errors.New("SubProcessManager: cmd.Process is Nil, start error").Error(), ""
	}
}



func (pm *SubProcessManager) ExecuteSubProcStream(
	dir, stdIn, commend string,
	args []string,
	envs []string,
) (bool, string, string, string) {
	/**
	 * @Author
	 * @Description dont use this for now.
	 * @Date 4:04 下午 14/12/20
	 * @Param
	 * @return
	 **/
	defer func() {
		logger.Do.Println("SubProcessManager: Getting lock")
		pm.Lock()
		pm.NumProc -= 1
		logger.Do.Println("SubProcessManager: Unlock")
		pm.Unlock()
		logger.Do.Println("SubProcessManager: Unlock done")
	}()

	cmd := exec.Command(commend, args...)

	if len(dir) > 0 {
		cmd.Dir = dir
	}

	if len(envs) > 0 {
		cmd.Env = append(os.Environ(), envs...)
	}

	if len(stdIn) > 0 {
		cmd.Stdin = strings.NewReader(stdIn)
	}

	// shutdown the process after 2 hour, if still not finish
	cmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}

	time.AfterFunc(2*time.Hour, func() {
		err := syscall.Kill(cmd.Process.Pid, syscall.SIGQUIT)
		logger.Do.Println("SubProcessManager: Timeout, Killed PID=cmd.Process.Pid")
		if err != nil {
			logger.Do.Fatal(err)
		}
	})

	var out outstream
	cmd.Stdout = out

	if err := cmd.Start(); err != nil {
		logger.Do.Println(err)
		return false, err.Error(), "", ""
	}

	// if start successfully
	if cmd.Process != nil {
		logger.Do.Println("SubProcessManager: Open subProcess, PID=", cmd.Process.Pid)

		go pm.SubProcessMonitor(cmd.Process.Pid)
		// if there is a running KillSubProc, nTasks add 1
		pm.Lock()
		pm.NumProc += 1
		pm.Unlock()

		var outErr error

		//阻塞直到该命令执行完成，该命令必须是被Start方法开始执行的
		outErr = cmd.Wait()

		var oe string
		if outErr != nil {
			oe = outErr.Error()
		} else {
			oe = common.SubProcessNormal
		}
		return true, oe, "", ""

	} else {
		return false, errors.New("SubProcessManager: cmd.Process is Nil, start error").Error(), "", ""
	}
}


type outstream struct{}

func (out outstream) Write(p []byte) (int, error) {
	fmt.Println(string(p))
	return len(p), nil
}


func ExecuteBash(command string) error{
	// 返回一个 cmd 对象
	logger.Do.Println("SubProcessManager: execute bash ::", command)

	cmd := exec.Command("bash", "-c", command)

	stderr, _ := cmd.StderrPipe()
	stdout, _ := cmd.StdoutPipe()

	if err := cmd.Start(); err != nil {
		logger.Do.Println("ExecuteBash: Start error ",err)
		return err
	}
	errLog, _ := ioutil.ReadAll(stderr)
	outLog, _ := ioutil.ReadAll(stdout)

	logger.Do.Println("ExecuteBash: ErrorLog is ",string(errLog))
	logger.Do.Println("ExecuteBash: OutPut is ",string(outLog))
	outErr := cmd.Wait()
	return outErr

}

func ExecuteOthers(command string) error{
	// 返回一个 cmd 对象

	logger.Do.Println("SubProcessManager: execute bash,", command)

	cmd := exec.Command(command)

	stderr, _ := cmd.StderrPipe()
	stdout, _ := cmd.StdoutPipe()

	if err := cmd.Start(); err != nil {
		logger.Do.Println("ExecuteBash: Start error ",err)
		return err
	}
	errLog, _ := ioutil.ReadAll(stderr)
	outLog, _ := ioutil.ReadAll(stdout)

	logger.Do.Println("ExecuteBash: ErrorLog is ",string(errLog))
	logger.Do.Println("ExecuteBash: OutPut is ",string(outLog))
	outErr := cmd.Wait()

	return outErr
}