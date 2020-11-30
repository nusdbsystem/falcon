package prediction

import (
	"coordinator/config"
	"coordinator/distributed/taskmanager"
	"log"
	"net/rpc"
)

func RunPrediction(masterAddress, predHost, predPort, Proxy string){
	predAddress := predHost + ":" + predPort

	msvc := new(ModelService)
	msvc.InitRpc(Proxy, predAddress)
	msvc.Name = config.ModelService
	msvc.pm = taskmanager.InitSubProcessManager()

	rpcSvc := rpc.NewServer()
	err := rpcSvc.Register(msvc)
	if err!= nil{
		log.Printf("%s: start Error \n", msvc.Name)
		return
	}

	log.Printf("%s: register to masterAddress=%s \n", msvc.Name, predAddress)

	msvc.register(masterAddress)

	msvc.StartRPCServer(rpcSvc, true)

}