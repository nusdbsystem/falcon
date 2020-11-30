package client

import (
	"log"
	"net/rpc"
)

func Call(srv string, proxy string, rpcname string, args interface{}, reply interface{}) bool {

	log.Printf("---------------in Calling----------------Address of this call, proxy: %s, address: %s, methodName: %s \n:", proxy, srv, rpcname)
	c, derr := rpc.Dial(proxy, srv)
	if derr != nil {
		log.Println("---------------in Calling----------------Error: Connection error", derr)
		return false
	}
	defer c.Close()

	log.Println("---------------in Calling----------------method of this call", rpcname)
	cerr := c.Call(rpcname, args, reply)
	if cerr == nil {
		log.Println("---------------in Calling----------------Calling Done")
		return true
	} else {
		log.Println("---------------in Calling----------------Error: call method error--", cerr)
		return false
	}
}
