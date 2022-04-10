package master

import (
	"bytes"
	"encoding/json"
	"falcon_platform/common"
	"falcon_platform/jobmanager/base"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"fmt"
	"log"
	"os"
	"sort"
	"strings"
	"sync"
)

type ExtractedResource struct {
	// each worker or ps has a mpc, and they will communicate with each other
	mpcPairNetworkCfg      map[common.WorkerIdType]string
	executorPairNetworkCfg map[common.WorkerIdType]string
	mpcExecutorNetworkCfg  map[common.WorkerIdType][]string
	distributedNetworkCfg  map[common.PartyIdType]map[common.GroupIdType]string
	requiredWorkers        map[common.PartyIdType]map[common.WorkerIdType]bool
}

type Master struct {
	base.RpcBaseClass
	// cond
	beginCountDown       *sync.Cond
	allWorkerReady       *sync.Cond
	BeginCountingWorkers *sync.Cond

	// required worker's resources,
	// partyServer reply to jobManager-master after deploying resources.
	RequiredResource  []*common.LaunchResourceReply
	ExtractedResource ExtractedResource
	// total party number
	PartyNums uint

	// lime
	//SchedulerPolicy *DAGscheduler.ParallelismSchedulePolicy

	// tmp slice to store registered workers
	tmpWorkers chan string

	// slice to store valid workers
	workers   []*entity.WorkerInfo
	workerNum int

	// jobStatus indicates if job successful, failed, killed running, init
	jobStatus     string
	jobStatusLock sync.Mutex
	// record job status message to db
	jobStatusLog string
	// record runtime status in real time
	runtimeStatus chan *entity.DoTaskReply

	// master heartbeat setting
	lastSendTime     int64
	heartbeatTimeout int

	// check if at least one worker found, decide when to board cast heartbeat
	foundWorker bool

	// common.TrainWorker or common.InferenceWorker
	workerType string

	// logger instance
	Logger  *log.Logger
	LogFile *os.File
}

func newMaster(masterAddr string, partyNum uint) (ms *Master) {
	ms = new(Master)
	ms.InitRpcBase(masterAddr)
	ms.Name = common.Master
	ms.beginCountDown = sync.NewCond(ms)
	ms.allWorkerReady = sync.NewCond(ms)
	ms.BeginCountingWorkers = sync.NewCond(ms)

	// default cache 100 workers at most,
	// use cache to allow workers to register before master running forwardRegistrations to consume worker's information
	ms.tmpWorkers = make(chan string, 100)

	//default to successful, update at runtime, record to db after job finishing
	ms.jobStatus = common.JobSuccessful
	ms.runtimeStatus = make(chan *entity.DoTaskReply)

	ms.heartbeatTimeout = common.MasterTimeout
	ms.workerNum = 0
	ms.PartyNums = partyNum
	ms.RequiredResource = make([]*common.LaunchResourceReply, ms.PartyNums)
	//ms.SchedulerPolicy = DAGscheduler.NewParallelismSchedulePolicy()
	return
}

// RegisterWorker is an RPC method that is called by workers after they have started
// up to report that they are ready to receive tasks.
func (master *Master) RegisterWorker(args *entity.WorkerInfo, _ *struct{}) error {

	var out bytes.Buffer
	bs, _ := json.Marshal(args)
	_ = json.Indent(&out, bs, "", "\t")
	master.Logger.Printf("[master.RegisterWorker] one Worker registered! args: %v\n", out.String())

	// Pass WorkerAddrIdType (addr:partyID) into tmpWorkers for pre-processing
	// IP:Port:WorkerID
	encodedStr := entity.EncodeWorkerInfo(args)
	master.tmpWorkers <- encodedStr
	return nil
}

