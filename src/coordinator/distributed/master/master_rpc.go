package master

import (
	"coordinator/config"
	"log"
	"net/rpc"
	"time"
)

func RunMaster(Proxy string, masterAddr, httpAddr string, qItem *config.QItem, taskType string) (ms *Master) {
	log.Println("Master: address is :", masterAddr)
	ms = newMaster(Proxy, masterAddr, len(qItem.IPs))

	ms.reset()
	go ms.eventLoop()

	rpcSvc := rpc.NewServer()
	err := rpcSvc.Register(ms)
	if err!= nil{
		log.Printf("%s: start Error \n", "Master")
		return
	}

	// launch a rpc server thread to process the requests.
	ms.StartRPCServer(rpcSvc, false)

	// set time out, no worker comes within 1 min, stop master
	time.AfterFunc(1*time.Minute, func() {
		if len(ms.workers) == 0 {
			ms.stopRPCServer()
		}
	})


	scheduler:= func() {
		ch := make(chan string, len(qItem.IPs))
		go ms.forwardRegistrations(ch, qItem)
		ms.schedule(ch, httpAddr, qItem, taskType)
	}

	clean := func() {
		// stop both master after finishing the job
		ms.stopRPCServer()
		// stop other related threads
		ms.Lock()
		ms.IsStop = true
		ms.Unlock()
	}

	// launch a thread to process the do the scheduling.
	go ms.run(scheduler,clean)

	return
}