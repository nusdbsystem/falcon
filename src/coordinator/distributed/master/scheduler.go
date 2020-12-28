package master

import (
	"context"
	"coordinator/cache"
	"coordinator/client"
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

func (master *Master) schedule(qItem *cache.QItem) string {

	// checking if the ip of worker match the qItem
	master.Lock()
	master.allWorkerReady.Wait()
	master.Unlock()

	logger.Do.Println("Scheduler: All worker found: ", master.workers, " required: ", qItem.AddrList)

	jsonString := master.schedulerHelper(qItem)
	return jsonString
}

func (master *Master) schedulerHelper(qItem *cache.QItem) string {

	wg := sync.WaitGroup{}

	var portArray [][]int32

	// for each ip addr
	for i := 0; i < len(qItem.AddrList); i++ {
		var ports []int32
		//generate n ports
		for i := 0; i < len(qItem.AddrList); i++ {
			port := client.GetFreePort(common.CoordAddr)
			pint, _ := strconv.Atoi(port)
			ports = append(ports, int32(pint))
		}
		portArray = append(portArray, ports)
	}

	// execute the task
	netCfg := common.GenerateNetworkConfig(qItem.AddrList, portArray)

	var trainStatuses []entity.DoTaskReply

	ctx, cancel := context.WithCancel(context.Background())

	go master.TaskStatusMonitor(&trainStatuses, ctx)

	mpcPort := client.GetFreePort(common.CoordAddr)
	mpcPint, _ := strconv.Atoi(mpcPort)

	tmpStack := make([]string, len(master.workers))
	copy(tmpStack, master.workers)

	// find mpc address
	var MpcIp string
	for i, addr := range qItem.AddrList {
		// todo: check by id==0 or type==active???
		// makes more sense to use party_type == active|passive
		if qItem.PartyInfo[i].PartyType == "active" {
			itemIP := strings.Split(addr, ":")[0]
			MpcIp = itemIP
		}
	}

	for i, addr := range qItem.AddrList {
		itemIP := strings.Split(addr, ":")[0]

		args := new(entity.DoTaskArgs)
		args.IP = itemIP
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
		for len(tmpStack) > 0 {

			if MaxSearchNumber <= 0 {
				panic("Max search Number reaches, Ip not Match Error ")
			}

			workerAddr := tmpStack[0]
			workerIP := strings.Split(workerAddr, ":")[0]
			tmpStack = tmpStack[1:]

			// match using ip
			if workerIP == itemIP {
				wg.Add(1)
				// execute the task
				// append will allocate new memory inside the func stack,
				// must pass addr of slice to func. such that multi goroutines can
				// update the original slices.
				go master.TaskHandler(workerAddr, args, &wg, &trainStatuses)
				break
			} else {
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
	if e != nil {
		logger.Do.Fatalln(e)
	}
	return string(jsonString)
}

func (master *Master) TaskHandler(
	workerAddr string,
	args *entity.DoTaskArgs,
	wg *sync.WaitGroup,
	trainStatuses *[]entity.DoTaskReply,
) {

	defer wg.Done()

	argAddr := entity.EncodeDoTaskArgs(args)
	var rep entity.DoTaskReply

	logger.Do.Printf("Scheduler: begin to call %s.DoTask of the worker: %s \n", master.workerType, workerAddr)
	ok := client.Call(workerAddr, master.Proxy, master.workerType+".DoTask", argAddr, &rep)

	if !ok {
		logger.Do.Printf("Scheduler: Master calling %s, DoTask error\n", workerAddr)
		rep.TaskMsg.RpcCallMsg = ""
		rep.RpcCallError = true

	} else {
		logger.Do.Printf("Scheduler: calling %s.DoTask of the worker: %s successful \n", master.workerType, workerAddr)
		rep.RpcCallError = false
	}
	*trainStatuses = append(*trainStatuses, rep)
}

func (master *Master) TaskStatusMonitor(status *[]entity.DoTaskReply, ctx context.Context) {

	/**
	 * @Author
	 * @Description  if one task got error, kill all workers.
	 * @Date 6:48 下午 13/12/20
	 * @Param
	 * @return
	 **/

	for {
		select {
		case <-ctx.Done():
			fmt.Println("Scheduler: Stop TaskStatusMonitor")
			master.jobStatus = common.JobSuccessful
			return
		default:
			for _, v := range *status {
				if v.RuntimeError == true || v.RpcCallError == true {
					master.jobStatus = common.JobFailed
					// kill all workers.
					master.killWorkers()
					return
				}

				if v.Killed == true {
					master.jobStatus = common.JobKilled
					return
				}
			}
		}
		time.Sleep(2 * time.Second)
	}
}