// sends information of worker to ch. which is used by scheduler
func (master *Master) forwardRegistrations() {
	master.Lock()
	master.BeginCountingWorkers.Wait()
	master.Unlock()

	master.Logger.Printf("[Master.forwardRegistrations]: start forwardRegistrations... ")

loop:
	for {
		select {
		case <-master.Ctx.Done():

			master.Logger.Println("[Master.forwardRegistrations]: Thread-2 forwardRegistrations: exit")
			break loop

		case tmpWorker := <-master.tmpWorkers:

			// 1. decode tmpWorker
			workerInfo := entity.DecodeWorkerInfo(tmpWorker)

			// 2. check if this work already exist
			if master.ExtractedResource.requiredWorkers[workerInfo.PartyID][workerInfo.WorkerID] == false {
				master.Lock()

				master.workers = append(master.workers, workerInfo)

				master.Unlock()
				master.beginCountDown.Broadcast()
				master.ExtractedResource.requiredWorkers[workerInfo.PartyID][workerInfo.WorkerID] = true

			} else {
				master.Logger.Printf("[Master]: the worker %s already registered, skip \n", tmpWorker)
			}

			// 3. check if all worker registered
			master.Lock()
			if len(master.workers) == master.workerNum {
				// it is not strictly necessary for you to hold the lock on M sync.Mutex when calling C.Broadcast()
				master.allWorkerReady.Broadcast()
			}
			master.Unlock()
		}
	}
}

