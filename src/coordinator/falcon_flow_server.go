package main

import (
	"coordinator/api"
	"coordinator/config"
	"coordinator/listener"
	"flag"
	"fmt"
)

var svc string
var httpAddr string

func init() {

	flag.StringVar(&svc, "svc", "http", "choose which service to run")
	flag.StringVar(&httpAddr, "addr", "", "addr of master")
}

func main() {
	flag.Parse()
	fmt.Println("Lunching falcon_flow_server, the svc", svc)

	// start work in remote machine automatically

	if svc == "worker" {

		if len(httpAddr) == 0 {
			panic("Must Provide ip of master")
		}
		masterAddr := "172.25.122.185" + ":" + config.MasterPort
		listener.SetupListener("172.25.122.185", config.ListenerPort, masterAddr)
	}

	if svc == "http" {
		api.SetupHttp("172.25.122.185", config.MasterPort, 3)
	}
}
