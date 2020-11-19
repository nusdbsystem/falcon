package test

import (
	"coordinator/distributed/taskmanager"
	"log"
	"testing"
	"time"
)

func TestSubProc(t *testing.T) {

	dir := "/Users/nailixing/GOProj/src/coordinator/falcon_ml"
	stdIn := "input from keyboard"
	commend := "/Users/nailixing/.virtualenvs/test_pip/bin/python"
	args := []string{"preprocessing.py", "-a 1 -b 1"}
	envs := []string{}

	pm := taskmanager.InitSubProcessManager()

	//go func(){
	//	for i:=8;i>0;i--{
	//		time.Sleep(time.Second*1)
	//		log.Println("Before kill process",i)
	//	}
	//	pm.IsStop <-true
	//}()

	killed, e, el, ol := pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	log.Println(killed, e, el, ol)

	log.Println("Worker:task model training start")
	args = []string{"plot_out_of_core_classification.py", "-a 1 -b 1"}
	//
	killed, e, el, ol = pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	log.Println(killed, e, el, ol)

	time.Sleep(time.Second * 10)
}
