package master

import (
	"coordinator/cache"
	"coordinator/client"
	"coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/logger"
	"strings"
	"sync"
)

type taskHandler func(workerAddr,svcName string, args *entitiy.DoTaskArgs, JobId uint, wg *sync.WaitGroup)

func (this *Master) schedule(registerChan chan string, qItem *cache.QItem, taskType string) {
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


	if taskType == common.TrainExecutor{

		taskFunc := this.trainTaskHandler

		this.schedulerHelper(taskFunc, common.Worker,qItem,workerAddress)

		client.ModelUpdate(
			common.CoordURLGlobal,
			1,
			qItem.JobId)

		// stop worker after finishing the job
		this.stats = this.killWorkers()

	}else if taskType == common.PredictExecutor{

		taskFunc := this.predictTaskHandler

		this.schedulerHelper(taskFunc, common.ModelService,qItem,workerAddress)

	}
}


func (this *Master) schedulerHelper(

	taskHandler taskHandler,

	svcName string,
	qItem *cache.QItem,
	workerAddress []string){

	wg := sync.WaitGroup{}

	// execute the task
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
				go taskHandler(workerAddr,svcName,args,qItem.JobId,&wg)

			}
		}
	}
	// wait all job has been finished successfully
	wg.Wait()

	logger.Do.Println("Scheduler: Finish schedule all prediction task ")
}


func (this *Master) trainTaskHandler(
	workerAddr,
	svcName string,
	args *entitiy.DoTaskArgs,
	JobId uint,
	wg *sync.WaitGroup){

	defer wg.Done()

	argAddr := entitiy.EncodeDoTaskArgs(args)
	var rep entitiy.DoTaskReply

	// todo, now master call worker.Dotask, worker start to train, until training is done, so how long it will wait? before timeout

	logger.Do.Printf("Scheduler: begin to call %s.DoTask\n", svcName)
	ok := client.Call(workerAddr, this.Proxy, svcName+".DoTask", argAddr, &rep)

	if !ok {
		logger.Do.Printf("Scheduler: %s.DoTask error\n", svcName)

		client.JobUpdateResInfo(
			common.CoordURLGlobal,
			"call Worker.DoTask error",
			"call Worker.DoTask error",
			"call Worker.DoTask error",
			JobId)
		client.JobUpdateStatus(common.CoordURLGlobal, common.JobFailed, JobId)
		return
	}

	errLen := 4096
	outLen := 4096
	errMsg := rep.ErrLogs[common.PreProcessing] + common.ModelTraining + rep.ErrLogs[common.ModelTraining]
	outMsg := rep.OutLogs[common.PreProcessing] + common.ModelTraining + rep.OutLogs[common.ModelTraining]
	if len(errMsg) < errLen {
		errLen = len(errMsg)
	}

	if len(outMsg) < outLen {
		outLen = len(outMsg)
	}

	logger.Do.Println("Scheduler: max length is", outLen, errLen)

	if rep.Killed == true {
	} else if rep.Errs[common.PreProcessing] != common.SubProcessNormal {
		// if pre-processing failed
		client.JobUpdateResInfo(
			common.CoordURLGlobal,
			rep.ErrLogs[common.PreProcessing],
			rep.OutLogs[common.PreProcessing],
			"PreProcessing Failed",
			JobId)
		client.JobUpdateStatus(common.CoordURLGlobal, common.JobFailed, JobId)

		// if pre-processing pass, but train failed
	} else if rep.Errs[common.ModelTraining] != common.SubProcessNormal {
		client.JobUpdateResInfo(
			common.CoordURLGlobal,
			errMsg[:errLen],
			outMsg[:outLen],
			"PreProcessing Passed, ModelTraining Failed",
			JobId)
		client.JobUpdateStatus(common.CoordURLGlobal, common.JobFailed, JobId)

		// if both train and process pass
	} else {
		client.JobUpdateResInfo(
			common.CoordURLGlobal,
			errMsg[:errLen],
			outMsg[:outLen],
			"PreProcessing Passed, ModelTraining Passed",
			JobId)
		client.JobUpdateStatus(common.CoordURLGlobal, common.JobSuccessful, JobId)
	}
}


func (this *Master) predictTaskHandler(
	workerAddr,
	svcName string,
	args *entitiy.DoTaskArgs,
	JobId uint,
	wg *sync.WaitGroup){

	defer wg.Done()

	argAddr := entitiy.EncodeDoTaskArgs(args)
	var rep entitiy.DoTaskReply

	logger.Do.Printf("Scheduler: begin to call %s.DoTask\n", svcName)

	ok := client.Call(workerAddr, this.Proxy, svcName+".DoTask", argAddr, &rep)

	if !ok {
		logger.Do.Printf("Scheduler: %s.CreateService error\n", svcName)

		client.ModelServiceUpdateStatus(common.CoordURLGlobal, common.JobFailed, JobId)
		return
	}else{
		client.ModelServiceUpdateStatus(common.CoordURLGlobal, common.JobRunning, JobId)
	}
}