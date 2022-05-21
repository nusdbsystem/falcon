package controller

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/coordserver/models"
	"falcon_platform/jobmanager"
	"falcon_platform/logger"
	"fmt"
	"sync"
	"time"
)

// Driver is used to launch the jobManager async
// Driver will read from dslObj queue, once fond one, launch jobManager for the dslObj
type Driver struct {
	sync.Mutex

	isStop        chan bool
	isStopMonitor chan bool
	nConsumer     int
	curConsumers  int
}

// Init driver
func Init(nConsumer int) *Driver {
	// n worker
	ds := new(Driver)

	ds.isStop = make(chan bool, nConsumer)
	ds.isStopMonitor = make(chan bool, 1)

	ds.nConsumer = nConsumer
	// 0 consumers are running at beginning
	ds.curConsumers = 0

	return ds
}

// Consume listen to the queue, and fetch item and call jobManager to start the tasks
func (ds *Driver) Consume(consumerId int, jobDB *models.JobDB) {
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

			//logger.Log.Println("Consume:" +fmt.Sprintf("%d",consumerId)+" Getting job from the jobQueue...")

			if job, ok := cache.GetJobQueue().Pop(); ok {

				logger.Log.Println("Consume:" + fmt.Sprintf("%d", consumerId) + " Got from jobQueue")

				models.JobUpdateStatus(job.JobId, common.JobRunning, jobDB)

				// Launching the jobManager to allocate the job
				go func(job common.TrainJob) {
					defer logger.HandleErrors()
					jobmanager.RunJobManager(&job, common.TrainWorker)
				}(*job)
			}

		}
		time.Sleep(10 * time.Millisecond)
	}
	logger.Log.Println("Consumer stopped")
}

// StopConsumer Stop the consumer
func (ds *Driver) StopConsumer() {
	ds.isStop <- true
}

// MonitorConsumers Monitor if need to stop driver.consume loop
func (ds *Driver) MonitorConsumers(jobDB *models.JobDB) {

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
				go func(curConsumers int) {
					defer logger.HandleErrors()
					ds.Consume(curConsumers, jobDB)
				}(ds.curConsumers + 1)
				ds.curConsumers += 1
			}
			ds.Unlock()
		}
		time.Sleep(10 * time.Millisecond)
	}
	logger.Log.Println("Consumer Monitor stopped")

}

func (ds *Driver) StopMonitor() {
	ds.isStopMonitor <- true
}