func (master *Master) ExtractResourceInformation() {
	// key: workerID, matching worker of different party has same id
	// value: executor address

	var executorPairIps = make(map[common.WorkerIdType][]string)
	var executorPairsPorts = make(map[common.WorkerIdType][][]common.PortType)

	var mpcPairsPorts = make(map[common.WorkerIdType][]common.PortType)
	var mpcExecutorPorts = make(map[common.WorkerIdType][]common.PortType)

	var distPsIp = make(map[common.PartyIdType]map[common.GroupIdType]string)
	var distPsPorts = make(map[common.PartyIdType]map[common.GroupIdType][]common.PortType)

	// worker address and port of one party
	var distWorkerIps = make(map[common.PartyIdType]map[common.GroupIdType][]string)
	var distWorkerPorts = make(map[common.PartyIdType]map[common.GroupIdType][]common.PortType)

	// each matching pair, have 3 network cfg, if there is ps, 4 network configs
	var executorPairNetworkCfg = make(map[common.WorkerIdType]string)
	var mpcPairNetworkCfg = make(map[common.WorkerIdType]string)
	var distributedNetworkCfg = make(map[common.PartyIdType]map[common.GroupIdType]string)

	// 2-dim to record required workers, each worker is labeled with partyID, workerID
	var requiredWorkers = make(map[common.PartyIdType]map[common.WorkerIdType]bool)

	// for each party's replied "LaunchResourceReply" struct
	for partyIndex, LaunchResourceReply := range master.RequiredResource {
		partyID := LaunchResourceReply.PartyID
		// there are only 1 ps in each party's group in distributed train
		trainWorkerPreGroup := LaunchResourceReply.ResourceNumPreGroup
		master.Logger.Println("[Master.extractResourceInfo] -----debug-----trainWorkerPreGroup:", trainWorkerPreGroup)

		// get sorted work id
		var orderedWorkIds []int
		for workerID, _ := range LaunchResourceReply.ResourceSVCs {
			orderedWorkIds = append(orderedWorkIds, int(workerID))
		}
		sort.Ints(orderedWorkIds)

		// for each resource in single party, force counting start from 0
		for _, orderedWorkId := range orderedWorkIds {
			for workerID, ResourceSVC := range LaunchResourceReply.ResourceSVCs {
				if orderedWorkId == int(workerID) {
					master.workerNum++
					// required workers id
					if _, ok := requiredWorkers[partyID]; !ok {
						requiredWorkers[partyID] = make(map[common.WorkerIdType]bool)
					}
					requiredWorkers[partyID][workerID] = false

					// resource executor-executor ops
					if _, ok := executorPairIps[workerID]; !ok {
						executorPairIps[workerID] = make([]string, master.PartyNums)
					}
					executorPairIps[workerID][partyIndex] = ResourceSVC.ResourceIP
					//master.Logger.Println("[Master.extractResourceInfo] -----debug-----extractResourceInfo:", executorPairIps)

					// record executor-executor ports
					if _, ok := executorPairsPorts[workerID]; !ok {
						executorPairsPorts[workerID] = make([][]common.PortType, master.PartyNums)
					}
					executorPairsPorts[workerID][partyIndex] = ResourceSVC.ExecutorExecutorPort

					// record mpc-mpc ports
					if _, ok := mpcPairsPorts[workerID]; !ok {
						mpcPairsPorts[workerID] = make([]common.PortType, master.PartyNums)
					}
					mpcPairsPorts[workerID][partyIndex] = ResourceSVC.MpcMpcPort

					// mpc-executor ports
					if _, ok := mpcExecutorPorts[workerID]; !ok {
						mpcExecutorPorts[workerID] = make([]common.PortType, master.PartyNums)
					}
					mpcExecutorPorts[workerID][partyIndex] = ResourceSVC.MpcExecutorPort

					// for distributed parameter
					if ResourceSVC.DistributedRole == common.DistributedParameterServer {

						if _, ok := distPsIp[partyID]; !ok {
							distPsIp[partyID] = make(map[common.GroupIdType]string)
						}
						distPsIp[partyID][ResourceSVC.GroupId] = ResourceSVC.ResourceIP

						// record ps-worker ports
						if _, ok := distPsPorts[partyID]; !ok {
							distPsPorts[partyID] = make(map[common.GroupIdType][]common.PortType)
						}
						if _, ok := distPsPorts[partyID][ResourceSVC.GroupId]; !ok {
							// len = number of train workers
							distPsPorts[partyID][ResourceSVC.GroupId] = make([]common.PortType, trainWorkerPreGroup)
						}
						for wIndex, port := range ResourceSVC.PsExecutorPorts {
							distPsPorts[partyID][ResourceSVC.GroupId][wIndex] = port
						}
					}

					// if it's distributed worker, record worker's ip to generate distributed network alter
					if ResourceSVC.DistributedRole == common.DistributedWorker {

						if _, ok := distWorkerIps[partyID]; !ok {
							distWorkerIps[partyID] = make(map[common.GroupIdType][]string)
						}

						//which is in order
						distWorkerIps[partyID][ResourceSVC.GroupId] =
							append(distWorkerIps[partyID][ResourceSVC.GroupId], ResourceSVC.ResourceIP)

						if _, ok := distWorkerPorts[partyID]; !ok {
							distWorkerPorts[partyID] = make(map[common.GroupIdType][]common.PortType)
						}

						distWorkerPorts[partyID][ResourceSVC.GroupId] =
							append(distWorkerPorts[partyID][ResourceSVC.GroupId], ResourceSVC.ExecutorPSPort)
					}
					break
				}
			}
		}

		// get distributed networkCfg used inside each party
		for groupIndex := 0; groupIndex < LaunchResourceReply.GroupNum; groupIndex++ {
			groupID := common.GroupIdType(groupIndex)
			if _, ok := distributedNetworkCfg[partyID]; !ok {
				distributedNetworkCfg[partyID] = make(map[common.GroupIdType]string)
			}

			distributedNetworkCfg[partyID][groupID] = getDistributedNetworkCfg(
				distPsIp[partyID][groupID], distPsPorts[partyID][groupID],
				distWorkerIps[partyID][groupID], distWorkerPorts[partyID][groupID])

			master.Logger.Println("[Master.extractResourceInfo], result of getDistributedNetworkCfg when partyID =",
				partyID, " group_id = ", groupID,
				"distPsIp: ", distPsIp[partyID][groupID],
				"distPsPorts: ", distPsPorts[partyID][groupID],
				"distWorkerIps: ", distWorkerIps[partyID][groupID],
				"distWorkerPorts: ", distWorkerPorts[partyID][groupID],
				"distributedNetworkCfg: ", distributedNetworkCfg[partyID][groupID])

			common.RetrieveDistributedNetworkConfig(distributedNetworkCfg[partyID][groupID])
		}
	}

	// generate network for each executor pair
	for workerID, IPs := range executorPairIps {

		var executorPortArray [][]int32
		var mpcPortArray []int32

		// generate port matrix for exe-exe communication
		executorPortArray = make([][]int32, master.PartyNums)
		for i, portArray := range executorPairsPorts[workerID] {
			tmp := make([]int32, master.PartyNums)
			for j, port := range portArray {
				tmp[j] = int32(port)
			}
			executorPortArray[i] = tmp
		}

		// generate mpc port array, for each executor
		for _, port := range mpcExecutorPorts[workerID] {
			mpcPortArray = append(mpcPortArray, int32(port))
		}
		executorPairNetworkCfg[workerID] = common.GenerateNetworkConfig(IPs, executorPortArray, mpcPortArray)

		// generate network for each mpc pair
		var portsMpc []int32
		for _, port := range mpcPairsPorts[workerID] {
			portsMpc = append(portsMpc, int32(port))
		}
		mpcPairNetworkCfg[workerID] = getMpcMpcNetworkCfg(IPs, portsMpc)
	}

	master.Logger.Println("[Master.extractResourceInfo] executorPairNetworkCfg workerID:b64_Str is ", executorPairNetworkCfg)

	mpcExecutorNetworkCfg := getMpcExecutorNetworkCfg(mpcExecutorPorts)

	master.Logger.Println("[Master.extractResourceInfo] -----checking configurations-----")
	master.Logger.Printf("[Master.extractResourceInfo] master.RequiredResource is :\n")

	master.Logger.Printf("[Master.extractResourceInfo] workerNum: %d", master.workerNum)

	for i := 0; i < len(master.RequiredResource); i++ {
		master.Logger.Printf("[Master.extractResourceInfo] party %d PartyID: %d", i, master.RequiredResource[i].PartyID)
		master.Logger.Printf("[Master.extractResourceInfo] party %d ResourceNum: %d", i, master.RequiredResource[i].ResourceNum)
		master.Logger.Printf("[Master.extractResourceInfo] party %d ResourceNumPreGroup: %d", i, master.RequiredResource[i].ResourceNumPreGroup)

		for workerID, svc := range master.RequiredResource[i].ResourceSVCs {
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.WorkerId: %d", i, workerID, svc.GroupId, svc.WorkerId)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d", i, workerID, svc.GroupId)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.ResourceIP: %s", i, workerID, svc.GroupId, svc.ResourceIP)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.WorkerPort: %d", i, workerID, svc.GroupId, svc.WorkerPort)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.ExecutorExecutorPort: %d", i, workerID, svc.GroupId, svc.ExecutorExecutorPort)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.MpcMpcPort: %d", i, workerID, svc.GroupId, svc.MpcMpcPort)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.MpcExecutorPort: %d", i, workerID, svc.GroupId, svc.MpcExecutorPort)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.ExecutorPSPort: %d", i, workerID, svc.GroupId, svc.ExecutorPSPort)
			master.Logger.Printf("[Master.extractResourceInfo] party %d, workerID %d, groupID %d, ResourceSVCs.DistributedRole: %s\n", i, workerID, svc.GroupId, common.DistributedRoleToName(svc.DistributedRole))
		}
	}

	master.Logger.Println("[Master.extractResourceInfo] requiredWorkers is ", requiredWorkers)
	master.Logger.Println("[Master.extractResourceInfo] executorPairIps is ", executorPairIps)
	master.Logger.Println("[Master.extractResourceInfo] executorPairsPorts is ", executorPairsPorts)
	master.Logger.Println("[Master.extractResourceInfo] mpcPairsPorts is ", mpcPairsPorts)
	master.Logger.Println("[Master.extractResourceInfo] mpcExecutorPorts workerID:[partyPort...] is ", mpcExecutorPorts)
	master.Logger.Println("[Master.extractResourceInfo] distPsIp is ", distPsIp)
	master.Logger.Println("[Master.extractResourceInfo] distWorkerIps is ", distWorkerIps)

	// network configs
	master.Logger.Println("[Master.extractResourceInfo] mpcPairNetworkCfg workerID:partyMpcMpcAddrs is is ", mpcPairNetworkCfg)
	master.Logger.Println("[Master.extractResourceInfo] executorPairNetworkCfg workerID:b64_Str is ", executorPairNetworkCfg)
	master.Logger.Println("[Master.extractResourceInfo] mpcExecutorNetworkCfg workerID:[partyMpcExecutorPort...] is ", mpcExecutorNetworkCfg)
	master.Logger.Println("[Master.extractResourceInfo] distributedNetworkCfg partyID:b64_Str is ", distributedNetworkCfg)

	master.Logger.Println("[Master.extractResourceInfo] -----done with checking configurations-----")

	master.ExtractedResource.mpcPairNetworkCfg = mpcPairNetworkCfg
	master.ExtractedResource.executorPairNetworkCfg = executorPairNetworkCfg
	master.ExtractedResource.mpcExecutorNetworkCfg = mpcExecutorNetworkCfg
	master.ExtractedResource.distributedNetworkCfg = distributedNetworkCfg
	master.ExtractedResource.requiredWorkers = requiredWorkers
}

