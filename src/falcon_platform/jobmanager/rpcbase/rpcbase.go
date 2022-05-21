package base

import (
	"context"
	"falcon_platform/client"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"fmt"
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

	Ctx   context.Context
	Clear context.CancelFunc
}

func (rb *RpcBaseClass) InitRpcBase(Addr string) {
	logger.Log.Println("[rpcbase] InitRpcBase called with Addr", Addr)
	rb.Network = "tcp"
	rb.Addr = Addr

	h := strings.Split(Addr, ":")

	rb.Port = h[1]

	rb.Ctx, rb.Clear = context.WithCancel(context.Background())
}

func (rb *RpcBaseClass) StartRPCServer(rpcSvc *rpc.Server, isBlocking bool) {
	//logger.Log.Println("[rpcbase] StartRPCServer called")

	logger.Log.Printf("[%s]: listening on %s, %s \n", rb.Name, rb.Network, "0.0.0.0:"+rb.Port)
	listener, e := net.Listen(rb.Network, "0.0.0.0:"+rb.Port)

	if e != nil {
		panic(fmt.Sprintf("[%s]: StartRPCServer error, %s\n", rb.Name, e))
	}

	rb.Listener = listener

	if !isBlocking {
		// accept connection
		go func() {
			defer logger.HandleErrors()
			// define loop label used for break
			for {
				conn, err := rb.Listener.Accept()
				if err == nil {

					//logger.Log.Printf("%s: got new conn", rb.Name)
					// user thread to process requests
					go func(conn net.Conn) {
						defer logger.HandleErrors()
						rpcSvc.ServeConn(conn)
						_ = conn.Close()
					}(conn)
				} else {
					logger.Log.Printf("[%s]: Listener.Accept: %v \n", rb.Name, err)
					break
				}
			}

			logger.Log.Printf("[%s]: RpcServer closed\n", rb.Name)
		}()

	} else {
		for {
			// create a connection
			conn, err := rb.Listener.Accept()
			if err == nil {

				//logger.Log.Println("Worker: got new conn")
				go func(conn net.Conn) {
					defer logger.HandleErrors()
					rpcSvc.ServeConn(conn)
					// close a connection
					_ = conn.Close()
				}(conn)
			} else {
				logger.Log.Printf("[%s]: Listener.Accept Error: %v \n", rb.Name, err)
				break
			}
		}
	}
}

// StopRPCServer  stops the master RPC server.
// This must be done through an RPC to avoid
// race conditions between the RPC server thread and the current thread.
func (rb *RpcBaseClass) StopRPCServer(addr, targetSvc string) {
	var reply entity.ShutdownReply

	logger.Log.Printf("[%s]: call %s\n", rb.Name, targetSvc)
	ok := client.Call(addr, rb.Network, targetSvc, new(struct{}), &reply)
	if ok == false {
		logger.Log.Printf("[%s]: call %s errored\n", rb.Name, targetSvc)
		return
	}

	logger.Log.Printf("[%s]: call %s successfully\n", rb.Name, targetSvc)
}
