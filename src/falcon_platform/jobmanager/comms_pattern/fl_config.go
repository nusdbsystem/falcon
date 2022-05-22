package comms_pattern

import (
	b64 "encoding/base64"
	"falcon_platform/common"
	v0 "falcon_platform/common/proto/v0"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"google.golang.org/protobuf/proto"
	"log"
	"sort"
	"strings"
)

// init register all network config pattern for each job.
func init() {
	GetAllNetworkCfg()["vertical"] = new(FLNetworkCfgPerParty)
	GetJobNetCfg()["vertical"] = new(FLNetworkCfg)
}

// FLNetworkCfgPerParty Each worker will has following cfg under FL task
type FLNetworkCfgPerParty struct {

	// Each Executor's port array, i-th element is listening requests from i-th party's executor
	ExecutorExecutorPort []common.PortType

	// Each Mpc listen two ports,
	// mpc port1, listening requests from other party's mpc
	MpcMpcPort common.PortType
	// mpc port2, listening requests from other party's executor
	MpcExecutorPort common.PortType

	// used in distributed training

	// if this worker is Executor, this port listen requests sent from current party's parameter server
	ExecutorPSPort common.PortType // workerId: Executor port
	// if this worker is parameter server, listen requests sent from current party's executor
	PsExecutorPorts []common.PortType

	// each worker will spawn a subprocess, which can be a train-worker or a parameter server
	DistributedRole uint
}

func (this *FLNetworkCfgPerParty) Constructor(partyNum int, workerId common.WorkerIdType, taskClassIDName string,
	role int, workerNum int) PartyNetworkConfig {

	newCfg := new(FLNetworkCfgPerParty)

	if role == common.CentralizedWorker {
		newCfg.ExecutorExecutorPort = resourcemanager.GetFreePort(partyNum)
		newCfg.MpcMpcPort = resourcemanager.GetOneFreePort()
		newCfg.MpcExecutorPort = resourcemanager.GetMpcExecutorPort(int(workerId), taskClassIDName)
		newCfg.ExecutorPSPort = 0
		newCfg.PsExecutorPorts = []common.PortType{}
		//resourceSVC.PsPsPorts = []common.PortType{}
		newCfg.DistributedRole = common.CentralizedWorker
		return newCfg
	}

	if role == common.DistributedParameterServer {
		newCfg.ExecutorExecutorPort = resourcemanager.GetFreePort(partyNum)
		newCfg.MpcMpcPort = resourcemanager.GetOneFreePort()
		newCfg.MpcExecutorPort = resourcemanager.GetMpcExecutorPort(0, taskClassIDName)
		newCfg.ExecutorPSPort = 0
		newCfg.PsExecutorPorts = resourcemanager.GetFreePort(workerNum - 1)
		newCfg.DistributedRole = common.DistributedParameterServer
		return newCfg
	}

	if role == common.DistributedWorker {
		newCfg.ExecutorExecutorPort = resourcemanager.GetFreePort(partyNum)
		newCfg.MpcMpcPort = resourcemanager.GetOneFreePort()
		newCfg.MpcExecutorPort = resourcemanager.GetMpcExecutorPort(int(workerId), taskClassIDName)
		newCfg.ExecutorPSPort = resourcemanager.GetOneFreePort()
		newCfg.PsExecutorPorts = []common.PortType{}
		newCfg.DistributedRole = common.DistributedWorker
		return newCfg
	}
	panic("Generatt FLNetworkCfgPerParty fail, role not match any ")
}

// FLNetworkCfg aggregate from all party, and generate this summarty.
type FLNetworkCfg struct {
	// each worker or ps has a mpc, and they will communicate with each other
	MpcPairNetworkCfg        map[common.WorkerIdType]string
	ExecutorPairNetworkCfg   map[common.WorkerIdType]string
	MpcExecutorNetworkCfg    map[common.WorkerIdType][]string
	MpcExecutorNetworkCfgMap map[common.WorkerIdType]map[common.PartyIdType]common.PortType
	DistributedNetworkCfg    map[common.PartyIdType]string
	RequiredWorkers          map[common.PartyIdType]map[common.WorkerIdType]bool

	// dist, ps, worker, centralized worker
	WorkerRole map[common.PartyIdType]map[common.WorkerIdType]uint

	// map
	PartyIdToIndex map[common.PartyIdType]int
}

