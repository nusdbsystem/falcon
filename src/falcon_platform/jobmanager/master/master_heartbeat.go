package master

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"time"
)

func (master *Master) heartBeat() {

loop:
	for {
		select {
		case <-master.Ctx.Done():

			logger.Log.Println("[Master.Heartbeat] Thread-1 heartBeatLoop exit")
			break loop

		default:
			if !master.checkWorker() {

				// if not worker found, wait here
				master.Lock()
				master.beginCountDown.Wait()
				master.Unlock()

			} else {
				// worker found, send heartbeat
				master.Lock()
				elapseTime := time.Now().UnixNano() - master.lastSendTime

				//fmt.Printf("[Master.Heartbeat] CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))

				if int(elapseTime/int64(time.Millisecond)) >= master.heartbeatTimeout {

					master.Unlock()
					//fmt.Println("[Master.Heartbeat] Timeout, server send heart beat to worker")
					master.broadcastHeartbeat()

				} else {
					master.Unlock()
				}
			}
		}
		time.Sleep(time.Millisecond * common.MasterTimeout / 5)
	}
}

// board cast heartbeat to current workers in worker list
func (master *Master) broadcastHeartbeat() {

	// logger.Log.Println("[broadcastHeartbeat]...")
	// update lastSendTime
	master.reset()

	for _, worker := range master.workers {
		ok := client.Call(worker.Addr, master.Network, master.workerType+".ResetTime", new(struct{}), new(struct{}))
		if ok == false {
			logger.Log.Printf("[Master.Heartbeat] RPC %s send heartbeat error\n", worker.Addr)
		}
	}
}

func (master *Master) reset() {
	master.Lock()
	master.lastSendTime = time.Now().UnixNano()
	master.Unlock()

}

func (master *Master) checkWorker() bool {
	// if at least one worker found, master send hearbeat,
	// otherwise, wait until one worker found..
	master.Lock()

	if len(master.workers) > 0 {
		master.foundWorker = true
	} else {
		master.foundWorker = false
	}

	master.Unlock()
	return master.foundWorker
}
