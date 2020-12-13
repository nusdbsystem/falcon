package master

import (
	"coordinator/cache"
	"coordinator/client"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/logger"
	"google.golang.org/protobuf/proto"
	"strconv"
	"strings"
	"sync"
	"time"
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
		addr := <-registerChan
		logger.Do.Printf("Scheduler: Reading worker %s from registerChan\n", addr)
		logger.Do.Printf("Scheduler: Reading corresponding listenerUrl %s from registerChan\n", qItem.IPs[i])
		workerAddress = append(workerAddress, addr)
	}


	if taskType == common.TrainExecutor{

		taskFunc := this.trainTaskHandler

		this.schedulerHelper(taskFunc, common.Worker, qItem, workerAddress)

		client.ModelUpdate(
			common.CoordSvcURLGlobal,
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
	logger.Do.Println("Scheduler: qitem.ips are ", qItem.IPs)
	netCfg := this.generateNetworkConfig(qItem.IPs)

	for i, v := range qItem.IPs {
		vip := strings.Split(v, ":")[0]

		args := new(entitiy.DoTaskArgs)
		args.IP = vip
		args.AssignID = qItem.PartyInfos[i].ID
		args.PartyNums = qItem.PartyNums
		args.JobFlType = qItem.JobFlType
		args.PartyInfo = qItem.PartyInfos[i]
		args.ExistingKey = qItem.ExistingKey
		args.TaskInfos = qItem.Tasks
		args.NetWorkFile = netCfg

		MaxSearchNumber := 100
		for len(workerAddress) > 0{

			if MaxSearchNumber <=0{
				panic("Max search Number reaches, Ip not Match Error ")
			}

			item := workerAddress[0]
			ip := strings.Split(item, ":")[0]
			workerAddress = workerAddress[1:]

			// match using ip
			if ip == vip {
				wg.Add(1)
				// execute the task
				go taskHandler(item,svcName,args,qItem.JobId,&wg)
				break
			}else{
				workerAddress = append(workerAddress, item)
				MaxSearchNumber--
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

	logger.Do.Printf("Scheduler: begin to call %s.DoTask of the worker: %s \n", svcName, workerAddr)
	ok := client.Call(workerAddr, this.Proxy, svcName+".DoTask", argAddr, &rep)

	if !ok {
		logger.Do.Printf("Scheduler: Master calling %s, DoTask error\n", workerAddr)

		client.JobUpdateResInfo(
			common.CoordSvcURLGlobal,
			"call Worker.DoTask error",
			"call Worker.DoTask error",
			"call Worker.DoTask error",
			JobId)
		client.JobUpdateStatus(common.CoordSvcURLGlobal, common.JobFailed, JobId)

		// todo define return or panic, how to handle panic if one panic, and others normal?

		time.Sleep(time.Minute*30)
		panic("trainTaskHandler error")
	}else{
		logger.Do.Printf("Scheduler: calling %s.DoTask of the worker: %s successful \n", svcName, workerAddr)
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
			common.CoordSvcURLGlobal,
			rep.ErrLogs[common.PreProcessing],
			rep.OutLogs[common.PreProcessing],
			"PreProcessing Failed",
			JobId)
		client.JobUpdateStatus(common.CoordSvcURLGlobal, common.JobFailed, JobId)

		// if pre-processing pass, but train failed
	} else if rep.Errs[common.ModelTraining] != common.SubProcessNormal {
		client.JobUpdateResInfo(
			common.CoordSvcURLGlobal,
			errMsg[:errLen],
			outMsg[:outLen],
			"PreProcessing Passed, ModelTraining Failed",
			JobId)
		client.JobUpdateStatus(common.CoordSvcURLGlobal, common.JobFailed, JobId)

		// if both train and process pass
	} else {
		client.JobUpdateResInfo(
			common.CoordSvcURLGlobal,
			errMsg[:errLen],
			outMsg[:outLen],
			"PreProcessing Passed, ModelTraining Passed",
			JobId)
		client.JobUpdateStatus(common.CoordSvcURLGlobal, common.JobSuccessful, JobId)
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

		client.ModelServiceUpdateStatus(common.CoordSvcURLGlobal, common.JobFailed, JobId)
		return
	}else{
		client.ModelServiceUpdateStatus(common.CoordSvcURLGlobal, common.JobRunning, JobId)
	}
}


func (this *Master) generateNetworkConfig(urls []string) string {

	partyNums := len(urls)
	var ips []string
	for _, v := range urls {
		ips = append(ips, strings.Split(v, ":")[0])
	}

	cfg := common.NetworkConfig{
		Ips:    ips,
		PortArrays:  []*common.PortArray{},
	}

	// for each ip address
	for i:=0; i<partyNums; i++{
		var ports []int32
		//generate n ports
		for i:=0; i<partyNums; i++{
			port := c.GetFreePort(common.CoordSvcURLGlobal)
			pint, _ := strconv.Atoi(port)
			ports = append(ports, int32(pint))
		}
		p := &common.PortArray{Ports: ports}
		cfg.PortArrays = append(cfg.PortArrays, p)
	}

	out, err := proto.Marshal(&cfg)
	if err != nil {
		logger.Do.Println("Scheduler: Generate NetworkCfg failed ", err)
		panic(err)
	}
	logger.Do.Println("Scheduler: Generate NetworkCfg successfully")

	return string(out)

}