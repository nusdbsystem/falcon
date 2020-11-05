package utils

import (
	"fmt"
	"net/rpc"
)

func Call(srv string, proxy string, rpcname string, args interface{}, reply interface{}) bool {

	fmt.Println("---------------in Calling----------------Address of this call:", proxy, srv, rpcname)
	c, derr := rpc.Dial(proxy, srv)
	if derr != nil {
		fmt.Println("---------------in Calling----------------Error: Connection error", derr)
		return false
	}
	defer c.Close()

	fmt.Println("---------------in Calling----------------method of this call", rpcname)
	cerr := c.Call(rpcname, args, reply)
	if cerr == nil {
		fmt.Println("---------------in Calling----------------Calling Done")
		return true
	} else {
		fmt.Println("---------------in Calling----------------Error: call method error--", cerr)
		return false
	}
}
