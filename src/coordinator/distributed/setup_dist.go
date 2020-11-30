package distributed

import (
	c "coordinator/client"
	"coordinator/config"
	"coordinator/distributed/master"
	"coordinator/distributed/prediction"
	"coordinator/distributed/taskmanager"
	"coordinator/distributed/utils"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"fmt"
	"strconv"
	"sync"
)

func SetupDist(httpHost, httpPort string, qItem *config.QItem, taskType string) {
	logger.Do.Println("SetupDist: Lunching master")


	httpAddr := httpHost + ":" + httpPort
	port, e := utils.GetFreePort()

	if e != nil {
		logger.Do.Println("Get port Error")
		return
	}
	masterAddress := httpHost + ":" + fmt.Sprintf("%d", port)
	ms := master.RunMaster("tcp", masterAddress, httpAddr, qItem, taskType)

	if taskType == config.TrainTaskType{
		// update job's master address
		c.JobUpdateMaster(httpAddr, masterAddress, qItem.JobId)
	}

	for _, ip := range qItem.IPs {

		// Launch the worker

		// todo currently worker port is fixed, not a good design, change to dynamic later
		// maybe check table wit ip, and + port got from table also

		// send a request to http
		c.SetupWorker(ip+":"+config.ListenerPort, masterAddress, taskType)
	}

	// wait until job done
	ms.Wait()
}

func SetupWorker(httpHost string, masterAddress string) error {
	logger.Do.Println("SetupDist: Launch worker threads")

	wg := sync.WaitGroup{}

	// each listener only have 1 worker thread

	for i := 0; i < 1; i++ {
		port, e := utils.GetFreePort()
		if e != nil {
			logger.Do.Println("SetupDist: Launch worker Get port Error")
			return e
		}
		wg.Add(1)

		// worker address share the same host with listener server
		go worker.RunWorker(masterAddress, "tcp", httpHost, fmt.Sprintf("%d", port), &wg)
	}
	wg.Wait()
	return nil
}

func KillJob(masterAddr, Proxy string) {
	ok := c.Call(masterAddr, Proxy, "Master.KillJob", new(struct{}), new(struct{}))
	if ok == false {
		logger.Do.Println("Master: KillJob error")
		panic("Master: KillJob error")
	} else {
		logger.Do.Println("Master: KillJob Done")
	}
}

func SetupPredictionHelper(httpHost string, masterAddress string) error {

	// the masterAddress is the master thread address

	dir:=""
	stdIn := "input from keyboard"
	commend := ""
	args := []string{"/coordinator_server", "-svc predictor -addr 1" + masterAddress}
	var envs []string

	pm := taskmanager.InitSubProcessManager()
	pm.IsWait = false

	killed, e, el, ol := pm.ExecuteSubProc(dir, stdIn, commend, args, envs)
	logger.Do.Println(killed, e, el, ol)

	return nil
}


func SetupPrediction(httpHost, masterAddress string) {

	// the masterAddress is the master thread address

	logger.Do.Println("SetupDist: Lunching prediction svc")

	port, e := utils.GetFreePort()
	if e != nil {
		logger.Do.Println("SetupDist: Lunching worker Get port Error")
	}

	sPort := strconv.Itoa(port)

	prediction.RunPrediction(masterAddress, httpHost, sPort,"tcp")

}
