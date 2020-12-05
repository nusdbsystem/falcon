package main

import (
	"coordinator/api"
	"coordinator/cache"
	"coordinator/common"
	"coordinator/distributed"
	"coordinator/listener"
	"coordinator/logger"
	"os"
	"runtime"
	"time"
)

func init() {

	// prority: env >  user provided > default value
	runtime.GOMAXPROCS(4)
	verifyArgs()
	initLogger()

}

func verifyArgs() {

	if len(common.ServiceNameGlobal) == 0 && len(common.ExecutorTypeGlobal) == 0{
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

	if common.ExecutorTypeGlobal == common.MasterExecutor{

		if common.MasterAddrGlobal=="" || common.CoordAddrGlobal=="" {
			logger.Do.Println("Error: Input Error, either MasterAddrGlobal or CoordAddrGlobal not provided")
			os.Exit(1)
		}

	}else if common.ExecutorTypeGlobal == common.TrainExecutor{
		if common.WorkerAddrGlobal=="" || common.MasterAddrGlobal=="" || common.TaskTypeGlobal==""  {
			logger.Do.Println("Error: Input Error, either WorkerAddrGlobal or MasterAddrGlobal or TaskTypeGlobal not provided")
			os.Exit(1)
		}


	}else if common.ExecutorTypeGlobal == common.PredictExecutor{
		if common.WorkerAddrGlobal=="" || common.MasterAddrGlobal=="" || common.TaskTypeGlobal==""  {
			logger.Do.Println("Error: Input Error, either WorkerAddrGlobal or MasterAddrGlobal or TaskTypeGlobal not provided")
			os.Exit(1)
		}
	}
}

func initLogger(){
	// this path is fixed, used to creating folder inside container
	fixedPath:=".falcon_runtime_resources"
	_ = os.Mkdir(fixedPath, os.ModePerm)
	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	logFileName := fixedPath + common.ServiceNameGlobal + rawTime.Format(layout) + ".log"

	logger.Do, logger.F = logger.GetLogger(logFileName)
}


func main() {

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

		ServerAddress := common.CoordAddrGlobal + ":" + common.MasterPort
		listener.SetupListener(common.ListenAddrGlobal, common.ListenerPort, ServerAddress)
	}


	//////////////////////////////////////////////////////////////////////////
	//						 start tasks, called internally 				//
	// 																	    //
	//////////////////////////////////////////////////////////////////////////

	//those 2 is only called internally

	if common.ExecutorTypeGlobal == common.MasterExecutor {

		logger.Do.Println("Lunching coordinator_server, the common.ExecutorTypeGlobal", common.ExecutorTypeGlobal)

		// this should be the service name, defined at runtime,
		masterIp := common.MasterAddrGlobal

		// todo. get from redis

		qItem := &cache.QItem{}

		taskType := common.TaskTypeGlobal

		distributed.SetupMaster(masterIp, qItem, taskType)
	}

	if common.ExecutorTypeGlobal == common.TrainExecutor {

		logger.Do.Println("Lunching coordinator_server, the common.ExecutorTypeGlobal", common.ExecutorTypeGlobal)

		distributed.SetupTrain(common.WorkerAddrGlobal, common.MasterAddrGlobal)
	}

	if common.ExecutorTypeGlobal == common.PredictExecutor {

		logger.Do.Println("Lunching coordinator_server, the common.ExecutorTypeGlobal", common.ExecutorTypeGlobal)

		distributed.SetupPrediction(common.WorkerAddrGlobal, common.MasterAddrGlobal)

	}
}
