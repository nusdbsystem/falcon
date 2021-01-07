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

	// checking if the IP of RegisteredWorker match the qItem
	master.Lock()
	master.allWorkerReady.Wait()
	master.Unlock()

	logger.Do.Println("Scheduler: All RegisteredWorker found: ", master.workers, " required: ", qItem.AddrList)

	jsonString := master.schedulerHelper(qItem)
	return jsonString
}

func (master *Master) schedulerHelper(qItem *cache.QItem) string {

	logger.Do.Println("master ===> ", master)
	logger.Do.Println("qItem  ---> ", qItem)

	wg := sync.WaitGroup{}

	var portArray [][]int32

	// for each IP addr
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

	// find mpc address
	var MpcIP string
	for i, addr := range qItem.AddrList {
		// todo: check by id==0 or type==active???
		// makes more sense to use party_type == active|passive
		if qItem.PartyInfo[i].PartyType == "active" {
			itemIP := strings.Split(addr, ":")[0]
			MpcIP = itemIP
		}
	}

	logger.Do.Println("complete qItem.PartyInfo = ", qItem.PartyInfo)
	for i, addr := range qItem.AddrList {
		logger.Do.Println("---iterating the qItem AddrList to dispatch job to Workers")
		logger.Do.Println("for i = ", i, " addr = ", addr)
		itemIP := strings.Split(addr, ":")[0]

		args := new(entity.DoTaskArgs)

		args.IP = itemIP
		logger.Do.Println("args.IP = ", args.IP)

		args.MpcIP = MpcIP
		logger.Do.Println("args.MpcIP = ", args.MpcIP)

		args.MpcPort = uint(mpcPint)
		logger.Do.Println("args.MpcPort = ", args.MpcPort)

		args.PartyInfo = qItem.PartyInfo[i]
		logger.Do.Println("args.PartyInfo = ", args.PartyInfo)
		// NOTE: PartyInfo contains ParyType which is parsed later in traintask.go:
		// traintask.go: partyTypeStr := doTaskArgs.PartyInfo.PartyType

		args.PartyID = qItem.PartyInfo[i].ID
		logger.Do.Println("args.PartyID = ", args.PartyID)

		args.PartyNums = qItem.PartyNums
		logger.Do.Println("args.PartyNums = ", args.PartyNums)

		args.JobFlType = qItem.JobFlType
		logger.Do.Println("args.JobFlType = ", args.JobFlType)

		args.ExistingKey = qItem.ExistingKey
		logger.Do.Println("args.ExistingKey = ", args.ExistingKey)

		args.TaskInfo = qItem.Tasks
		logger.Do.Println("args.TaskInfo = ", args.TaskInfo)

		args.NetWorkFile = netCfg
		logger.Do.Println("args.NetWorkFile = ", args.NetWorkFile)

		// a list of master.workers:
		// eg: [127.0.0.1:30009:0 127.0.0.1:30010:1 127.0.0.1:30011:2]
		for i, RegisteredWorker := range master.workers {
			logger.Do.Println("iterating the master.wokers i=", i)
			// RegisteredWorker is IP:Port:PartyID
			logger.Do.Println("RegisteredWorker = ", RegisteredWorker)
			// Addr = IP:Port
			RegisteredWorkerAddr := strings.Join(strings.Split(RegisteredWorker, ":")[:2], ":")
			logger.Do.Println("RegisteredWorkerAddr = ", RegisteredWorkerAddr)

			RegisteredWorkerIP := strings.Split(RegisteredWorker, ":")[0]
			logger.Do.Println("RegisteredWorkerIP = ", RegisteredWorkerIP)
			RegisteredWorkerPort := strings.Split(RegisteredWorker, ":")[1]
			logger.Do.Println("RegisteredWorkerPort = ", RegisteredWorkerPort)
			RegisteredWorkerPartyID := strings.Split(RegisteredWorker, ":")[2]
			logger.Do.Println("RegisteredWorkerPartyID = ", RegisteredWorkerPartyID)

			// match using IP (if the IPs are from different devices)
			// practically for single-machine and multiple terminals, the IP will be the same
			// thus, needs to match the registered worker with the PartyServer info
			// check for registeredworker's partyID == args.partyID
			if RegisteredWorkerIP == args.IP && RegisteredWorkerPartyID == fmt.Sprintf("%d", args.PartyID) {
				logger.Do.Println("...RegisteredWorkerIP = ", RegisteredWorkerIP, " == ", args.IP, " = args.IP")
				logger.Do.Println("...PartyID matches RegisteredWorkerPartyID match args PartyID")
				logger.Do.Println(
					"...Dispatch the Job with args.PartyInfo = ",
					args.PartyInfo,
					" to RegisteredWorker @ ", RegisteredWorker)
				wg.Add(1)
				// execute the task
				// append will allocate new memory inside the func stack,
				// must pass addr of slice to func. such that multi goroutines can
				// update the original slices.
				go master.TaskHandler(RegisteredWorkerAddr, args, &wg, &trainStatuses)
				break
			} else {
				logger.Do.Println("Register Worker IP does not match args.IP, or the PartyID has no match")
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
	RegisteredWorkerAddr string,
	args *entity.DoTaskArgs,
	wg *sync.WaitGroup,
	trainStatuses *[]entity.DoTaskReply,
) {

	defer wg.Done()

	argAddr := entity.EncodeDoTaskArgs(args)
	var rep entity.DoTaskReply

	logger.Do.Printf("Scheduler: begin to call %s.DoTask of the RegisteredWorker: %s \n", master.workerType, RegisteredWorkerAddr)
	ok := client.Call(RegisteredWorkerAddr, master.Network, master.workerType+".DoTask", argAddr, &rep)

	if !ok {
		logger.Do.Printf("Scheduler: Master calling %s, DoTask error\n", RegisteredWorkerAddr)
		rep.TaskMsg.RpcCallMsg = ""
		rep.RpcCallError = true

	} else {
		logger.Do.Printf("Scheduler: calling %s.DoTask of the RegisteredWorker: %s successful \n", master.workerType, RegisteredWorkerAddr)
		rep.RpcCallError = false
	}
	*trainStatuses = append(*trainStatuses, rep)
}

func (master *Master) TaskStatusMonitor(status *[]entity.DoTaskReply, ctx context.Context) {
	// if one task got error, kill all RegisteredWorkers.
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
					// kill all RegisteredWorkers.
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
