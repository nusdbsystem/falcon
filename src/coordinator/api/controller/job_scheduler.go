package controller

import (
	"coordinator/api/entity"
	"coordinator/api/models"
	"coordinator/config"
	dist "coordinator/distributed"
	"coordinator/logger"
	"fmt"
	"sync"
	"time"
)

type DslScheduler struct {
	sync.Mutex

	isStop        chan bool
	isStopMonitor chan bool
	httpHost      string
	httpPort      string
	nConsumer     int
	curConsumers  int
}

func Init(httpHost, httpPort string, nConsumer int) *DslScheduler {
	// n worker
	ds := new(DslScheduler)

	ds.isStop = make(chan bool, nConsumer)
	ds.isStopMonitor = make(chan bool, 1)

	ds.httpHost = httpHost
	ds.httpPort = httpPort

	ds.nConsumer = nConsumer
	// 0 consumers are running at beginning
	ds.curConsumers = 0

	return ds
}

func (ds *DslScheduler) Consume(consumerId int) {
	defer func() {
		err := recover()
		logger.Do.Println("Consume: Error of this thread, ", consumerId, err)
		ds.Lock()
		ds.curConsumers -= 1
		ds.Unlock()

	}()

loop:
	for {
		select {
		case stop := <-ds.isStop:
			logger.Do.Println("Consume:" + fmt.Sprintf("%d", consumerId) + " Get from isStop")
			if stop == true {
				logger.Do.Println("Consume: Stop consuming")
				break loop
			}
		default:

			//logger.Do.Println("Consume:" +fmt.Sprintf("%d",consumerId)+" Getting job from the queue...")

			if qItem, ok := entity.JobQueue.Pop(); ok {

				logger.Do.Println("Consume:" + fmt.Sprintf("%d", consumerId) + " Got from queue")

				models.JobUpdateStatus(qItem.JobId, config.JobRunning)
				// lunching the master
				go dist.SetupDist(ds.httpHost, ds.httpPort, qItem, config.TrainTaskType)
			}

		}
		time.Sleep(10 * time.Millisecond)
	}
	logger.Do.Println("Consumer stopped")
}

func (ds *DslScheduler) StopConsumer() {
	ds.isStop <- true
}

func (ds *DslScheduler) MonitorConsumers() {

loop:
	for {
		select {
		case stop := <-ds.isStopMonitor:
			if stop == true {
				logger.Do.Println("Consume: Stopping monitor")
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
	logger.Do.Println("Consumer Monitor stopped")

}

func (ds *DslScheduler) StopMonitor() {
	ds.isStopMonitor <- true
}
