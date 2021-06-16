package master

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"strings"
	"time"
)

func (master *Master) heartBeat() {

loop:
	for {
		select {
		case <-master.Ctx.Done():

			logger.Log.Printf("Master: Thread-1 %s quit eventLoop \n", master.Port)
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

				//fmt.Printf("Master: CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))

				if int(elapseTime/int64(time.Millisecond)) >= master.heartbeatTimeout {

					master.Unlock()
					//fmt.Println("Master: Timeout, server send heart beat to worker")
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

	// logger.Log.Println("[broadcastHeartbeat]...")
	// update lastSendTime
	master.reset()

	for _, RegisteredWorker := range master.workers {
		// RegisteredWorker is IP:Port:PartyID

		// logger.Log.Println("RegisteredWorker = ", RegisteredWorker)
		// Addr = IP:Port
		RegisteredWorkerAddr := strings.Join(strings.Split(RegisteredWorker, ":")[:2], ":")
		// logger.Log.Println("RegisteredWorkerAddr = ", RegisteredWorkerAddr)

		ok := client.Call(RegisteredWorkerAddr, master.Network, master.workerType+".ResetTime", new(struct{}), new(struct{}))
		if ok == false {
			logger.Log.Printf("Master: RPC %s send heartbeat error\n", RegisteredWorkerAddr)
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
