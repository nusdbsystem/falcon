package master

import (
	"context"
	"encoding/json"
	"falcon_platform/cache"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/distributed/entity"
	"falcon_platform/logger"
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

	logger.Log.Println("Scheduler: All RegisteredWorker found: ", master.workers, " required IP: ", qItem.AddrList, "Ip matched !!")

	jsonString := master.schedulerHelper(qItem)
	return jsonString
}

func (master *Master) schedulerHelper(qItem *cache.QItem) string {

	wg := sync.WaitGroup{}

	var portArray [][]int32

	// 1. generate config (ports) for each party-server's train task
	// 2. generate config (ports) for each party-server's mpc task
	// 3. combine the config and assign it to each worker
	// 4. generate config for each party-server

	// 1. generate config (ports) for each party-server's train task
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
	netCfg := common.GenerateNetworkConfig(qItem.AddrList, portArray)

	var trainStatuses []entity.DoTaskReply

	// use to cancel threads
	ctx, cancel := context.WithCancel(context.Background())
	// wait until status are recorded
	statusRecorded := make(chan bool)
	go master.TaskStatusMonitor(&trainStatuses, ctx, statusRecorded)

	mpcPort := client.GetFreePort(common.CoordAddr)
	mpcPint, _ := strconv.Atoi(mpcPort)

	// 2. generate config (ports) for each party-server's mpc task
	var MpcIP string
	for i, addr := range qItem.AddrList {
		// todo: check by id==0 or type==active???
		// makes more sense to use party_type == active|passive
		if qItem.PartyInfo[i].PartyType == "active" {
			MpcIP = strings.Split(addr, ":")[0]
		}
	}
	//logger.Log.Println("complete qItem.PartyInfo = ", qItem.PartyInfo)

	// 3. combine the config and assign it to each worker
	for i, addr := range qItem.AddrList {
		itemIP := strings.Split(addr, ":")[0]
		args := new(entity.DoTaskArgs)
		args.IP = itemIP
		args.MpcIP = MpcIP
		args.MpcPort = uint(mpcPint)
		args.PartyInfo = qItem.PartyInfo[i]
		// NOTE: PartyInfo contains ParyType which is parsed later in traintask.go:
		// traintask.go: partyTypeStr := doTaskArgs.PartyInfo.PartyType
		args.PartyID = qItem.PartyInfo[i].ID
		args.PartyNums = qItem.PartyNums
		args.JobFlType = qItem.JobFlType
		args.ExistingKey = qItem.ExistingKey
		args.TaskInfo = qItem.Tasks
		args.NetWorkFile = netCfg
		// a list of master.workers:
		// eg: [127.0.0.1:30009:0 127.0.0.1:30010:1 127.0.0.1:30011:2]
		for _, RegisteredWorker := range master.workers {
			// RegisteredWorker is IP:Port:PartyID
			RegisteredWorkerAddr := strings.Join(strings.Split(RegisteredWorker, ":")[:2], ":")
			RegisteredWorkerIP := strings.Split(RegisteredWorker, ":")[0]
			RegisteredWorkerPartyID := strings.Split(RegisteredWorker, ":")[2]

			// match using IP (if the IPs are from different devices)
			// practically for single-machine and multiple terminals, the IP will be the same
			// thus, needs to match the registered worker with the PartyServer info
			// check for registeredworker's partyID == args.partyID
			if RegisteredWorkerIP == args.IP && RegisteredWorkerPartyID == fmt.Sprintf("%d", args.PartyID) {
				//logger.Log.Println("RegisteredWorkerIP = ", RegisteredWorkerIP, " == ", args.IP, " = args.IP")
				//logger.Log.Println("PartyID matches RegisteredWorkerPartyID match args PartyID")
				logger.Log.Println(
					"Dispatch RegisteredWorker=", RegisteredWorker,
					"with the Job - args.PartyInfo = ", args.PartyInfo)

				wg.Add(1)
				// execute the task
				// append will allocate new memory inside the func stack
				// so must pass addr of slice to func. such that multi goroutines can update the original slices.
				go master.TaskHandler(RegisteredWorkerAddr, args, &wg, &trainStatuses)
				break
			}
		}
	}
	// wait all job has been finished successfully
	wg.Wait()

	// shutdown all goroutines. now it's only TaskStatusMonitor
	cancel()

	// wait until status updated
	<-statusRecorded
	jsonString, e := json.Marshal(trainStatuses)

	logger.Log.Println("Scheduler: Final status are: ", string(jsonString))

	if e != nil {
		logger.Log.Fatalln(e)
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

	logger.Log.Printf("[Scheduler]: call %s.DoTask of the RegisteredWorker: %s \n", master.workerType, RegisteredWorkerAddr)

	// call worker to do task, until worker finish
	ok := client.Call(RegisteredWorkerAddr, master.Network, master.workerType+".DoTask", argAddr, &rep)

	if !ok {
		logger.Log.Printf("[Scheduler]: Master calling %s, DoTask error\n", RegisteredWorkerAddr)
		rep.TaskMsg.RpcCallMsg = "RpcCallError, one worker is probably terminated"
		rep.RpcCallError = true

	} else {
		logger.Log.Printf("[Scheduler]: calling %s.DoTask of the RegisteredWorker: %s successful \n", master.workerType, RegisteredWorkerAddr)
		rep.RpcCallError = false
		rep.TaskMsg.RpcCallMsg = ""
	}
	*trainStatuses = append(*trainStatuses, rep)
}

func (master *Master) TaskStatusMonitor(status *[]entity.DoTaskReply, ctx context.Context, statusRecorded chan bool) {
	// monitor each worker's status, update the final status accordingly
	// if one task got error, kill all RegisteredWorkers.

	defer func() {
		statusRecorded <- true
	}()

	for {
		select {
		case <-ctx.Done():
			// after finishing,
			// 1st loop, check each worker's status failed or not, one failed , all failed
			for _, v := range *status {
				if v.RuntimeError == true || v.RpcCallError == true {
					master.jobStatus = common.JobFailed
					return
				}
			}
			// 2rd loop, check each worker's status killed or not, one killed , all killed
			for _, v := range *status {
				if v.Killed == true {
					master.jobStatus = common.JobKilled
					return
				}
			}

			// no errors means successful.
			master.jobStatus = common.JobSuccessful
			return
		default:
			// run at the same time with tasks, once found one errored, kill other process and record status as failed.
			for _, v := range *status {
				if v.RuntimeError == true || v.RpcCallError == true {
					master.jobStatus = common.JobFailed
					// kill all RegisteredWorkers.
					logger.Log.Printf("[Scheduler]: Some worker failed, kill other workers")
					master.killWorkers()
					return
				}

			}
		}
		time.Sleep(10 * time.Millisecond)
	}
}
