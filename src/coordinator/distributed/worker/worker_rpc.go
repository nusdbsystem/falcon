package worker

import (
	"coordinator/distributed/taskmanager"
	"fmt"
	"net"
	"net/rpc"
	"sync"
)

func RunWorker(masterAddress, workerProxy, workerHost, workerPort string, wg *sync.WaitGroup) {

	workerAddress := workerHost + ":" + workerPort

	wk := new(Worker)
	wk.Proxy = workerProxy
	wk.name = workerAddress

	// the lock needs to pass to multi funcs, must create a instance
	wk.pm = taskmanager.InitSubProcessManager()
	wk.taskFinish = make(chan bool)

	rpcs := rpc.NewServer()

	_ = rpcs.Register(wk)
	//_ = os.Remove(workerName)
	listener, err := net.Listen(workerProxy, workerAddress)

	if err != nil {
		fmt.Println("Worker: runWorker: ", workerAddress, " error: ", err)
	}

	wk.l = listener
	fmt.Println("Worker: register to ", masterAddress)
	wk.register(masterAddress)

	for {
		conn, err := wk.l.Accept()
		if err == nil {
			fmt.Println("Worker: got new conn")
			go rpcs.ServeConn(conn)
		} else {
			fmt.Println("Worker: got conn error", err)
			break
		}
	}
	//_ = wk.l.Close()
	fmt.Println("Worker: ", workerAddress, "runWorker exit")
	wg.Done()
}
