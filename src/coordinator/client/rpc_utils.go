package client

import (
	"coordinator/logger"
	"net/rpc"
)

func Call(srv string, proxy string, rpcname string, args interface{}, reply interface{}) bool {

	logger.Do.Printf("---------------in Calling----------------Address of this call, proxy: %s, address: %s, methodName: %s \n:", proxy, srv, rpcname)
	c, derr := rpc.Dial(proxy, srv)
	if derr != nil {
		logger.Do.Println("---------------in Calling----------------Error: Connection error", derr)
		return false
	}
	defer c.Close()

	logger.Do.Println("---------------in Calling----------------method of this call", rpcname)
	cerr := c.Call(rpcname, args, reply)
	if cerr == nil {
		logger.Do.Println("---------------in Calling----------------Calling Done")
		return true
	} else {
		logger.Do.Println("---------------in Calling----------------Error: call method error--", cerr)
		return false
	}
}
