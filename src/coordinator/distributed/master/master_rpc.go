package master

import (
	"coordinator/cache"
	"coordinator/client"
	"coordinator/common"
	"coordinator/logger"
	"net/rpc"
	"time"
)

func RunMaster(masterAddr string, qItem *cache.QItem, workerType string) (ms *Master) {
	/**
	 * @Author
	 * @Description
	 * @Date 5:09 下午 4/12/20
	 * @Param  launch 2 thread, one is rpc server, another is scheduler, once got party info, assign work
	 * @return
	 **/
	ms = newMaster(masterAddr, len(qItem.AddrList))
	ms.workerType = workerType
	ms.reset()

	// thread 0, heartBeat
	go ms.eventLoop()

	rpcSvc := rpc.NewServer()
	err := rpcSvc.Register(ms)
	if err!= nil{
		logger.Do.Printf("%s: start Error \n", "Master")
		return
	}

	// thread 1
	go ms.forwardRegistrations(qItem)

	// thread 2
	// launch a rpc server thread to process the requests.
	ms.StartRPCServer(rpcSvc, false)

	scheduler:= func() string{
		js := ms.schedule(qItem)
		return js
	}

	var updateStatus func(jsonString string)
	var finish func()

	if workerType == common.TrainWorker{

		updateStatus = func(jsonString string){
			client.JobUpdateResInfo(common.CoordAddr, "", string(jsonString), "", qItem.JobId)
			client.JobUpdateStatus(common.CoordAddr, ms.jobStatus, qItem.JobId)
			client.ModelUpdate(common.CoordAddr, 1, qItem.JobId)
		}

		finish = func() {
			// stop worker after finishing the job
			ms.killWorkers()
			// stop other related threads
			// close eventLoop and forwardRegistrations
			ms.Cancel()
			// stop both master after finishing the job
			ms.StopRPCServer(ms.Addr,"Master.Shutdown")
		}

	}else if workerType == common.InferenceWorker{

		updateStatus = func(jsonString string){
			client.InferenceUpdateStatus(common.CoordAddr, ms.jobStatus, qItem.JobId)
		}

		finish = func() {
			// stop other related threads
			// close eventLoop and forwardRegistrations
			ms.Cancel()
			// stop both master after finishing the job
			ms.StopRPCServer(ms.Addr,"Master.Shutdown")
		}
	}


	// set time out, no worker comes within 1 min, stop master
	time.AfterFunc(1*time.Minute, func() {
		if len(ms.workers) < ms.workerNum {

			logger.Do.Printf("Master: Wait for 1 Min, No enough worker come, stop, required %d, got %d ",
				ms.workerNum,
				len(ms.workers),
				)

			finish()
		}
	})

	// thread 3
	// launch a thread to process the do the scheduling.
	go ms.run(scheduler, updateStatus, finish)

	return
}