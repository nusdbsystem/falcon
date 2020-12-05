package base

import (
	"coordinator/logger"
	"net"
	"net/rpc"
	"strings"
	"sync"
)

type RpcBase struct {
	sync.Mutex
	Name 		string
	Proxy 		string
	Address  	string //  which is the ip+port address of worker
	Port  	string //  which is the port address of worker
	Listener 	net.Listener

	// if the master is stopped
	IsStop 		bool
}

func (rb *RpcBase) InitRpc(masterAddr string) {
	rb.Proxy = "tcp"
	rb.Address = masterAddr

	h := strings.Split(masterAddr, ":")

	rb.Port = h[1]
	rb.IsStop = false
}



func (rb *RpcBase) StartRPCServer(rpcSvc *rpc.Server, isBlocking bool){


	logger.Do.Printf("%s: listening on %s, %s \n", rb.Name, rb.Proxy, "0.0.0.0:"+rb.Port)
	listener, e := net.Listen(rb.Proxy, "0.0.0.0:"+rb.Port)

	if e != nil {
		logger.Do.Printf("%s: StartRPCServer error", rb.Name)
	}

	rb.Listener = listener

	if !isBlocking{
		// accept connection
		go func() {
			// define loop label used for break
			for {
				conn, err := rb.Listener.Accept()
				if err == nil {
					logger.Do.Printf("%s: got new conn", rb.Name)
					// user thread to process requests
					go func() {
						rpcSvc.ServeConn(conn)
						_ = conn.Close()
					}()
				} else {

					logger.Do.Printf("%s: RegistrationServer: Accept errored, %v \n",rb.Name, err)
					break
				}
			}
			logger.Do.Printf("%s: masterServer: done\n", rb.Name)
		}()

	}else{

		for {
			// create a connection
			conn, err := rb.Listener.Accept()
			if err == nil {
				logger.Do.Println("Worker: got new conn")
				go func() {
					rpcSvc.ServeConn(conn)
					// close a connection
					_ = conn.Close()
				}()
			} else {
				logger.Do.Println("Worker: got conn error", err)
				break
			}
		}
	}


}

