package prediction

import (
	"coordinator/client"
	"coordinator/config"
	"coordinator/distributed/base"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
)

type ModelService struct{
	base.RpcBase
	pm         *taskmanager.SubProcessManager

}

func (msvc *ModelService) DoTask(arg []byte, _ *struct{}) error {

	var dta *entitiy.DoTaskArgs = entitiy.DecodeDoTaskArgs(arg)

	// no blocking ,
	go msvc.CreateService(dta)

	return nil
}


func (msvc *ModelService) CreateService(dta *entitiy.DoTaskArgs) {
	// todo build.sh sub process to run prediction job

	logger.Do.Println("ModelService: CreateService")
	dir := "PartyPath.DataInput"
	stdIn := ""
	command := "/Users/nailixing/.virtualenvs/test_pip/bin/python"
	args := []string{dta.ExecutablePath[0], "-a=1", "-b=2"}
	envs := []string{}

	// 2 thread will ready from isStop channel, only one is running at the any time

	killed, e, el, ol := msvc.pm.ExecuteSubProc(dir, stdIn, command, args, envs)

	logger.Do.Println("Worker:task 1 pre processing done", killed, e, el, ol)

	if e != config.SubProcessNormal {
		// return res is used to control the rpc call status, always return nil, but
		// keep error at rep.Errs
		return
	}

}


func (msvc *ModelService) LaunchService() error {

	return nil
}


func (msvc *ModelService) UpdateService() error {
	return nil
}

func (msvc *ModelService) QueryService() error {
	return nil
}

func (msvc *ModelService) StopService() {
	msvc.pm.IsStop <- true
}

func (msvc *ModelService) DeleteService() error {
	return nil
}


// call the master's register method,
func (msvc *ModelService) register(master string) {
	args := new(entitiy.RegisterArgs)
	args.WorkerAddr = msvc.Address

	logger.Do.Printf("ModelService Worker: begin to call Master.Register to register address= %s \n", args.WorkerAddr)
	ok := client.Call(master, msvc.Proxy, "Master.Register", args, new(struct{}))
	if ok == false {
		logger.Do.Printf("ModelService Worker: Register RPC %s, register error\n", master)
	}
}