package prediction

import (
	"coordinator/config"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
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
		logger.Do.Printf("%s: start Error \n", msvc.Name)
		return
	}

	logger.Do.Printf("%s: register to masterAddress=%s \n", msvc.Name, predAddress)

	msvc.register(masterAddress)

	msvc.StartRPCServer(rpcSvc, true)

}