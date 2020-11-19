package main

import (
	"coordinator/api"
	"coordinator/config"
	"coordinator/listener"
	"flag"
	"fmt"
	"runtime"
	"strings"
)

var svc string
var httpAddr string
var listenerAddr string

func init() {
	runtime.GOMAXPROCS(4)
	flag.StringVar(&svc, "svc", "coordinator", "choose which service to run, 'coordinator' or 'listener'")
	flag.StringVar(&httpAddr, "cip", "", "Ip Address of coordinator")
	flag.StringVar(&listenerAddr, "lip", "", "Ip Address of listener")
}

func main() {
	flag.Parse()
	if len(httpAddr) == 0 {
		fmt.Println("Error: Input Error, Must Provide ip of coordinator")
		return
	}

	if !(strings.Contains(svc, "coordinator") || strings.Contains(svc, "listener")){
		fmt.Println("Error: Input Error, svc is either 'coordinator' or 'listener'")
		return
	}

	// start work in remote machine automatically

	if svc == "listener" {

		if len(listenerAddr) == 0 {
			fmt.Println("Error: Input Error, Must Provide ip of listener")
			return
		}
		fmt.Println("Lunching coordinator_server, the svc", svc)

		masterAddr := httpAddr + ":" + config.MasterPort
		listener.SetupListener(listenerAddr, config.ListenerPort, masterAddr)
	}

	if svc == "coordinator" {
		fmt.Println("Lunching coordinator_server, the svc", svc)

		api.SetupHttp(httpAddr, config.MasterPort, 3)
	}
}
