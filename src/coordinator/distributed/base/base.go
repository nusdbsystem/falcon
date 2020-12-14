package base

import (
	"context"
	"coordinator/client"
	"coordinator/distributed/entitiy"
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
	Port  		string //  which is the port address of worker
	Listener 	net.Listener

	Ctx    context.Context
	Cancel context.CancelFunc
}

func (rb *RpcBase) InitRpc(Address string) {
	rb.Proxy = "tcp"
	rb.Address = Address

	h := strings.Split(Address, ":")

	rb.Port = h[1]

	rb.Ctx, rb.Cancel = context.WithCancel(context.Background())
}



func (rb *RpcBase) StartRPCServer(rpcSvc *rpc.Server, isBlocking bool){


	logger.Do.Printf("%s: listening on %s, %s \n", rb.Name, rb.Proxy, "0.0.0.0:"+rb.Port)
	listener, e := net.Listen(rb.Proxy, "0.0.0.0:"+rb.Port)

	if e != nil {
		logger.Do.Fatalf("%s: StartRPCServer error, %s\n", rb.Name, e)
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

// stopRPCServer stops the master RPC server.
// This must be done through an RPC to avoid
// race conditions between the RPC server thread and the current thread.
func (rb *RpcBase) StopRPCServer(addr, targetSvc string) {
	var reply entitiy.ShutdownReply

	logger.Do.Printf("%s: begin to call %s\n", rb.Name, targetSvc)
	ok := client.Call(addr, rb.Proxy, targetSvc, new(struct{}), &reply)
	if ok == false {
		logger.Do.Printf("%s: Cleanup: RPC %s error\n", rb.Name, addr)
	}
	logger.Do.Printf("%s: cleanupRegistration: done\n", rb.Name)
}

