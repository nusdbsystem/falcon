package master

import (
	"coordinator/client"
	"coordinator/config"
	"coordinator/distributed/entitiy"
	"coordinator/logger"
	"strings"
	"sync"
)

func (this *Master) schedule(registerChan chan string, httpAddr string, qItem *config.QItem, taskType string) {
	logger.Do.Println("Scheduler: Begin to schedule")

	// checking if the ip of worker match the qItem
	this.Lock()
	this.allWorkerReady.Wait()
	this.Unlock()

	logger.Do.Println("Scheduler: All worker found")

	// extract ip from register chan to static slice
	var workerAddress []string

	for i := 0; i < len(qItem.IPs); i++ {
		logger.Do.Println("Scheduler: Reading from registerChan")
		addr := <-registerChan
		workerAddress = append(workerAddress, addr)
	}

	if taskType == config.TrainTaskType{
		this.scheduleWorker(httpAddr,config.Worker,qItem,workerAddress)

		// stop worker after finishing the job
		this.stats = this.killWorkers()

	}else if taskType == config.PredictTaskType{
		this.schedulePrediction(httpAddr,config.ModelService,qItem,workerAddress)
	}
}


func (this *Master) scheduleWorker(httpAddr, svcName string, qItem *config.QItem, workerAddress []string){
	// execute the task
	wg := sync.WaitGroup{}
	startTask := func(workerAddr string, args *entitiy.DoTaskArgs) {
		defer wg.Done()

		argAddr := entitiy.EncodeDoTaskArgs(args)
		var rep entitiy.DoTaskReply

		// todo, now master call worker.Dotask, worker start to train, until training is done, so how long it will wait? before timeout

		logger.Do.Printf("Scheduler: begin to call %s.DoTask\n", svcName)
		ok := client.Call(workerAddr, this.Proxy, svcName+".DoTask", argAddr, &rep)

		if !ok {
			logger.Do.Printf("Scheduler: %s.DoTask error\n", svcName)

			client.JobUpdateResInfo(
				httpAddr,
				"call Worker.DoTask error",
				"call Worker.DoTask error",
				"call Worker.DoTask error",
				qItem.JobId)
			client.JobUpdateStatus(httpAddr, config.JobFailed, qItem.JobId)
			return
		}

		errLen := 4096
		outLen := 4096
		errMsg := rep.ErrLogs[config.PreProcessing] + config.ModelTraining + rep.ErrLogs[config.ModelTraining]
		outMsg := rep.OutLogs[config.PreProcessing] + config.ModelTraining + rep.OutLogs[config.ModelTraining]
		if len(errMsg) < errLen {
			errLen = len(errMsg)
		}

		if len(outMsg) < outLen {
			outLen = len(outMsg)
		}

		logger.Do.Println("Scheduler: max length is", outLen, errLen)

		if rep.Killed == true {
		} else if rep.Errs[config.PreProcessing] != config.SubProcessNormal {
			// if pre-processing failed
			client.JobUpdateResInfo(
				httpAddr,
				rep.ErrLogs[config.PreProcessing],
				rep.OutLogs[config.PreProcessing],
				"PreProcessing Failed",
				qItem.JobId)
			client.JobUpdateStatus(httpAddr, config.JobFailed, qItem.JobId)

			// if pre-processing pass, but train failed
		} else if rep.Errs[config.ModelTraining] != config.SubProcessNormal {
			client.JobUpdateResInfo(
				httpAddr,
				errMsg[:errLen],
				outMsg[:outLen],
				"PreProcessing Passed, ModelTraining Failed",
				qItem.JobId)
			client.JobUpdateStatus(httpAddr, config.JobFailed, qItem.JobId)

			// if both train and process pass
		} else {
			client.JobUpdateResInfo(
				httpAddr,
				errMsg[:errLen],
				outMsg[:outLen],
				"PreProcessing Passed, ModelTraining Passed",
				qItem.JobId)
			client.JobUpdateStatus(httpAddr, config.JobSuccessful, qItem.JobId)
		}
	}

	for i, v := range qItem.IPs {
		args := new(entitiy.DoTaskArgs)
		args.IP = v
		args.PartyPath = qItem.PartyPath[i]
		args.TaskInfos = qItem.TaskInfos

		for _, workerAddr := range workerAddress {
			ip := strings.Split(workerAddr, ":")[0]

			// match using ip
			if ip == v {

				wg.Add(1)

				// execute the task
				go startTask(workerAddr, args)

			}
		}
	}

	// wait until all task is done
	wg.Wait()

	client.ModelUpdate(
		httpAddr,
		1,
		qItem.JobId)

	logger.Do.Println("Scheduler: Finish all task done")
}


func (this *Master) schedulePrediction(httpAddr, svcName string, qItem *config.QItem, workerAddress []string){

	// execute the task
	startTask := func(workerAddr string, args *entitiy.DoTaskArgs) {

		argAddr := entitiy.EncodeDoTaskArgs(args)
		var rep entitiy.DoTaskReply

		logger.Do.Printf("Scheduler: begin to call %s.DoTask\n", svcName)

		ok := client.Call(workerAddr, this.Proxy, svcName+".DoTask", argAddr, &rep)

		if !ok {
			logger.Do.Printf("Scheduler: %s.CreateService error\n", svcName)

			client.ModelServiceUpdateStatus(httpAddr, config.JobFailed, qItem.JobId)
			return
		}else{
			client.ModelServiceUpdateStatus(httpAddr, config.JobRunning, qItem.JobId)
		}
	}

	for i, v := range qItem.IPs {
		args := new(entitiy.DoTaskArgs)
		args.IP = v
		args.PartyPath = qItem.PartyPath[i]
		args.TaskInfos = qItem.TaskInfos

		for _, workerAddr := range workerAddress {
			ip := strings.Split(workerAddr, ":")[0]

			// match using ip
			if ip == v {

				// execute the task
				go startTask(workerAddr, args)

			}
		}
	}

	logger.Do.Println("Scheduler: Finish schedule all prediction task ")
}