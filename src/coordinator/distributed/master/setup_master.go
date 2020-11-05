package master

import (
	"coordinator/config"
	"fmt"
	"time"
)

func RunMaster(Proxy string, masterAddr, httpAddr string, qItem *config.QItem) (ms *Master) {
	fmt.Println("Master: address is :", masterAddr)
	ms = newMaster(Proxy, masterAddr, len(qItem.IPs))
	ms.startRPCServer()

	// set time out, no worker comes within 1 min, stop master
	time.AfterFunc(1*time.Minute, func() {
		if len(ms.workers) == 0 {
			ms.stopRPCServer()
		}
	})

	go ms.run(
		func() {
			ch := make(chan string, len(qItem.IPs))
			go ms.forwardRegistrations(ch)
			schedule(ch, Proxy, httpAddr, masterAddr, qItem)
		},
		func() {
			// stop both master and worker after finishing the job
			ms.stats = ms.killWorkers()
			ms.stopRPCServer()
		})
	return
}
