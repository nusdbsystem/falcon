package taskmanager

import (
	"coordinator/config"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"strings"
	"sync"
	"syscall"
	"time"
)

type SubProcessManager struct {
	sync.Mutex

	// if stop the subprocess
	IsStop chan bool
	// number of subprocess
	NumProc int

	// if the subprocess is killed

	// if the subprocess is finished normally

}

func InitSubProcessManager() *SubProcessManager {
	pm := new(SubProcessManager)

	pm.IsStop = make(chan bool)
	pm.NumProc = 0

	return pm

}

func (pm *SubProcessManager) KillSubProc(pid int, isKilled, isFinish chan bool) {

	killed := false
loop:
	for {
		select {
		case stop := <-pm.IsStop:
			if stop == true {

				err := syscall.Kill(pid, syscall.SIGQUIT)
				if err != nil {
					fmt.Println(err)
					//log.Fatal(err)
				}
				fmt.Println("SubProcessManager: Manually Killed PID=cmd.Process.Pid", pid)
				killed = true

				<-isFinish
				fmt.Println("SubProcessManager: Put true to isKilled")
				isKilled <- killed
				fmt.Println("SubProcessManager: Put true to isKilled done")

				fmt.Println("SubProcessManager: break the loop, quite KillSubProc thread")
				break loop
			}
		case finish := <-isFinish:
			if finish == true {
				killed = false
				isKilled <- killed
				break loop
			}
		default:
		}
	}
}

func (pm *SubProcessManager) ExecuteSubProc(
	dir, stdIn, commend string,
	args []string,
	envs []string,
) (bool, string, string, string) {

	defer func() {
		fmt.Println("SubProcessManager: Getting lock")
		pm.Lock()
		pm.NumProc -= 1
		fmt.Println("SubProcessManager: Unlock")
		pm.Unlock()
		fmt.Println("SubProcessManager: Unlock done")
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

	stderr, err := cmd.StderrPipe()
	if err != nil {
		fmt.Println(err)
		return false, err.Error(), "", ""

	}

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		fmt.Println(err)
		return false, err.Error(), "", ""

	}

	// shutdown the process after 2 hour, if still not finish
	cmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}
	time.AfterFunc(2*time.Hour, func() {
		err := syscall.Kill(cmd.Process.Pid, syscall.SIGQUIT)
		fmt.Println("SubProcessManager: Timeout, Killed PID=cmd.Process.Pid")
		if err != nil {
			log.Fatal(err)
		}
	})

	if err := cmd.Start(); err != nil {
		fmt.Println(err)
		return false, err.Error(), "", ""
	}

	isKilled := make(chan bool, 1)
	isFinish := make(chan bool, 1)
	// if start successfully
	if cmd.Process != nil {
		fmt.Println("SubProcessManager: Open subProcess, PID=", cmd.Process.Pid)

		go pm.KillSubProc(cmd.Process.Pid, isKilled, isFinish)
		// if there is a running KillSubProc, nTasks add 1
		pm.Lock()
		pm.NumProc += 1
		pm.Unlock()

		errLog, _ := ioutil.ReadAll(stderr)
		outLog, _ := ioutil.ReadAll(stdout)

		outErr := cmd.Wait()

		var oe string
		if outErr != nil {
			oe = outErr.Error()
		} else {
			oe = config.SubProcessNormal
		}
		for {

			select {
			case killed := <-isKilled:
				fmt.Println("SubProcessManager: Job Done, return")
				return killed, oe, string(errLog), string(outLog)
			case isFinish <- true:
				fmt.Println("SubProcessManager: Write to isFinish")
			default:
			}
		}

	} else {
		return false, errors.New("SubProcessManager: cmd.Process is Nil, start error").Error(), "", ""
	}

}
