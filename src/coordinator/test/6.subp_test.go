package test

import (
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"fmt"
	"os/exec"
	"testing"
	"time"
)

func TestSubProc(t *testing.T) {
	logger.Do, logger.F = logger.GetLogger("./TestSubProc")

	//dir := "/Users/nailixing/GOProj/src/coordinator/falcon_ml"

	out, err := exec.Command("python3", "/go/preprocessing.py", "-a=1", "-b=2").Output()
	fmt.Println(out, err)


	cmd := exec.Command("python3", "/go/preprocessing.py", "-a=1", "-b=2")
	var envs []string

	pm := taskmanager.InitSubProcessManager()

	//go func(){
	//	for i:=8;i>0;i--{
	//		time.Sleep(time.Second*1)
	//		logger.Do.Println("Before kill process",i)
	//	}
	//	pm.IsStop <-true
	//}()

	killed, el, e:= pm.CreateResources(cmd, envs)
	logger.Do.Println(killed, e, el)

	//logger.Do.Println("Worker:task model training start")
	//args = []string{"plot_out_of_core_classification.py", "-a 1 -b 1"}
	////
	//killed, e, el, ol = pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	//logger.Do.Println(killed, e, el, ol)

	time.Sleep(time.Second * 3)
}

func TestSubProcessShell(t *testing.T){
	logger.Do, logger.F = logger.GetLogger("./TestSubProc")

	commend := "./scripts/_create_runtime_master.sh master-6369386254669931332 30006 6369386254669931332 train"
	err := taskmanager.ExecuteBash(commend)
	fmt.Println(err)

	_=taskmanager.ExecuteBash("ls")
	_=taskmanager.ExecuteBash("pwd")

}
