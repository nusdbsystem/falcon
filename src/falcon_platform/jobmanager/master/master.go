package master

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/base"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"strings"
	"sync"
)

type ExtractedResource struct {
	mpcPairNetworkCfg      map[common.WorkerIdType]string
	executorPairNetworkCfg map[common.WorkerIdType]string
	mpcExecutorNetworkCfg  map[common.WorkerIdType][]string
	distributedNetworkCfg  map[common.PartyIdType]string
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
	return
}

// RegisterWorker is an RPC method that is called by workers after they have started
// up to report that they are ready to receive tasks.
func (master *Master) RegisterWorker(args *entity.WorkerInfo, _ *struct{}) error {

	logger.Log.Println("[master/RegisterWorker] one Worker registered! args: ", args)
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

	logger.Log.Printf("[Master]: start forwardRegistrations... ")

loop:
	for {
		select {
		case <-master.Ctx.Done():

			logger.Log.Println("[Master]: Thread-2 forwardRegistrations: exit")
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
				logger.Log.Printf("[Master]: the worker %s already registered, skip \n", tmpWorker)
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
	var psPairIps = make(map[common.WorkerIdType][]string)
	var executorPairsPorts = make(map[common.WorkerIdType][][]common.PortType)
	var mpcPairsPorts = make(map[common.WorkerIdType][]common.PortType)
	var mpcExecutorPorts = make(map[common.WorkerIdType][]common.PortType)
	var distPsIp = make(map[common.PartyIdType]string)
	var distWorkerIps = make(map[common.PartyIdType][]string)
	var psPairsPorts = make(map[common.WorkerIdType][][]common.PortType)

	// each matching pair, have 3 network cfg, if there is ps, 4 network configs
	var executorPairNetworkCfg = make(map[common.WorkerIdType]string)
	var mpcPairNetworkCfg = make(map[common.WorkerIdType]string)
	var distributedNetworkCfg = make(map[common.PartyIdType]string)

	// 2-dim to record required workers, each worker is labeled with partyID, workerID
	var requiredWorkers = make(map[common.PartyIdType]map[common.WorkerIdType]bool)

	// for each party's replied "LaunchResourceReply" struct
	for partyIndex, LaunchResourceReply := range master.RequiredResource {
		partyID := LaunchResourceReply.PartyID

		for workerID, ResourceSVC := range LaunchResourceReply.ResourceSVCs {

			if ResourceSVC.DistributedRole != common.DistributedParameterServer {
				master.workerNum++

				// required workers id
				if _, ok := requiredWorkers[partyID]; !ok {
					requiredWorkers[partyID] = make(map[common.WorkerIdType]bool)
				}
				requiredWorkers[partyID][workerID] = false

				// resourceIps
				if _, ok := executorPairIps[workerID]; !ok {
					executorPairIps[workerID] = make([]string, master.PartyNums)
				}
				executorPairIps[workerID][partyIndex] = ResourceSVC.ResourceIP
				logger.Log.Println("[Master.extractResourceInfo] -----debug-----", executorPairIps)

				// record executor-executor port
				if _, ok := executorPairsPorts[workerID]; !ok {
					executorPairsPorts[workerID] = make([][]common.PortType, master.PartyNums)
				}
				executorPairsPorts[workerID][partyIndex] = ResourceSVC.ExecutorExecutorPort

				// record mpc-mpc port
				if _, ok := mpcPairsPorts[workerID]; !ok {
					mpcPairsPorts[workerID] = make([]common.PortType, master.PartyNums)
				}
				mpcPairsPorts[workerID][partyIndex] = ResourceSVC.MpcMpcPort

				// mpc-executor ports
				if _, ok := mpcExecutorPorts[workerID]; !ok {
					mpcExecutorPorts[workerID] = make([]common.PortType, master.PartyNums)
				}
				mpcExecutorPorts[workerID][partyIndex] = ResourceSVC.MpcExecutorPort
			}

			if ResourceSVC.DistributedRole == common.DistributedParameterServer {
				distPsIp[partyID] = ResourceSVC.ResourceIP
				master.workerNum++

				// record ps-ps ips
				if _, ok := psPairIps[workerID]; !ok {
					psPairIps[workerID] = make([]string, master.PartyNums)
				}
				psPairIps[workerID][partyIndex] = ResourceSVC.ResourceIP

				// record executor-executor port
				if _, ok := psPairsPorts[workerID]; !ok {
					psPairsPorts[workerID] = make([][]common.PortType, master.PartyNums)
				}
				psPairsPorts[workerID][partyIndex] = ResourceSVC.PsPsPorts

			}
			// if it's distributed worker, record worker's ip to generate distributed network alter
			if ResourceSVC.DistributedRole == common.DistributedWorker {
				//which is in order
				distWorkerIps[partyID] = append(distWorkerIps[partyID], ResourceSVC.ResourceIP)
			}
		}

		// get distributed networkCfg used inside each party
		distributedNetworkCfg[partyID] = getDistributedNetworkCfg(distPsIp[partyID], distWorkerIps[partyID])

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

		networkCfg := common.GenerateNetworkConfig(IPs, executorPortArray, mpcPortArray)
		executorPairNetworkCfg[workerID] = networkCfg
	}
	logger.Log.Println("[Master.extractResourceInfo] executorPairNetworkCfg workerID:b64_Str is ", executorPairNetworkCfg)

	// generate network for each parameter server pair
	for workerID, IPs := range psPairIps {

		var psPortArray [][]int32
		var mpcPortArray []int32

		// generate port matrix for exe-exe communication
		psPortArray = make([][]int32, master.PartyNums)
		for i, portArray := range psPairsPorts[workerID] {
			tmp := make([]int32, master.PartyNums)
			for j, port := range portArray {
				tmp[j] = int32(port)
			}
			psPortArray[i] = tmp
		}
		for i := 0; i < int(master.PartyNums); i++ {
			mpcPortArray = append(mpcPortArray, int32(0))
		}

		// generate mpc port array, for each executor
		networkCfg := common.GenerateNetworkConfig(IPs, psPortArray, mpcPortArray)
		executorPairNetworkCfg[workerID] = networkCfg
		logger.Log.Println("[Master.extractResourceInfo] executorPairNetworkCfg including parameter server "+
			"workerID:b64_Str is ", executorPairNetworkCfg)
	}

	// generate network for each mpc pair
	for workerID, ips := range executorPairIps {
		var ports []int32

		for _, port := range mpcPairsPorts[workerID] {
			ports = append(ports, int32(port))
		}

		networkCfg := getMpcMpcNetworkCfg(ips, ports)
		mpcPairNetworkCfg[workerID] = networkCfg
	}
	mpcExecutorNetworkCfg := getMpcExecutorNetworkCfg(mpcExecutorPorts)

	logger.Log.Println("[Master.extractResourceInfo] -----checking configurations-----")
	logger.Log.Printf("[Master.extractResourceInfo] master.RequiredResource is :\n")

	logger.Log.Printf("[Master.extractResourceInfo] workerNum: %d", master.workerNum)

	for i := 0; i < len(master.RequiredResource); i++ {
		logger.Log.Printf("[Master.extractResourceInfo] party %d PartyID: %d", i, master.RequiredResource[i].PartyID)
		logger.Log.Printf("[Master.extractResourceInfo] party %d ResourceNum: %d", i, master.RequiredResource[i].ResourceNum)

		for workerID, svc := range master.RequiredResource[i].ResourceSVCs {
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.WorkerId: %d", i, workerID, svc.WorkerId)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.ResourceIP: %s", i, workerID, svc.ResourceIP)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.WorkerPort: %d", i, workerID, svc.WorkerPort)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.ExecutorExecutorPort: %d", i, workerID, svc.ExecutorExecutorPort)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.MpcMpcPort: %d", i, workerID, svc.MpcMpcPort)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.MpcExecutorPort: %d", i, workerID, svc.MpcExecutorPort)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.ExecutorPSPort: %d", i, workerID, svc.ExecutorPSPort)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.PsExecutorPorts: %v, ResourceSVCs.PsPsPorts: %v", i, workerID, svc.PsExecutorPorts, svc.PsPsPorts)
			logger.Log.Printf("[Master.extractResourceInfo] party %d, workerID %d, ResourceSVCs.DistributedRole: %s\n", i, workerID, common.DistributedRoleToName(svc.DistributedRole))
		}
	}

	logger.Log.Println("[Master.extractResourceInfo] requiredWorkers is ", requiredWorkers)
	logger.Log.Println("[Master.extractResourceInfo] executorPairIps is ", executorPairIps)
	logger.Log.Println("[Master.extractResourceInfo] psPairIps is ", psPairIps)
	logger.Log.Println("[Master.extractResourceInfo] executorPairsPorts is ", executorPairsPorts)
	logger.Log.Println("[Master.extractResourceInfo] mpcPairsPorts is ", mpcPairsPorts)
	logger.Log.Println("[Master.extractResourceInfo] mpcExecutorPorts workerID:[partyPort...] is ", mpcExecutorPorts)
	logger.Log.Println("[Master.extractResourceInfo] distPsIp is ", distPsIp)
	logger.Log.Println("[Master.extractResourceInfo] distWorkerIps is ", distWorkerIps)

	// network configs
	logger.Log.Println("[Master.extractResourceInfo] mpcPairNetworkCfg workerID:partyMpcMpcAddrs is is ", mpcPairNetworkCfg)
	logger.Log.Println("[Master.extractResourceInfo] executorPairNetworkCfg workerID:b64_Str is ", executorPairNetworkCfg)
	logger.Log.Println("[Master.extractResourceInfo] mpcExecutorNetworkCfg workerID:[partyMpcExecutorPort...] is ", mpcExecutorNetworkCfg)
	logger.Log.Println("[Master.extractResourceInfo] distributedNetworkCfg partyID:b64_Str is ", distributedNetworkCfg)

	logger.Log.Println("[Master.extractResourceInfo] -----done with checking configurations-----")

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
	logger.Log.Printf("[Master]: Finish job, update to coord, jobStatus: <%s> \n", master.jobStatus)

	updateStatus(master.jobStatusLog)
	logger.Log.Println("[Master]: Finish job, begin to shutdown master")

	finish()
	logger.Log.Printf("[Master] %s: Thread-4 master.finish, job completed\n", master.Addr)

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

	logger.Log.Println("[Master]: Shutdown server")
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
func getDistributedNetworkCfg(distPsIp string, workerIps []string) string {

	var workerPorts []int32
	var PsPorts []int32
	for i := 0; i < len(workerIps); i++ {
		workerPort := resourcemanager.GetFreePort(1)[0]
		psPort := resourcemanager.GetFreePort(1)[0]
		workerPorts = append(workerPorts, int32(workerPort))
		PsPorts = append(PsPorts, int32(psPort))
	}

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
