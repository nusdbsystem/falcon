package master

import (
	"coordinator/client"
	"coordinator/common"
	"coordinator/logger"
	"fmt"
	"time"
)

func (master *Master) eventLoop() {

loop:
	for {
		select {
		case <-master.Ctx.Done():
			logger.Do.Printf("Master: %s quit eventLoop \n", master.Port)
			break loop

		default:
			if !master.checkWorker() {

				// if not worker found, wait here
				master.Lock()
				master.beginCountDown.Wait()
				master.Unlock()

			} else {

				master.Lock()
				elapseTime := time.Now().UnixNano() - master.lastSendTime
				fmt.Printf("Master: CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))

				if int(elapseTime/int64(time.Millisecond)) >= master.heartbeatTimeout {

					master.Unlock()
					fmt.Println("Master: Timeout, server begin to send heart beat to worker")
					master.broadcastHeartbeat()

				} else {
					master.Unlock()
				}
			}
		}
		time.Sleep(time.Millisecond * common.MasterTimeout / 5)
	}
}

// boardcast heartbeat to current workers in worker list
func (master *Master) broadcastHeartbeat() {
	// update lastSendTime
	master.reset()

	for _, worker := range master.workers {

		ok := client.Call(worker, master.Proxy, master.workerType+".ResetTime", new(struct{}), new(struct{}))
		if ok == false {
			logger.Do.Printf("Master: RPC %s send heartbeat error\n", worker)
		}
	}
}

func (master *Master) reset() {
	master.Lock()
	master.lastSendTime = time.Now().UnixNano()
	master.Unlock()

}

func (master *Master) checkWorker() bool {

	master.Lock()

	if len(master.workers) > 0 {
		master.foundWorker = true
	} else {
		master.foundWorker = false
	}

	master.Unlock()
	return master.foundWorker
}
