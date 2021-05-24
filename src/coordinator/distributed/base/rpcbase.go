package base

import (
	"context"
	"coordinator/client"
	"coordinator/distributed/entity"
	"coordinator/logger"
	"net"
	"net/rpc"
	"strings"
	"sync"
)

type RpcBaseClass struct {
	sync.Mutex
	Name     string
	Network  string
	Addr     string //  IP + Port = addr of worker
	Port     string //  Port of worker
	Listener net.Listener

	Ctx    context.Context
	Cancel context.CancelFunc
}

func (rb *RpcBaseClass) InitRpcBase(Addr string) {
	logger.Log.Println("[rpcbase] InitRpcBase called with Addr ", Addr)
	rb.Network = "tcp"
	rb.Addr = Addr

	h := strings.Split(Addr, ":")

	rb.Port = h[1]

	rb.Ctx, rb.Cancel = context.WithCancel(context.Background())
}

func (rb *RpcBaseClass) StartRPCServer(rpcSvc *rpc.Server, isBlocking bool) {

	logger.Log.Printf("%s: listening on %s, %s \n", rb.Name, rb.Network, "0.0.0.0:"+rb.Port)
	listener, e := net.Listen(rb.Network, "0.0.0.0:"+rb.Port)

	if e != nil {
		logger.Log.Fatalf("%s: StartRPCServer error, %s\n", rb.Name, e)
	}

	rb.Listener = listener

	if !isBlocking {
		// accept connection
		go func() {
			// define loop label used for break
			for {
				conn, err := rb.Listener.Accept()
				if err == nil {
					logger.Log.Printf("%s: got new conn", rb.Name)
					// user thread to process requests
					go func() {
						rpcSvc.ServeConn(conn)
						_ = conn.Close()
					}()
				} else {

					logger.Log.Printf("%s: RegistrationServer: Accept errored, %v \n", rb.Name, err)
					break
				}
			}
			logger.Log.Printf("%s: masterServer: done\n", rb.Name)
		}()

	} else {

		for {
			// create a connection
			conn, err := rb.Listener.Accept()
			if err == nil {
				logger.Log.Println("Worker: got new conn")
				go func() {
					rpcSvc.ServeConn(conn)
					// close a connection
					_ = conn.Close()
				}()
			} else {
				logger.Log.Println("Worker: got conn error", err)
				break
			}
		}
	}
}

// stopRPCServer stops the master RPC server.
// This must be done through an RPC to avoid
// race conditions between the RPC server thread and the current thread.
func (rb *RpcBaseClass) StopRPCServer(addr, targetSvc string) {
	var reply entity.ShutdownReply

	logger.Log.Printf("%s: begin to call %s\n", rb.Name, targetSvc)
	ok := client.Call(addr, rb.Network, targetSvc, new(struct{}), &reply)
	if ok == false {
		logger.Log.Printf("%s: Cleanup: RPC %s error\n", rb.Name, addr)
	}
	logger.Log.Printf("%s: cleanupRegistration: done\n", rb.Name)
}
