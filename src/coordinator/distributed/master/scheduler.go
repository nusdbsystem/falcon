package master

import (
	"context"
	"coordinator/cache"
	"coordinator/client"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/logger"
	"encoding/json"
	"fmt"
	"google.golang.org/protobuf/proto"
	"strconv"
	"strings"
	"sync"
	"time"
)

type taskHandler func(
	workerAddr,svcName string,
	args *entitiy.DoTaskArgs,
	wg *sync.WaitGroup,
	trainStatuses *[]entitiy.DoTaskReply)


func (this *Master) schedule(qItem *cache.QItem, taskType string) {
	logger.Do.Println("Scheduler: Begin to schedule")

	// checking if the ip of worker match the qItem
	this.Lock()
	this.allWorkerReady.Wait()
	this.Unlock()

	logger.Do.Println("Scheduler: All worker found: ", this.workers)

	if taskType == common.TrainExecutor{

		jsonString := this.schedulerHelper(this.TaskHandler, common.Worker, qItem)

		logger.Do.Println("Scheduler: Finish schedule all tasks, update status")

		client.JobUpdateResInfo(
			common.CoordSvcURLGlobal,
			"",
			string(jsonString),
			"",
			qItem.JobId)

		client.JobUpdateStatus(common.CoordSvcURLGlobal, this.jobStatus, qItem.JobId)

		client.ModelUpdate(
			common.CoordSvcURLGlobal,
			1,
			qItem.JobId)

		// stop worker after finishing the job
		this.killWorkers()

	}else if taskType == common.PredictExecutor{

		this.schedulerHelper(this.TaskHandler, common.ModelService,qItem)

		logger.Do.Println("Scheduler: Finish schedule all tasks, update status")

		client.ModelServiceUpdateStatus(common.CoordSvcURLGlobal, this.jobStatus, qItem.JobId)
	}
}


func (this *Master) schedulerHelper(

	taskHandler taskHandler,
	svcName string,
	qItem *cache.QItem) string{


	wg := sync.WaitGroup{}

	// execute the task
	logger.Do.Println("Scheduler: qItem.ips are ", qItem.IPs)
	netCfg := this.generateNetworkConfig(qItem.IPs)

	var trainStatuses []entitiy.DoTaskReply

	ctx, cancel := context.WithCancel(context.Background())

	go this.TaskStatusMonitor(&trainStatuses, ctx)

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
		for len(this.workers) > 0{

			if MaxSearchNumber <=0{
				panic("Max search Number reaches, Ip not Match Error ")
			}

			addr := this.workers[0]
			ip := strings.Split(addr, ":")[0]
			this.workers = this.workers[1:]

			// match using ip
			if ip == vip {
				wg.Add(1)
				// execute the task
				// append will allocate new memory inside the func stack,
				// must pass address of slice to func. such that multi goroutines can
				// update the original slices.
				go taskHandler(addr, svcName, args, &wg, &trainStatuses)
				break
			}else{
				this.workers = append(this.workers, addr)
				MaxSearchNumber--
			}
		}
	}
	// wait all job has been finished successfully
	wg.Wait()

	// shutdown all goroutines.
	cancel()
	jsonString, e := json.Marshal(trainStatuses)
	if e!=nil{
		logger.Do.Fatalln(e)
	}
	return string(jsonString)
}


func (this *Master) TaskHandler(
	workerAddr,
	svcName string,
	args *entitiy.DoTaskArgs,
	wg *sync.WaitGroup,
	trainStatuses *[]entitiy.DoTaskReply,
){

	defer wg.Done()

	argAddr := entitiy.EncodeDoTaskArgs(args)
	var rep entitiy.DoTaskReply

	logger.Do.Printf("Scheduler: begin to call %s.DoTask of the worker: %s \n", svcName, workerAddr)
	ok := client.Call(workerAddr, this.Proxy, svcName+".DoTask", argAddr, &rep)

	if !ok {
		logger.Do.Printf("Scheduler: Master calling %s, DoTask error\n", workerAddr)
		rep.ErrorMsg.RpcCallErrorMsg = "RpcCallError"
		rep.RpcCallError = true

	}else{
		logger.Do.Printf("Scheduler: calling %s.DoTask of the worker: %s successful \n", svcName, workerAddr)
		rep.RpcCallError = false
	}
	*trainStatuses = append(*trainStatuses, rep)
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


func (this *Master) TaskStatusMonitor(status *[]entitiy.DoTaskReply, ctx context.Context) {

	/**
	 * @Author
	 * @Description  if one task got error, kill all workers.
	 * @Date 6:48 下午 13/12/20
	 * @Param
	 * @return
	 **/

	for {
		select {
		case <- ctx.Done():
			fmt.Println("Scheduler: Stop TaskStatusMonitor")
			this.jobStatus = common.JobSuccessful
			return
		default:
			for _, v:= range *status{
				if v.RuntimeError == true || v.RpcCallError == true {
					this.jobStatus = common.JobFailed
					// kill all workers.
					this.killWorkers()
					return
				}

				if v.Killed==true{
					this.jobStatus = common.JobKilled
					return
				}
			}
		}
		time.Sleep(2*time.Second)
	}
}