func (master *Master) run(
	dispatcher func(),
	updateStatus func(jsonString string),
	finish func(),
) {
	// master's main logic,
	// 1. schedule the job to workers,
	// 2. collect the job status, call coordinator's endpoints to update status to db
	// 3. close all resources related to this job

	dispatcher()
	master.Logger.Printf("[Master]: Finish job, update to coord, jobStatus: <%s> \n", master.jobStatus)

	updateStatus(master.jobStatusLog)
	master.Logger.Println("[Master]: Finish job, begin to shutdown master")

	finish()
	master.Logger.Printf("[Master] %s: Thread-4 master.finish, job completed\n", master.Addr)

}

// master wait until all master's logic finished
func (master *Master) WaitJobComplete() {

loop:
	for {
		select {
		case <-master.Ctx.Done():
			break loop
		}
	}
}

// Shutdown is an RPC method that shuts down the Master's RPC server.
// for rpc method, must be public method, only 2 params, second one must be pointer,return err type
func (master *Master) Shutdown(_, _ *struct{}) error {

	master.Logger.Println("[Master]: Shutdown server")
	// causes the Accept to fail, then break out the accept loop
	_ = master.Listener.Close()
	return nil
}

// called by coordinator, to shutdown the running job
func (master *Master) KillJob(_, _ *struct{}) error {
	master.killWorkers()
	master.jobStatusLock.Lock()
	master.jobStatus = common.JobKilled
	master.jobStatusLock.Unlock()
	return nil
}

