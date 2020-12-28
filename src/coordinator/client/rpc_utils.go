package client

import (
	"coordinator/exceptions"
	"coordinator/logger"
	"net/rpc"
	"time"
)

func Call(address string, network string, rpcname string, args interface{}, reply interface{}) bool {
	/**
	 * @Author
	 * @Description if connection error, retry, otherwise, return false
	 * @Date 3:01 下午 13/12/20
	 * @Param
	 * @return,
			true: successfully,
			false: error happens
	 **/

	NTimes := 3
	for {
		if NTimes < 0 {
			logger.Do.Printf("RpcCall: Connection error, max retry reached!")
			return false
		}
		res := doCall(address, network, rpcname, args, reply)
		if res != nil {

			if res.Error() == exceptions.ConnectionErr {
				logger.Do.Printf("RpcCall: Connection error, retry.......")
				time.Sleep(time.Second * 3)
				NTimes--
			}

			if res.Error() == exceptions.CallingErr {
				logger.Do.Printf("RpcCall: Call method error, return")
				return false
			}

		} else {
			logger.Do.Printf("RpcCall: Calling successfully")
			return true
		}
	}
}

func doCall(address string, network string, rpcname string, args interface{}, reply interface{}) error {

	logger.Do.Printf("----in Calling----, Calling network: %s, addr: %s, methodName: %s \n", network, address, rpcname)
	client, err := rpc.Dial(network, address)
	if err != nil {
		logger.Do.Printf("----in Calling----, Connection error, <<%s>>\n", err)
		return exceptions.ConnectionError()
	}
	defer client.Close()

	cerr := client.Call(rpcname, args, reply)
	if cerr == nil {
		return nil
	} else {
		logger.Do.Printf("----in Calling----, Call method error, <<%s>>\n", cerr)
		return exceptions.CallingError()
	}
}
