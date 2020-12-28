package master

import (
	"coordinator/client"
	"coordinator/common"
	"coordinator/logger"
	"fmt"
	"time"
)

func (this *Master) eventLoop() {

loop:
	for {
		select {
		case <-this.Ctx.Done():
			logger.Do.Printf("Master: %s quit eventLoop \n", this.Port)
			break loop

		default:
			if !this.checkWorker() {

				// if not worker found, wait here
				this.Lock()
				this.beginCountDown.Wait()
				this.Unlock()

			} else {

				this.Lock()
				elapseTime := time.Now().UnixNano() - this.lastSendTime
				fmt.Printf("Master: CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))

				if int(elapseTime/int64(time.Millisecond)) >= this.heartbeatTimeout {

					this.Unlock()
					fmt.Println("Master: Timeout, server begin to send heart beat to worker")
					this.broadcastHeartbeat()

				} else {
					this.Unlock()
				}
			}
		}
		time.Sleep(time.Millisecond * common.MasterTimeout / 5)
	}
}

func (this *Master) broadcastHeartbeat() {
	// update lastSendTime
	this.reset()
	/**
	 * @Author
	 * @Description boardCast hb to current workers in worker list
	 * @Date 7:07 下午 13/12/20
	 * @Param
	 * @return
	 **/
	for _, worker := range this.workers {

		ok := client.Call(worker, this.Proxy, this.workerType+".ResetTime", new(struct{}), new(struct{}))
		if ok == false {
			logger.Do.Printf("Master: RPC %s send heartbeat error\n", worker)
		}
	}
}

func (this *Master) reset() {
	this.Lock()
	this.lastSendTime = time.Now().UnixNano()
	this.Unlock()

}

func (this *Master) checkWorker() bool {

	this.Lock()

	if len(this.workers) > 0 {
		this.foundWorker = true
	} else {
		this.foundWorker = false
	}

	this.Unlock()
	return this.foundWorker
}
