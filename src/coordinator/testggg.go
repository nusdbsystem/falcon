package main

import (
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"strings"
	"sync"
	"syscall"
	"time"
)


/*
("party-id", po::value<int>(&party_id), "current party id")
("party-num", po::value<int>(&party_num), "total party num")
("party-type", po::value<int>(&party_type), "type of this party, active or passive")
("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")

("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
("algorithm-parajobDB", po::value<std::string>(&algorithm_parajobDB), "parameters for the algorithm");


("network-file", po::value<std::string>(&network_file), "file name of network configurations")
("log-file", po::value<std::string>(&log_file), "file name of log destination")
("data-input-file", po::value<std::string>(&data_file), "file name of dataset")
("data-output-file", po::value<std::string>(&data_file), "file name of dataset")
("model-save-file", po::value<std::string>(&data_file), "file name of dataset")
("key-file", po::value<std::string>(&key_file), "file name of phe keys")
("model-report-file", po::value<std::string>(&key_file), "file name of phe keys")


 */


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
					fmt.Println(err)
					//fmt.Fatal(err)
				}
				fmt.Println("SubProcessManager: Manually Killed PID=cmd.Process.Pid", pid)
				killed = true

				<-isFinish
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

	var stderr io.ReadCloser
	var stdout io.ReadCloser
	var err error

	if pm.IsWait{
		stderr, err = cmd.StderrPipe()
		if err != nil {
			fmt.Println(err)
			return false, err.Error(), "", ""

		}

		stdout, err = cmd.StdoutPipe()
		if err != nil {
			fmt.Println(err)
			return false, err.Error(), "", ""

		}
	}else{
		_, err := cmd.StderrPipe()
		if err != nil {
			fmt.Println(err)
			return false, err.Error(), "", ""
		}

		_, err = cmd.StdoutPipe()
		if err != nil {
			fmt.Println(err)
			return false, err.Error(), "", ""

		}
	}


	// shutdown the process after 2 hour, if still not finish
	cmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}
	time.AfterFunc(2*time.Hour, func() {
		err := syscall.Kill(cmd.Process.Pid, syscall.SIGQUIT)
		fmt.Println("SubProcessManager: Timeout, Killed PID=cmd.Process.Pid")
		if err != nil {
			fmt.Println(err)
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
			oe = "ok"
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




func main(){

	out, err := exec.Command("env").Output()

	fmt.Println(string(out), err)
	fmt.Println("begin python")
	out, err = exec.Command("python").Output()
	fmt.Println(string(out), err)
	fmt.Println("begin python done")

	dir:=""
	stdIn := "input from keyboard"
	commend := "echo"
	args := []string{"$PATH"}
	envs := []string{}

	pm := InitSubProcessManager()

	//go func(){
	//	for i:=8;i>0;i--{
	//		time.Sleep(time.Second*1)
	//		fmt.Println("Before kill process",i)
	//	}
	//	pm.IsStop <-true
	//}()

	killed, e, el, ol := pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	fmt.Println(killed, e, el, ol)

	//fmt.Println("Worker:task model training start")
	//args = []string{"plot_out_of_core_classification.py", "-a 1 -b 1"}
	////
	//killed, e, el, ol = pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	//fmt.Println(killed, e, el, ol)

	time.Sleep(time.Second * 3)
}