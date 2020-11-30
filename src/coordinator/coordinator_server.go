package main

import (
	"coordinator/api"
	"coordinator/config"
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
var predictorAddr string
var masterAddr string

func init() {
	runtime.GOMAXPROCS(4)
	flag.StringVar(&svc, "svc", "coordinator", "choose which service to run, 'coordinator' or 'listener'")
	flag.StringVar(&httpAddr, "cip", "localhost", "Ip Address of coordinator")
	flag.StringVar(&listenerAddr, "lip", "localhost", "Ip Address of listener")
	flag.StringVar(&predictorAddr, "pip", "localhost", "Ip Address of predictor")
	flag.StringVar(&masterAddr, "master_addr", "localhost", "Ip Address of master, this is only used for predictor")
}

func verifyArgs() {


	if !(strings.Contains(svc, "coordinator") || strings.Contains(svc, "listener") || strings.Contains(svc, "predictor") ) {
		logger.Do.Println("Error: Input Error, svc is either 'coordinator' or 'listener' or 'predictor' ")
		os.Exit(1)
	}
}

func main() {
	flag.Parse()
	verifyArgs()

	_ = os.Mkdir(".logs", os.ModePerm)
	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	logFileName := ".logs/" + svc + rawTime.Format(layout) + ".log"

	logger.Do, logger.F = logger.GetLogger(logFileName)
	defer logger.F.Close()

	// start work in remote machine automatically
	if svc == "listener" {
		if len(httpAddr) == 0 {
			logger.Do.Println("Error: Input Error, Must Provide ip of coordinator")
			os.Exit(1)
		}
		if len(listenerAddr) == 0 {
			logger.Do.Println("Error: Input Error, Must Provide ip of listener")
			os.Exit(1)
		}
		logger.Do.Println("Launch coordinator_server, the svc", svc)

		ServerAddress := httpAddr + ":" + config.MasterPort
		listener.SetupListener(listenerAddr, config.ListenerPort, ServerAddress)
	}

	if svc == "coordinator" {
		if len(httpAddr) == 0 {
			logger.Do.Println("Error: Input Error, Must Provide ip of coordinator")
			os.Exit(1)
		}
		logger.Do.Println("Launch coordinator_server, the svc", svc)

		api.SetupHttp(httpAddr, config.MasterPort, 3)
	}

	if svc == "predictor" {

		if len(predictorAddr) == 0 || len(masterAddr) ==0 {
			logger.Do.Println("Error: Input Error, Must Provide ip of predictor and masterAddr,", predictorAddr, masterAddr)
			os.Exit(1)
		}
		logger.Do.Println("Lunching coordinator_server, the svc", svc)

		distributed.SetupPrediction(predictorAddr, masterAddr)

	}
}