func (this *FLNetworkCfg) Constructor(encodeStr [][]byte, PartyNums uint, masLogger *log.Logger) {

	var requiredResource []*PartyRunWorkerReply
	for _, ele := range encodeStr {
		requiredResource = append(requiredResource, DecodePartyRunWorkerReply(ele))
	}

	this.PartyIdToIndex = make(map[common.PartyIdType]int)
	this.WorkerRole = make(map[common.PartyIdType]map[common.WorkerIdType]uint)

	// key: workerID, matching worker of different party has same id

	// [ workerID: [ ip1, ip2...] ]
	var executorPairIps = make(map[common.WorkerIdType][]string)
	// [ workerID: port matrix ]
	var executorPairsPorts = make(map[common.WorkerIdType][][]common.PortType)

	// [ workerID: port matrix ]
	var mpcPairsPorts = make(map[common.WorkerIdType][]common.PortType)
	var mpcExecutorPorts = make(map[common.WorkerIdType][]common.PortType)

	var distPsIp = make(map[common.PartyIdType]string)
	var distPsPorts = make(map[common.PartyIdType][]common.PortType)

	// worker address and port of one party
	var distWorkerIps = make(map[common.PartyIdType][]string)
	var distWorkerPorts = make(map[common.PartyIdType][]common.PortType)

	// each matching pair, have 3 network cfg, if there is ps, 4 network configs
	var executorPairNetworkCfg = make(map[common.WorkerIdType]string)
	var mpcPairNetworkCfg = make(map[common.WorkerIdType]string)
	var distributedNetworkCfg = make(map[common.PartyIdType]string)

	// 2-dim to record required workers, each worker is labeled with partyID, workerID
	var requiredWorkers = make(map[common.PartyIdType]map[common.WorkerIdType]bool)

	// for each party's replied "PartyRunWorkerReply" struct
	for partyIndex, PartyRunWorkerReply := range requiredResource {
		partyID := PartyRunWorkerReply.PartyID
		this.PartyIdToIndex[partyID] = partyIndex
		// there are only 1 ps in each party's group in distributed train
		trainWorkerPreParty := PartyRunWorkerReply.ResourceNum - 1

		// get sorted work id
		var orderedWorkIds []int
		for workerID, _ := range PartyRunWorkerReply.ResourceSVCs {
			orderedWorkIds = append(orderedWorkIds, int(workerID))
		}
		sort.Ints(orderedWorkIds)

		// for each resource in single party, force counting start from 0
		for _, orderedWorkId := range orderedWorkIds {
			// gather all resource
			for workerID, ResourceSVC := range PartyRunWorkerReply.ResourceSVCs {
				if orderedWorkId == int(workerID) {

					flConfig := ResourceSVC.JobNetCfg.(*FLNetworkCfgPerParty)

					// store role of each worker
					if _, ok := this.WorkerRole[partyID][workerID]; !ok {
						this.WorkerRole[partyID] = make(map[common.WorkerIdType]uint)
					}
					this.WorkerRole[partyID][workerID] = flConfig.DistributedRole

					// required workers id
					if _, ok := requiredWorkers[partyID]; !ok {
						requiredWorkers[partyID] = make(map[common.WorkerIdType]bool)
					}
					requiredWorkers[partyID][workerID] = false

					// resource executor-executor ops
					if _, ok := executorPairIps[workerID]; !ok {
						executorPairIps[workerID] = make([]string, PartyNums)
					}
					executorPairIps[workerID][partyIndex] = ResourceSVC.ResourceIP

					// record executor-executor ports
					if _, ok := executorPairsPorts[workerID]; !ok {
						executorPairsPorts[workerID] = make([][]common.PortType, PartyNums)
					}
					executorPairsPorts[workerID][partyIndex] = flConfig.ExecutorExecutorPort

					// record mpc-mpc ports
					if _, ok := mpcPairsPorts[workerID]; !ok {
						mpcPairsPorts[workerID] = make([]common.PortType, PartyNums)
					}
					mpcPairsPorts[workerID][partyIndex] = flConfig.MpcMpcPort

					// mpc-executor ports
					if _, ok := mpcExecutorPorts[workerID]; !ok {
						mpcExecutorPorts[workerID] = make([]common.PortType, PartyNums)
					}
					mpcExecutorPorts[workerID][partyIndex] = flConfig.MpcExecutorPort

					// for distributed parameter
					if flConfig.DistributedRole == common.DistributedParameterServer {
						distPsIp[partyID] = ResourceSVC.ResourceIP
						if _, ok := distPsPorts[partyID]; !ok {
							// len = number of train workers
							distPsPorts[partyID] = make([]common.PortType, trainWorkerPreParty)
						}
						for wIndex, port := range flConfig.PsExecutorPorts {
							distPsPorts[partyID][wIndex] = port
						}
					}

					// if it's distributed worker, record worker's ip to generate distributed network alter
					if flConfig.DistributedRole == common.DistributedWorker {
						//which is in order
						distWorkerIps[partyID] = append(distWorkerIps[partyID], ResourceSVC.ResourceIP)
						distWorkerPorts[partyID] = append(distWorkerPorts[partyID], flConfig.ExecutorPSPort)
					}
					break
				}
			}
		}

		// get distributed networkCfg used inside each party
		distributedNetworkCfg[partyID] = getDistributedNetworkCfg(
			distPsIp[partyID], distPsPorts[partyID],
			distWorkerIps[partyID], distWorkerPorts[partyID])

		masLogger.Println("[Master.extractResourceInfo], result of getDistributedNetworkCfg when partyID =",
			partyID,
			"distPsIp: ", distPsIp[partyID],
			"distPsPorts: ", distPsPorts[partyID],
			"distWorkerIps: ", distWorkerIps[partyID],
			"distWorkerPorts: ", distWorkerPorts[partyID],
			"distributedNetworkCfg: ", distributedNetworkCfg[partyID])
		retrieveDistributedNetworkConfig(distributedNetworkCfg[partyID])
	}

	// generate network for each executor pair
	for workerID, IPs := range executorPairIps {

		var executorPortArray [][]int32
		var mpcPortArray []int32

		// generate port matrix for exe-exe communication
		executorPortArray = make([][]int32, PartyNums)
		for i, portArray := range executorPairsPorts[workerID] {
			tmp := make([]int32, PartyNums)
			for j, port := range portArray {
				tmp[j] = int32(port)
			}
			executorPortArray[i] = tmp
		}

		// generate mpc port array, for each executor
		for _, port := range mpcExecutorPorts[workerID] {
			mpcPortArray = append(mpcPortArray, int32(port))
		}
		executorPairNetworkCfg[workerID] = generateNetworkConfig(IPs, executorPortArray, mpcPortArray)

		// generate network for each mpc pair
		var portsMpc []int32
		for _, port := range mpcPairsPorts[workerID] {
			portsMpc = append(portsMpc, int32(port))
		}
		mpcPairNetworkCfg[workerID] = getMpcMpcNetworkCfg(IPs, portsMpc)
	}

	masLogger.Println("[Master.extractResourceInfo] executorPairNetworkCfg workerID:b64_Str is ", executorPairNetworkCfg)

	mpcExecutorNetworkCfg := getMpcExecutorNetworkCfg(mpcExecutorPorts)

	masLogger.Println("[Master.extractResourceInfo] -----checking configurations-----")
	masLogger.Printf("[Master.extractResourceInfo] requiredResource is :\n")

	for i := 0; i < len(requiredResource); i++ {
		masLogger.Printf("[Master.extractResourceInfo] party %d PartyID: %d", i, requiredResource[i].PartyID)
		masLogger.Printf("[Master.extractResourceInfo] party %d ResourceNum: %d", i, requiredResource[i].ResourceNum)

		for workerID, svc := range requiredResource[i].ResourceSVCs {
			flConfig := svc.JobNetCfg.(*FLNetworkCfgPerParty)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.WorkerId: %d", i, workerID, svc, svc.WorkerId)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d", i, workerID, svc)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.ResourceIP: %s", i, workerID, svc, svc.ResourceIP)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.WorkerPort: %d", i, workerID, svc, svc.WorkerPort)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.ExecutorExecutorPort: %d", i, workerID, svc, flConfig.ExecutorExecutorPort)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.MpcMpcPort: %d", i, workerID, svc, flConfig.MpcMpcPort)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.MpcExecutorPort: %d", i, workerID, svc, flConfig.MpcExecutorPort)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.ExecutorPSPort: %d", i, workerID, svc, flConfig.ExecutorPSPort)
			masLogger.Printf("[Master.extractResourceInfo] party %d, workerID %d, %d, ResourceSVCs.DistributedRole: %s\n", i, workerID, svc, common.DistributedRoleToName(flConfig.DistributedRole))
		}
	}

	masLogger.Println("[Master.extractResourceInfo] requiredWorkers is ", requiredWorkers)
	masLogger.Println("[Master.extractResourceInfo] executorPairIps is ", executorPairIps)
	masLogger.Println("[Master.extractResourceInfo] executorPairsPorts is ", executorPairsPorts)
	masLogger.Println("[Master.extractResourceInfo] mpcPairsPorts is ", mpcPairsPorts)
	masLogger.Println("[Master.extractResourceInfo] mpcExecutorPorts workerID:[partyPort...] is ", mpcExecutorPorts)
	masLogger.Println("[Master.extractResourceInfo] distPsIp is ", distPsIp)
	masLogger.Println("[Master.extractResourceInfo] distWorkerIps is ", distWorkerIps)

	// network configs
	masLogger.Println("[Master.extractResourceInfo] mpcPairNetworkCfg workerID:partyMpcMpcAddrs is is ", mpcPairNetworkCfg)
	masLogger.Println("[Master.extractResourceInfo] executorPairNetworkCfg workerID:b64_Str is ", executorPairNetworkCfg)
	masLogger.Println("[Master.extractResourceInfo] mpcExecutorNetworkCfg workerID:[partyMpcExecutorPort...] is ", mpcExecutorNetworkCfg)
	masLogger.Println("[Master.extractResourceInfo] distributedNetworkCfg partyID:b64_Str is ", distributedNetworkCfg)

	masLogger.Println("[Master.extractResourceInfo] -----done with checking configurations-----")

	this.MpcPairNetworkCfg = mpcPairNetworkCfg
	this.ExecutorPairNetworkCfg = executorPairNetworkCfg
	this.MpcExecutorNetworkCfg = mpcExecutorNetworkCfg
	this.DistributedNetworkCfg = distributedNetworkCfg
	this.RequiredWorkers = requiredWorkers
}