func (master *Master) killWorkers() {
	// after called killWorker, scheduler in master.run will finish, "jobUpdateStatus" and "finish" will be called
	master.Lock()
	defer master.Unlock()

	for len(master.workers) > 0 {
		worker := master.workers[0]
		master.StopRPCServer(worker.Addr, master.workerType+".Shutdown")
		master.workers = master.workers[1:]
	}
}

// generate network config for executor-parameter server communication
func getDistributedNetworkCfg(distPsIp string,
	partyPsPorts []common.PortType,
	workerIps []string,
	partyWorkerPorts []common.PortType) string {

	var workerPorts []int32
	var PsPorts []int32

	for _, p := range partyPsPorts {
		PsPorts = append(PsPorts, int32(p))
	}

	for _, p := range partyWorkerPorts {
		workerPorts = append(workerPorts, int32(p))
	}
	logger.Log.Println("[getDistributedNetworkCfg]:", workerPorts, PsPorts)

	netCfg := common.GenerateDistributedNetworkConfig(
		distPsIp, PsPorts, workerIps, workerPorts)

	return netCfg
}

// generate network config for mpc-mpc server communication
func getMpcMpcNetworkCfg(workerIps []string, ports []int32) string {

	var res []string
	for i, workerIp := range workerIps {
		mpcPortInt := ports[i]
		res = append(res, fmt.Sprintf("%s:%d", workerIp, mpcPortInt))
	}
	return strings.Join(res, "\n")
}

// generate network config for mpc-mpc server communication
func getMpcExecutorNetworkCfg(mpcExecutorPorts map[common.WorkerIdType][]common.PortType) map[common.WorkerIdType][]string {
	var mpcExecutorNetworkCfg = make(map[common.WorkerIdType][]string)
	for workerID, portArry := range mpcExecutorPorts {
		mpcExecutorNetworkCfg[workerID] = make([]string, len(mpcExecutorPorts[workerID]))
		for partyIndex, port := range portArry {
			mpcExecutorNetworkCfg[workerID][partyIndex] = fmt.Sprintf("%d", port)
		}
	}
	return mpcExecutorNetworkCfg
}
