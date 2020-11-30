package taskmanager

import (
	"coordinator/config"
	"errors"
	"io"
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
	IsWait  bool
	// if the subprocess is finished normally

}

func InitSubProcessManager() *SubProcessManager {
	pm := new(SubProcessManager)

	pm.IsStop = make(chan bool)
	pm.NumProc = 0

	pm.IsWait = true

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
					log.Println(err)
					//log.Fatal(err)
				}
				log.Println("SubProcessManager: Manually Killed PID=cmd.Process.Pid", pid)
				killed = true

				<-isFinish
				log.Println("SubProcessManager: Put true to isKilled")
				isKilled <- killed
				log.Println("SubProcessManager: Put true to isKilled done")

				log.Println("SubProcessManager: break the loop, quite KillSubProc thread")
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
		log.Println("SubProcessManager: Getting lock")
		pm.Lock()
		pm.NumProc -= 1
		log.Println("SubProcessManager: Unlock")
		pm.Unlock()
		log.Println("SubProcessManager: Unlock done")
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

	var stderr io.ReadCloser
	var stdout io.ReadCloser
	var err error

	if pm.IsWait{
		stderr, err = cmd.StderrPipe()
		if err != nil {
			log.Println(err)
			return false, err.Error(), "", ""

		}

		stdout, err = cmd.StdoutPipe()
		if err != nil {
			log.Println(err)
			return false, err.Error(), "", ""

		}
	}else{
		_, err := cmd.StderrPipe()
		if err != nil {
			log.Println(err)
			return false, err.Error(), "", ""

		}

		_, err = cmd.StdoutPipe()
		if err != nil {
			log.Println(err)
			return false, err.Error(), "", ""

		}
	}


	// shutdown the process after 2 hour, if still not finish
	cmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}
	time.AfterFunc(2*time.Hour, func() {
		err := syscall.Kill(cmd.Process.Pid, syscall.SIGQUIT)
		log.Println("SubProcessManager: Timeout, Killed PID=cmd.Process.Pid")
		if err != nil {
			log.Fatal(err)
		}
	})

	if err := cmd.Start(); err != nil {
		log.Println(err)
		return false, err.Error(), "", ""
	}

	isKilled := make(chan bool, 1)
	isFinish := make(chan bool, 1)
	// if start successfully
	if cmd.Process != nil {
		log.Println("SubProcessManager: Open subProcess, PID=", cmd.Process.Pid)

		go pm.KillSubProc(cmd.Process.Pid, isKilled, isFinish)
		// if there is a running KillSubProc, nTasks add 1
		pm.Lock()
		pm.NumProc += 1
		pm.Unlock()


		var errLog []byte
		var outLog []byte
		var outErr error

		if pm.IsWait{
			errLog, _ = ioutil.ReadAll(stderr)
			outLog, _ = ioutil.ReadAll(stdout)
			outErr = cmd.Wait()
		}


		var oe string
		if outErr != nil {
			oe = outErr.Error()
		} else {
			oe = config.SubProcessNormal
		}
		for {

			select {
			case killed := <-isKilled:
				log.Println("SubProcessManager: Job Done, return")
				return killed, oe, string(errLog), string(outLog)
			case isFinish <- true:
				log.Println("SubProcessManager: Write to isFinish")
			default:
			}
		}

	} else {
		return false, errors.New("SubProcessManager: cmd.Process is Nil, start error").Error(), "", ""
	}

}
