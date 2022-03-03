package common

import (
	"bytes"
	"encoding/gob"
	"fmt"
)

/**
 * @Description: partyServer reply this to jobManager,
				 the struct contains multiple resource and each resource have many services.
 * @File:  job-mngr
 * @Version: 1.0.0
 * @Params:
 * @Date: 23/08/21 1:50
*/
type LaunchResourceReply struct {

	// partyID
	PartyID PartyIdType

	// how many resources created by this partyServer
	ResourceNum int

	// key workerID, value: *ResourceSVC
	ResourceSVCs map[WorkerIdType]*ResourceSVC

	//group number, how many worker groups in this partyServer
	GroupNum int

	//group number, how many workers in this group
	ResourceNumPreGroup int
}

type ResourceSVC struct {
	// worker id
	WorkerId WorkerIdType

	// group id, this is used by lime, lime can be trained in distributed way
	GroupId GroupIdType

	// Ip of the service running in this resource
	ResourceIP string

	// Each worker's port, listen master's requests
	WorkerPort PortType

	// Each Executor's port array, i-th element is listening requests from i-th party's executor
	ExecutorExecutorPort []PortType

	// Each Mpc listen two ports,
	// mpc port1, listen requests from other party's mpc
	MpcMpcPort PortType
	// mpc port2, listen requests from other party's executor
	MpcExecutorPort PortType

	// used in distributed training

	// if this worker is Executor, this port listen requests sent from current party's parameter server
	ExecutorPSPort PortType // workerId: Executor port
	// if this worker is parameter server, listen requests sent from current party's executor
	PsExecutorPorts []PortType

	// each worker will spawn a subprocess, which can be a train-worker or a parameter server
	DistributedRole uint
}

func (rs *ResourceSVC) ToAddr(port PortType) (address string) {
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
