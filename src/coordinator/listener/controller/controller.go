package controller

import (
	"coordinator/config"
	dist "coordinator/distributed"
)

func SetupWorker(httpHost, masterAddress, taskType string) {

	if taskType == config.TrainTaskType{
		e := dist.SetupWorker(httpHost, masterAddress)
		if e != nil {
			panic(e)
		}
	}else if taskType == config.PredictTaskType{
		e := dist.SetupPredictionHelper(httpHost, masterAddress)
		if e != nil {
			panic(e)
		}
	}


}
