package controller

import (
	"coordinator/config"
	dist "coordinator/distributed"
)

func SetupWorker(httpHost, masterAddress, taskType string) {

	if taskType == config.TrainTaskType{
		// launching the thread, to manage the subprocess train job

		// todo should we use process instead of thread?

		e := dist.SetupWorker(httpHost, masterAddress)
		if e != nil {
			panic(e)
		}
	}else if taskType == config.PredictTaskType{
		// launching the subprocess or docker containers
		e := dist.SetupPredictionHelper(httpHost, masterAddress)
		if e != nil {
			panic(e)
		}
	}


}
