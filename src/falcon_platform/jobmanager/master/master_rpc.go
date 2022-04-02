package master

import (
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"math/rand"
	"net/rpc"
	"time"
)

func RunMaster(masterAddr string, dslOjb *cache.DslObj, workerType string, stageName common.FalconStage, stageNameLog string) (master *Master) {
	// launch 4 thread,
	// 1. heartbeat loop, stopped by master.Cancel()
	// 2. waiting for worker register, stopped by master.Cancel()
	// 3. rpc server, used to get requests from worker, stopped by master.StopRPCServer
	// 4. scheduling process, call finish to stop above threads

	logFileName := common.LogPath + "/master-" + stageNameLog + "-" + fmt.Sprintf("%d", rand.Intn(90000)) + ".log"
	master = newMaster(masterAddr, dslOjb.PartyNums)
	master.Logger, master.LogFile = logger.GetLogger(logFileName)
	master.workerType = workerType
	master.reset()

	// thread 1, heartBeat
	go func() {
		defer logger.HandleErrors()
		master.heartBeat()
	}()

	rpcServer := rpc.NewServer()
	err := rpcServer.Register(master)
	// NOTE the rpc native Register() method will produce warning in console:
	// rpc.Register: method ...; needs exactly three
	// reply type of method ... is not a pointer: "bool"
	// all those can be ignored
	if err != nil {
		master.Logger.Println("rpcServer Register master Error", err)
		return
	}

	// thread 2
	go func() {
		defer logger.HandleErrors()
		master.forwardRegistrations()
	}()

	// thread 3
	// launch a rpc server thread to process the requests.
	master.StartRPCServer(rpcServer, false)

	// define 3 functions, called in master.run
	dispatcher := func() {
		master.dispatch(dslOjb, stageName)
	}
	finish := func() {
		// stop master after finishing the job
		master.StopRPCServer(master.Addr, "Master.Shutdown")
		// stop other related threads
		// close heartBeat and forwardRegistrations
		master.Clear()
	}

	var updateStatus func(jsonString string)
	if workerType == common.TrainWorker {
		updateStatus = func(jsonString string) {
			// call coordinator to update status
			client.JobUpdateResInfo(common.CoordAddr, "", jsonString, "", dslOjb.JobId)
			master.jobStatusLock.Lock()
			jobStatus := master.jobStatus
			master.jobStatusLock.Unlock()
			client.JobUpdateStatus(common.CoordAddr, jobStatus, dslOjb.JobId)
			client.ModelUpdate(common.CoordAddr, 1, dslOjb.JobId)
		}
	} else if workerType == common.InferenceWorker {
		updateStatus = func(jsonString string) {
			client.InferenceUpdateStatus(common.CoordAddr, master.jobStatus, dslOjb.JobId)
		}
	}

	// set time out, no worker comes within 1 min, stop master
	time.AfterFunc(100*time.Minute, func() {
		master.Lock()
		if len(master.workers) < master.workerNum {
			master.Logger.Printf("Master: Wait for 100 Min, No enough worker come, stop, required %d, got %d ",
				master.workerNum,
				len(master.workers),
			)
			master.Unlock()

			finish()
		} else {
			master.Unlock()
		}

	})

	// thread 4
	// launch a thread to process then do the scheduling.
	go func() {
		defer logger.HandleErrors()
		master.run(dispatcher, updateStatus, finish)
	}()

	return
}
