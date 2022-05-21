package fl_comms_pattern

import (
	"bytes"
	"encoding/gob"
	"falcon_platform/common"
	"fmt"
)

// LaunchResourceReply
// * @Description: partyServer reply this to jobManager,
//				 the struct contains multiple resource and each resource have many services.
// * @File:  job-mngr
// * @Version: 1.0.0
// * @Params:
// * @Date: 23/08/21 1:50
type LaunchResourceReply struct {

	// partyID
	PartyID common.PartyIdType

	// how many resources created by this partyServer
	ResourceNum int

	// key workerID, value: *ResourceSVC
	ResourceSVCs map[common.WorkerIdType]*ResourceSVC
}

type ResourceSVC struct {
	// worker id
	WorkerId common.WorkerIdType

	// Ip of the service running in this resource
	ResourceIP string

	// Each worker's port, listen master's requests
	WorkerPort common.PortType

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

func (rs *ResourceSVC) ToAddr(port common.PortType) (address string) {
	return rs.ResourceIP + ":" + fmt.Sprintf("%d", port)
}

type RunWorkerReply struct {
	EncodedStr []byte
}

func EncodeLaunchResourceReply(args *LaunchResourceReply) []byte {

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeLaunchResourceReply(by []byte) *LaunchResourceReply {
	reader := bytes.NewReader(by)
	var decoder = gob.NewDecoder(reader)
	var d LaunchResourceReply
	err := decoder.Decode(&d)
	if err != nil {
		panic(err)
	}
	return &d
}
