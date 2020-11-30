package base

import (
	"log"
	"net"
	"net/rpc"
	"sync"
)

type RpcBase struct {
	sync.Mutex
	Name 		string
	Proxy 		string
	Address  	string //  which is the ip+port address of worker
	Listener 	net.Listener

	// if the master is stopped
	IsStop 		bool
}

func (rb *RpcBase) InitRpc(Proxy, Address string) {
	rb.Proxy = Proxy
	rb.Address = Address
	rb.IsStop = false
}



func (rb *RpcBase) StartRPCServer(rpcSvc *rpc.Server, isBlocking bool){


	log.Printf("%s: listening on %s, %s \n", rb.Name, rb.Proxy, rb.Address)
	listener, e := net.Listen(rb.Proxy, rb.Address)

	if e != nil {
		log.Printf("%s: StartRPCServer error", rb.Name)
	}

	rb.Listener = listener

	if !isBlocking{
		// accept connection
		go func() {
			// define loop label used for break
			for {
				conn, err := rb.Listener.Accept()
				if err == nil {
					log.Printf("%s: got new conn", rb.Name)
					// user thread to process requests
					go func() {
						rpcSvc.ServeConn(conn)
						_ = conn.Close()
					}()
				} else {

					log.Printf("%s: RegistrationServer: Accept errored, %v \n",rb.Name, err)
					break
				}
			}
			log.Printf("%s: masterServer: done\n", rb.Name)
		}()

	}else{

		for {
			// create a connection
			conn, err := rb.Listener.Accept()
			if err == nil {
				log.Println("Worker: got new conn")
				go func() {
					rpcSvc.ServeConn(conn)
					// close a connection
					_ = conn.Close()
				}()
			} else {
				log.Println("Worker: got conn error", err)
				break
			}
		}
	}


}

