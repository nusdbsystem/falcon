package client

import (
	"coordinator/logger"
	"net/rpc"
	"time"
)

func Call(srv string, proxy string, rpcname string, args interface{}, reply interface{}) bool {

	NTimes := 20
	for {
		if NTimes<0{
			panic("RpcCall failed!")
		}
		res := doCall(srv, proxy, rpcname, args, reply)
		if res == false{
			logger.Do.Printf("RpcCall: failed, retry.......")
			time.Sleep(time.Second*3)
			NTimes--
		}else if res == true{
			logger.Do.Printf("RpcCall: successfully")
			return res
		}
	}
}

func doCall(srv string, proxy string, rpcname string, args interface{}, reply interface{}) bool {

	logger.Do.Printf("---------------in Calling----------------Address of this call, proxy: %s, address: %s, methodName: %s \n", proxy, srv, rpcname)
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