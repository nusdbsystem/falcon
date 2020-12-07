package main

import (
	"coordinator/api"
	"coordinator/cache"
	"coordinator/common"
	"coordinator/distributed"
	"coordinator/distributed/prediction"
	"coordinator/distributed/worker"
	"coordinator/listener"
	"coordinator/logger"
	"os"
	"runtime"
	"time"
)

func init() {

	// prority: env >  user provided > default value
	runtime.GOMAXPROCS(4)
	initLogger()
	verifyArgs()

}

func verifyArgs() {

	if len(common.ServiceNameGlobal) == 0 && len(common.ExecutorTypeGlobal) == 0 && common.ISMASTER=="false"{
		logger.Do.Println("Error: Input Error, ServiceNameGlobal is either 'coord' or 'listener' ")
		os.Exit(1)
	}

	if common.ServiceNameGlobal == "coord"{
		if common.CoordAddrGlobal==""{
			logger.Do.Println("Error: Input Error, common.CoordAddrGlobal not provided")
			os.Exit(1)
		}


	}else if common.ServiceNameGlobal == "listener"{
		if common.CoordAddrGlobal=="" || common.ListenAddrGlobal=="" {
			logger.Do.Println("Error: Input Error, either CoordAddrGlobal or ListenAddrGlobal not provided")
			os.Exit(1)
		}
	}

	// this will be executed only in production, in dev, the common.ExecutorTypeGlobal==""
	if common.ISMASTER == "true"{

		if common.CoordSvcName==""{
			logger.Do.Println("Error: Input Error, either MasterAddrGlobal or CoordAddrGlobal not provided")
			os.Exit(1)
		}

	}else if common.ExecutorTypeGlobal == common.TrainExecutor{
		if common.MasterURLGlobal=="" || common.WorkerURLGlobal=="" {
			logger.Do.Println("Error: Input Error, either WorkerAddrGlobal or MasterAddrGlobal or TaskTypeGlobal not provided")
			os.Exit(1)
		}


	}else if common.ExecutorTypeGlobal == common.PredictExecutor{
		if common.MasterURLGlobal=="" || common.WorkerURLGlobal==""{
			logger.Do.Println("Error: Input Error, either WorkerAddrGlobal or MasterAddrGlobal or TaskTypeGlobal not provided")
			os.Exit(1)
		}
	}
}

func initLogger(){
	// this path is fixed, used to creating folder inside container
	fixedPath:="/logs"
	_ = os.Mkdir(fixedPath, os.ModePerm)
	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	logFileName := fixedPath + "/" + common.ServiceNameGlobal + rawTime.Format(layout) + "logs"

	logger.Do, logger.F = logger.GetLogger(logFileName)
}


func main() {

	defer logger.HandleErrors()

	defer func(){
		_=logger.F.Close()
	}()

	if common.ServiceNameGlobal == "coord" {
		logger.Do.Println("Launch coordinator_server, the common.ServiceNameGlobal", common.ServiceNameGlobal)

		api.SetupHttp(3)
	}

	// start work in remote machine automatically
	if common.ServiceNameGlobal == "listener" {

		logger.Do.Println("Launch coordinator_server, the common.ServiceNameGlobal", common.ServiceNameGlobal)

		listener.SetupListener()
	}


	//////////////////////////////////////////////////////////////////////////
	//						 start tasks, called internally 				//
	// 																	    //
	//////////////////////////////////////////////////////////////////////////

	//those 2 is only called internally

	if common.ISMASTER == "true" {

		logger.Do.Println("Lunching coordinator_server, the common.ExecutorTypeGlobal", common.ExecutorTypeGlobal)

		// this should be the service name, defined at runtime,
		masterUrl := common.MasterURLGlobal

		qItem := cache.Deserialize(cache.RedisClient.Get(common.MasterQItem))

		taskType := common.ExecutorTypeGlobal

		distributed.SetupMaster(masterUrl, qItem, taskType)

	}else{

		if common.ExecutorTypeGlobal == common.TrainExecutor {

			logger.Do.Println("Lunching coordinator_server, the common.ExecutorTypeGlobal", common.ExecutorTypeGlobal)

			worker.RunWorker(common.MasterURLGlobal, common.WorkerURLGlobal)
		}

		if common.ExecutorTypeGlobal == common.PredictExecutor {

			logger.Do.Println("Lunching coordinator_server, the common.ExecutorTypeGlobal", common.ExecutorTypeGlobal)

			prediction.RunPrediction(common.MasterURLGlobal, common.WorkerURLGlobal)

		}


	}
}
