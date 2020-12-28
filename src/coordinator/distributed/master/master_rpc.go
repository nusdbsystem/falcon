package master

import (
	"coordinator/cache"
	"coordinator/client"
	"coordinator/common"
	"coordinator/logger"
	"net/rpc"
	"time"
)

func RunMaster(masterAddr string, qItem *cache.QItem, workerType string) (master *Master) {
	// launch 2 thread, one is rpc server, another is scheduler, once got party info, assign work

	master = newMaster(masterAddr, len(qItem.AddrList))
	master.workerType = workerType
	master.reset()

	// thread 0, heartBeat
	go master.eventLoop()

	rpcServer := rpc.NewServer()
	logger.Do.Println("[master_rpc/RunMaster] rpcServer ready, now register with master")
	err := rpcServer.Register(master)
	// NOTE TODO: the rpc native Register() method will produce warning in console:
	// rpc.Register: method ...; needs exactly three
	// reply type of method ... is not a pointer: "bool"
	if err != nil {
		logger.Do.Println("rpcServer Register master Error", err)
		return
	}

	logger.Do.Println("[master_rpc/RunMaster] rpcServer registered with master")

	// thread 1
	go master.forwardRegistrations(qItem)

	// thread 2
	// launch a rpc server thread to process the requests.
	master.StartRPCServer(rpcServer, false)

	scheduler := func() string {
		js := master.schedule(qItem)
		return js
	}

	var updateStatus func(jsonString string)
	var finish func()

	if workerType == common.TrainWorker {

		updateStatus = func(jsonString string) {
			client.JobUpdateResInfo(common.CoordAddr, "", string(jsonString), "", qItem.JobId)
			client.JobUpdateStatus(common.CoordAddr, master.jobStatus, qItem.JobId)
			client.ModelUpdate(common.CoordAddr, 1, qItem.JobId)
		}

		finish = func() {
			// stop worker after finishing the job
			master.killWorkers()
			// stop other related threads
			// close eventLoop and forwardRegistrations
			master.Cancel()
			// stop both master after finishing the job
			master.StopRPCServer(master.Addr, "Master.Shutdown")
		}

	} else if workerType == common.InferenceWorker {

		updateStatus = func(jsonString string) {
			client.InferenceUpdateStatus(common.CoordAddr, master.jobStatus, qItem.JobId)
		}

		finish = func() {
			// stop other related threads
			// close eventLoop and forwardRegistrations
			master.Cancel()
			// stop both master after finishing the job
			master.StopRPCServer(master.Addr, "Master.Shutdown")
		}
	}

	// set time out, no worker comes within 1 min, stop master
	time.AfterFunc(1*time.Minute, func() {
		if len(master.workers) < master.workerNum {

			logger.Do.Printf("Master: Wait for 1 Min, No enough worker come, stop, required %d, got %d ",
				master.workerNum,
				len(master.workers),
			)

			finish()
		}
	})

	// thread 3
	// launch a thread to process then do the scheduling.
	go master.run(scheduler, updateStatus, finish)

	return
}
