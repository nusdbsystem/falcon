package test

import (
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"testing"
	"time"
)

func TestSubProc(t *testing.T) {

	//dir := "/Users/nailixing/GOProj/src/coordinator/falcon_ml"
	dir:=""
	stdIn := "input from keyboard"
	commend := "/Users/nailixing/.virtualenvs/test_pip/bin/python3.6"
	args := []string{"/Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/falcon_ml/preprocessing.py", "-a 1 -b 1"}
	envs := []string{}

	pm := taskmanager.InitSubProcessManager()

	//go func(){
	//	for i:=8;i>0;i--{
	//		time.Sleep(time.Second*1)
	//		logger.Do.Println("Before kill process",i)
	//	}
	//	pm.IsStop <-true
	//}()

	killed, e, el, ol := pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	logger.Do.Println(killed, e, el, ol)

	//logger.Do.Println("Worker:task model training start")
	//args = []string{"plot_out_of_core_classification.py", "-a 1 -b 1"}
	////
	//killed, e, el, ol = pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	//logger.Do.Println(killed, e, el, ol)

	time.Sleep(time.Second * 3)
}
