package master

import (
	"context"
	"coordinator/cache"
	"coordinator/client"
	c "coordinator/client"
	"coordinator/common"
	"coordinator/distributed/entity"
	"coordinator/logger"
	"encoding/json"
	"fmt"
	"strconv"
	"strings"
	"sync"
	"time"
)

func (this *Master) schedule(qItem *cache.QItem) string {

	// checking if the ip of worker match the qItem
	this.Lock()
	this.allWorkerReady.Wait()
	this.Unlock()

	logger.Do.Println("Scheduler: All worker found: ", this.workers, " required: ",qItem.AddrList)

	jsonString := this.schedulerHelper(qItem)
	return jsonString
}


func (this *Master) schedulerHelper(qItem *cache.QItem) string{

	wg := sync.WaitGroup{}

	var portArray [][]int32

	// for each ip addr
	for i:=0; i<len(qItem.AddrList); i++{
		var ports []int32
		//generate n ports
		for i:=0; i<len(qItem.AddrList); i++{
			port := c.GetFreePort(common.CoordAddr)
			pint, _ := strconv.Atoi(port)
			ports = append(ports, int32(pint))
		}
		portArray = append(portArray, ports)
	}

	// execute the task
	netCfg := common.GenerateNetworkConfig(qItem.AddrList, portArray)

	var trainStatuses []entity.DoTaskReply

	ctx, cancel := context.WithCancel(context.Background())

	go this.TaskStatusMonitor(&trainStatuses, ctx)

	mpcPort := c.GetFreePort(common.CoordAddr)
	mpcPint, _ := strconv.Atoi(mpcPort)

	tmpStack :=  make([]string, len(this.workers))
	copy(tmpStack, this.workers)

	// find moc id address
	var MpcIp string
	for i, v := range qItem.AddrList {
		// todo: check by id==0 or type==active???
		if qItem.PartyInfo[i].ID==0{
			vip := strings.Split(v, ":")[0]
			MpcIp = vip
		}
	}

	for i, v := range qItem.AddrList {
		vip := strings.Split(v, ":")[0]

		args := new(entity.DoTaskArgs)
		args.IP = vip
		args.MpcIp = MpcIp
		args.MpcPort = uint(mpcPint)
		args.AssignID = qItem.PartyInfo[i].ID
		args.PartyNums = qItem.PartyNums
		args.JobFlType = qItem.JobFlType
		args.PartyInfo = qItem.PartyInfo[i]
		args.ExistingKey = qItem.ExistingKey
		args.TaskInfo = qItem.Tasks
		args.NetWorkFile = netCfg

		MaxSearchNumber := 100
		for len(tmpStack) > 0{

			if MaxSearchNumber <=0{
				panic("Max search Number reaches, Ip not Match Error ")
			}

			workerAddr := tmpStack[0]
			ip := strings.Split(workerAddr, ":")[0]
			tmpStack = tmpStack[1:]

			// match using ip
			if ip == vip {
				wg.Add(1)
				// execute the task
				// append will allocate new memory inside the func stack,
				// must pass addr of slice to func. such that multi goroutines can
				// update the original slices.
				go this.TaskHandler(workerAddr, args, &wg, &trainStatuses)
				break
			}else{
				tmpStack = append(tmpStack, workerAddr)
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
	workerAddr string,
	args *entity.DoTaskArgs,
	wg *sync.WaitGroup,
	trainStatuses *[]entity.DoTaskReply,
){

	defer wg.Done()

	argAddr := entity.EncodeDoTaskArgs(args)
	var rep entity.DoTaskReply

	logger.Do.Printf("Scheduler: begin to call %s.DoTask of the worker: %s \n", this.workerType, workerAddr)
	ok := client.Call(workerAddr, this.Proxy, this.workerType+".DoTask", argAddr, &rep)

	if !ok {
		logger.Do.Printf("Scheduler: Master calling %s, DoTask error\n", workerAddr)
		rep.TaskMsg.RpcCallMsg = ""
		rep.RpcCallError = true

	}else{
		logger.Do.Printf("Scheduler: calling %s.DoTask of the worker: %s successful \n", this.workerType, workerAddr)
		rep.RpcCallError = false
	}
	*trainStatuses = append(*trainStatuses, rep)
}

func (this *Master) TaskStatusMonitor(status *[]entity.DoTaskReply, ctx context.Context) {

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