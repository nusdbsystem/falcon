package main

import (
	"coordinator/api"
	"coordinator/common"
	"coordinator/distributed"
	"coordinator/listener"
	"coordinator/logger"
	"flag"
	"os"
	"runtime"
	"strings"
	"time"
)

var svc string
var httpAddr string
var listenerAddr string

var taskType string
var workerAddr string
var masterAddr string

func init() {
	runtime.GOMAXPROCS(4)
	flag.StringVar(&svc, "svc", "coordinator", "choose which service to run, 'coordinator' or 'listener'")
	flag.StringVar(&httpAddr, "cip", "localhost", "Ip Address of coordinator")
	flag.StringVar(&listenerAddr, "lip", "localhost", "Ip Address of listener")

	// worker ip ,master addr
	flag.StringVar(&taskType, "taskType", "", "master, train, predict")
	flag.StringVar(&workerAddr, "workerAddr", "", "Ip Address of WorkerAddr")
	flag.StringVar(&masterAddr, "masterAddr", "", "Ip Address of master, this is only used for predictor")
}

func verifyArgs() {

	if !(
			strings.Contains(svc, "coord") ||
			strings.Contains(svc, "listener")){
		logger.Do.Println("Error: Input Error, svc is either 'coordinator' or 'listener' or 'predictor' ")
		os.Exit(1)
	}
}

func checkEnv(){
	/**
	 * @Author
	 * @Description , if there is env, use env (in prod), otherwise use parameters provided, (in dev), pick the env first be default
	 * @Date 8:36 下午 3/12/20
	 * @Param
	 * @return
	 **/
	svc = common.GetEnv("SERVICE_NAME", svc)
	httpAddr = common.GetEnv("COORDINATOR_IP", httpAddr)
	listenerAddr = common.GetEnv("LISTENER_IP", listenerAddr)
}


func initLogger(){
	// this path is fixed, used to creating folder inside container
	fixedPath:=".falcon_runtime_resources"
	_ = os.Mkdir(fixedPath, os.ModePerm)
	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	logFileName := fixedPath + svc + rawTime.Format(layout) + ".log"

	logger.Do, logger.F = logger.GetLogger(logFileName)
}


func main() {
	flag.Parse()
	verifyArgs()
	initLogger()
	checkEnv()

	defer func(){
		_=logger.F.Close()
	}()

	// start work in remote machine automatically
	if svc == "listener" {
		if len(httpAddr) == 0 {
			logger.Do.Println("Error: Input Error, Must Provide ip of coordinator")
			os.Exit(1)
		}
		logger.Do.Println("Launch coordinator_server, the svc", svc)

		ServerAddress := httpAddr + ":" + common.MasterPort
		listener.SetupListener(listenerAddr, common.ListenerPort, ServerAddress)
	}

	if svc == "coordinator" {
		logger.Do.Println("Launch coordinator_server, the svc", svc)

		api.SetupHttp(httpAddr, common.MasterPort, 3)
	}


	//those 2 is only called internally

	if taskType == common.MasterTaskType {

		logger.Do.Println("Lunching coordinator_server, the taskType", taskType)

		distributed.SetupMaster(workerAddr, masterAddr)
	}

	if taskType == common.TrainTaskType {

		if len(workerAddr) == 0 || len(masterAddr) == 0 {
			logger.Do.Println("Error: Input Error, Must Provide ip of predictor and masterAddr,", workerAddr, masterAddr)
			os.Exit(1)
		}
		logger.Do.Println("Lunching coordinator_server, the taskType", taskType)

		distributed.SetupTrain(workerAddr, masterAddr)
	}

	if taskType == common.PredictTaskType {

		if len(workerAddr) == 0 || len(masterAddr) == 0 {
			logger.Do.Println("Error: Input Error, Must Provide ip of trainWorkerAddr and masterAddr,", workerAddr, masterAddr)
			os.Exit(1)
		}
		logger.Do.Println("Lunching coordinator_server, the taskType", taskType)

		distributed.SetupPrediction(workerAddr, masterAddr)

	}
}
