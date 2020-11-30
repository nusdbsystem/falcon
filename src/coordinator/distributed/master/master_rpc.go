package master

import (
	"coordinator/config"
	"log"
	"net/rpc"
	"time"
)

func RunMaster(Proxy string, masterAddr, httpAddr string, qItem *config.QItem) (ms *Master) {
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

	ms.StartRPCServer(rpcSvc, "Master", false)

	// set time out, no worker comes within 1 min, stop master
	time.AfterFunc(1*time.Minute, func() {
		if len(ms.workers) == 0 {
			ms.stopRPCServer()
		}
	})

	go ms.run(
		func() {
			ch := make(chan string, len(qItem.IPs))
			go ms.forwardRegistrations(ch, qItem)
			ms.schedule(ch, httpAddr, qItem,config.TrainTaskType)
		},
		func() {
			// stop both master and worker after finishing the job
			ms.stats = ms.killWorkers()
			ms.stopRPCServer()

			// stop other related threads

			ms.Lock()
			ms.IsStop = true
			ms.Unlock()

		})
	return
}