func (this *FLNetworkCfg) GetPartyIdToIndex() map[common.PartyIdType]int {
	return this.PartyIdToIndex
}

func (this *FLNetworkCfg) GetRequiredWorkers() map[common.PartyIdType]map[common.WorkerIdType]bool {
	return this.RequiredWorkers
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

	netCfg := generateDistributedNetworkConfig(
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

// generateNetworkConfig executorPortArray : 2-dim array, r1 demote receiver 1's port,
//	   [ [na, r2, r3],
//		 [r1, na, r3 ],
//		 [r1, r2, na ] ]
// mpcPortArray: each party's mpc port
func generateNetworkConfig(mIps []string, executorPortArray [][]int32, mpcPortArray []int32) string {

	//logger.Log.Println("Scheduler: Assigned IP and ports are: ", Addresses, portArray)

	partyNums := len(mIps)
	var IPs []string
	for _, v := range mIps {
		IPs = append(IPs, strings.Split(v, ":")[0])
	}

	cfg := v0.NetworkConfig{
		Ips:                        IPs,
		ExecutorExecutorPortArrays: []*v0.PortArray{},
		ExecutorMpcPortArray:       nil,
	}

	// for each IP Url
	for i := 0; i < partyNums; i++ {
		p := &v0.PortArray{Ports: executorPortArray[i]}
		cfg.ExecutorExecutorPortArrays = append(cfg.ExecutorExecutorPortArrays, p)
	}

	// each executor knows other mpc's port
	cfg.ExecutorMpcPortArray = &v0.PortArray{Ports: mpcPortArray}

	out, err := proto.Marshal(&cfg)
	if err != nil {
		logger.Log.Println("Generate NetworkCfg failed ", err)
		panic(err)
	}
	return b64.StdEncoding.EncodeToString(out)

}

func generateDistributedNetworkConfig(psIp string, psPort []int32, workerIps []string, workerPorts []int32) string {

	// each port corresponding to one worker
	var psWorkers []*v0.PS
	for i := 0; i < len(psPort); i++ {
		ps := v0.PS{
			PsIp:   psIp,
			PsPort: psPort[i],
		}
		psWorkers = append(psWorkers, &ps)
	}

	//  worker address
	var workers []*v0.Worker
	for i := 0; i < len(workerIps); i++ {
		worker := v0.Worker{
			WorkerIp:   workerIps[i],
			WorkerPort: workerPorts[i]}
		workers = append(workers, &worker)
	}
	//  network config
	psCfg := v0.PSNetworkConfig{
		Ps:      psWorkers,
		Workers: workers,
	}

	out, err := proto.Marshal(&psCfg)
	if err != nil {
		logger.Log.Println("Generate Distributed NetworkCfg failed ", err)
		panic(err)
	}

	return b64.StdEncoding.EncodeToString(out)
}

func retrieveDistributedNetworkConfig(a string) {

	res, _ := b64.StdEncoding.DecodeString(a)

	cfg := v0.PSNetworkConfig{}

	_ = proto.Unmarshal(res, &cfg)

	logger.Log.Println("[retrieveDistributedNetworkConfig]: cfg.Ps", cfg.Ps)
	logger.Log.Println("[retrieveDistributedNetworkConfig]: cfg.Workers", cfg.Workers)

}

func retrieveNetworkConfig(a string) {
	res, _ := b64.StdEncoding.DecodeString(a)

	cfg := v0.NetworkConfig{}

	_ = proto.Unmarshal(res, &cfg)

	logger.Log.Println("[retrieveNetworkConfig]: cfg.Ips", cfg.Ips)
	logger.Log.Println("[retrieveNetworkConfig]: cfg.ExecutorExecutorPortArrays", cfg.ExecutorExecutorPortArrays)
	logger.Log.Println("[retrieveNetworkConfig]: cfg.ExecutorMpcPortArray", cfg.ExecutorMpcPortArray)

}
