package master

import (
	"coordinator/distributed/entitiy"
	"coordinator/distributed/utils"
	"fmt"
	"net"
	"net/rpc"
)

func (this *Master) startRPCServer() {
	// register master
	rpcs := rpc.NewServer()
	rerr := rpcs.Register(this)
	if rerr != nil {
		fmt.Println("Master: start Error, ", rerr)
		return
	}

	fmt.Println("Master: listening on", this.Proxy, this.Address)

	listener, e := net.Listen(this.Proxy, this.Address)

	if e != nil {
		fmt.Println("Master: StartRPCServer error")
	}
	this.l = listener
	// accept connection
	go func() {
		// define loop label used for break
		for {
			conn, err := this.l.Accept()
			if err == nil {
				// user thread to process requests
				go func() {
					rpcs.ServeConn(conn)
					_ = conn.Close()
				}()
			} else {
				fmt.Printf("Master: RegistrationServer: Accept errored, %v \n", err)
				break
			}
		}
		fmt.Printf("Maseter: masterServer: done\n")
	}()
}

// stopRPCServer stops the master RPC server.
// This must be done through an RPC to avoid
// race conditions between the RPC server thread and the current thread.
func (this *Master) stopRPCServer() {
	var reply entitiy.ShutdownReply

	fmt.Println("Master: begin to call Master.Shutdown")
	ok := utils.Call(this.Address, this.Proxy, "Master.Shutdown", new(struct{}), &reply)
	if ok == false {
		fmt.Printf("Master: Cleanup: RPC %s error\n", this.Address)
	}
	fmt.Println("Master: cleanupRegistration: done")
}

// Shutdown is an RPC method that shuts down the Master's RPC server.
// for rpc method, must be public method, only 2 params, second one must be pointer,return err type
func (this *Master) Shutdown(_, _ *struct{}) error {
	fmt.Println("Master: Shutdown: registration server")
	_ = this.l.Close() // causes the Accept to fail, then break out the accetp loop
	return nil
}
