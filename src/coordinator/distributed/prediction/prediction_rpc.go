package prediction

import (
	"log"
	"net/rpc"
)

func RunPrediction(predHost, predPort, Proxy string){
	predAddress := predHost + ":" + predPort
	ServiceName := "ModelServing"

	msvc := new(ModelService)

	msvc.InitRpc(Proxy, predAddress)

	rpcSvc := rpc.NewServer()
	err := rpcSvc.Register(msvc)
	if err!= nil{
		log.Printf("%s: start Error \n", ServiceName)
		return
	}

	log.Printf("%s: register to masterAddress=%s \n", ServiceName, predAddress)

	msvc.StartRPCServer(rpcSvc, ServiceName, true)

}