package client

import (
	"falcon_platform/exceptions"
	"falcon_platform/logger"
	"net/rpc"
	"time"
)

// Rpc call
func Call(address string, network string, rpcname string, args interface{}, reply interface{}) bool {

	NTimes := 3
	for {
		if NTimes < 0 {
			logger.Log.Printf("RpcCall: Connection error, max retry reached!")
			return false
		}
		res := doCall(address, network, rpcname, args, reply)
		if res != nil {

			if res.Error() == exceptions.ConnectionErr {
				logger.Log.Printf("RpcCall: Connection error, retry.......")
				time.Sleep(time.Second * 1)
				NTimes--
			}

			if res.Error() == exceptions.CallingErr {
				logger.Log.Printf("RpcCall: Call method error, return")
				return false
			}

		} else {
			//logger.Log.Printf("RpcCall: Calling successfully")
			return true
		}
	}
}

func doCall(address string, network string, rpcname string, args interface{}, reply interface{}) error {

	logger.Log.Printf("----Rpc Call----, addr: %s, methodName: %s \n", address, rpcname)
	client, err := rpc.Dial(network, address)
	if err != nil {
		logger.Log.Printf("----Rpc Call----, Connection error, <<%s>>\n", err)
		return exceptions.ConnectionError()
	}
	defer client.Close()

	cerr := client.Call(rpcname, args, reply)
	if cerr == nil {
		return nil
	} else {
		logger.Log.Printf("----Rpc Call----, Call method error, <<%s>>\n", cerr)
		return exceptions.CallingError()
	}
}
