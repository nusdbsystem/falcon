package master

import (
	"coordinator/cache"
	"coordinator/logger"
	"net/rpc"
	"time"
)

func RunMaster(masterAddr string, qItem *cache.QItem, taskType string) (ms *Master) {
	/**
	 * @Author
	 * @Description
	 * @Date 5:09 下午 4/12/20
	 * @Param  launch 2 thread, one is rpc server, another is scheduler, once got party info, assign work
	 * @return
	 **/
	logger.Do.Println("Master: addr is :", masterAddr)
	ms = newMaster(masterAddr, len(qItem.IPs))

	ms.reset()
	go ms.eventLoop()

	rpcSvc := rpc.NewServer()
	err := rpcSvc.Register(ms)
	if err!= nil{
		logger.Do.Printf("%s: start Error \n", "Master")
		return
	}

	// thread 1
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
		ms.schedule(ch, qItem, taskType)
	}

	clean := func() {
		// stop both master after finishing the job
		ms.stopRPCServer()
		// stop other related threads
		ms.Lock()
		ms.IsStop = true
		ms.Unlock()
	}

	// thread 2
	// launch a thread to process the do the scheduling.
	go ms.run(scheduler,clean)

	return
}