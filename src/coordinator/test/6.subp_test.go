package test

import (
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"testing"
	"time"
)

func TestSubProc(t *testing.T) {
	logger.Do, logger.F = logger.GetLogger("./TestSubProc")

	//dir := "/Users/nailixing/GOProj/src/coordinator/falcon_ml"
	dir:=""
	stdIn := "input from keyboard"
	commend := "docker"
	args := []string{"image ls"}
	envs := []string{}

	pm := taskmanager.InitSubProcessManager()

	//go func(){
	//	for i:=8;i>0;i--{
	//		time.Sleep(time.Second*1)
	//		logger.Do.Println("Before kill process",i)
	//	}
	//	pm.IsStop <-true
	//}()

	killed, e, el, ol := pm.ExecuteSubProc2(dir, stdIn, commend, args, envs)
	logger.Do.Println(killed, e, el, ol)

	//logger.Do.Println("Worker:task model training start")
	//args = []string{"plot_out_of_core_classification.py", "-a 1 -b 1"}
	////
	//killed, e, el, ol = pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	//logger.Do.Println(killed, e, el, ol)

	time.Sleep(time.Second * 3)
}
