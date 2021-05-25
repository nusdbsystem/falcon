package controller

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/coordserver/models"
	dist "falcon_platform/distributed"
	"falcon_platform/logger"
	"fmt"
	"sync"
	"time"
)

type JobScheduler struct {
	sync.Mutex

	isStop        chan bool
	isStopMonitor chan bool
	nConsumer     int
	curConsumers  int
}

func Init(nConsumer int) *JobScheduler {
	// n worker
	ds := new(JobScheduler)

	ds.isStop = make(chan bool, nConsumer)
	ds.isStopMonitor = make(chan bool, 1)

	ds.nConsumer = nConsumer
	// 0 consumers are running at beginning
	ds.curConsumers = 0

	return ds
}

func (ds *JobScheduler) Consume(consumerId int) {
	defer func() {
		err := recover()
		logger.Log.Println("Consume: Error of this thread, ", consumerId, err)
		ds.Lock()
		ds.curConsumers -= 1
		ds.Unlock()

	}()

loop:
	for {
		select {
		case stop := <-ds.isStop:
			logger.Log.Println("Consume:" + fmt.Sprintf("%d", consumerId) + " Get from isStop")
			if stop == true {
				logger.Log.Println("Consume: Stop consuming")
				break loop
			}
		default:

			//logger.Log.Println("Consume:" +fmt.Sprintf("%d",consumerId)+" Getting job from the queue...")

			if qItem, ok := cache.JobQueue.Pop(); ok {

				logger.Log.Println("Consume:" + fmt.Sprintf("%d", consumerId) + " Got from queue")

				models.JobUpdateStatus(qItem.JobId, common.JobRunning)

				// Launching the master
				go func() {
					defer logger.HandleErrors()
					dist.SetupDist(qItem, common.TrainWorker)
				}()
			}

		}
		time.Sleep(10 * time.Millisecond)
	}
	logger.Log.Println("Consumer stopped")
}

func (ds *JobScheduler) StopConsumer() {
	ds.isStop <- true
}

func (ds *JobScheduler) MonitorConsumers() {

loop:
	for {
		select {
		case stop := <-ds.isStopMonitor:
			if stop == true {
				logger.Log.Println("Consume: Stopping monitor")
				break loop
			}
		default:
			ds.Lock()
			if ds.curConsumers < ds.nConsumer {
				go ds.Consume(ds.curConsumers + 1)
				ds.curConsumers += 1
			}
			ds.Unlock()
		}
		time.Sleep(10 * time.Millisecond)
	}
	logger.Log.Println("Consumer Monitor stopped")

}

func (ds *JobScheduler) StopMonitor() {
	ds.isStopMonitor <- true
}
