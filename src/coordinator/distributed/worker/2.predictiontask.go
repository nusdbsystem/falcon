package worker

import (
	"coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/logger"
	"os/exec"
)

func (wk *PredictWorker) CreateService(dta *entitiy.DoTaskArgs) {
	// todo gobuild.sh sub process to run prediction job

	logger.Do.Println("PredictWorker: CreateService")

	cmd := exec.Command("python3", "/go/preprocessing.py", "-a=1", "-b=2")

	var envs []string

	// 2 thread will ready from isStop channel, only one is running at the any time

	el,e := wk.Pm.CreateResources(cmd, envs)

	logger.Do.Println("Worker:task 1 pre processing done", el)

	if e != common.SubProcessNormal {
		// return res is used to control the rpc call status, always return nil, but
		// keep error at rep.Errs
		return
	}

}


func (wk *PredictWorker) LaunchService() error {

	return nil
}


func (wk *PredictWorker) UpdateService() error {
	return nil
}

func (wk *PredictWorker) QueryService() error {
	return nil
}

func (wk *PredictWorker) StopService() {
}

func (wk *PredictWorker) DeleteService() error {
	return nil
}