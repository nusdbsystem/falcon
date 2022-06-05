package comms_pattern

import (
	"bytes"
	"encoding/gob"
	"falcon_platform/common"
	"fmt"
)

// PartyRunWorkerReply
// * @Description: partyServer reply this to jobManager,
//				 the struct contains multiple resource and each resource have many services.
// * @File:  job-mngr
// * @Version: 1.0.0
// * @Params:
// * @Date: 23/08/21 1:50
type PartyRunWorkerReply struct {
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
	// network config pattern for a specific job.
	JobNetCfg []byte
}

func (rs *ResourceSVC) ToAddr(port common.PortType) (address string) {
	return rs.ResourceIP + ":" + fmt.Sprintf("%d", port)
}

type RunWorkerReply struct {
	EncodedStr []byte
}

func EncodePartyRunWorkerReply(args *PartyRunWorkerReply) []byte {
	var buff bytes.Buffer
	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodePartyRunWorkerReply(by []byte) *PartyRunWorkerReply {
	reader := bytes.NewReader(by)
	var decoder = gob.NewDecoder(reader)
	var d PartyRunWorkerReply
	err := decoder.Decode(&d)
	if err != nil {
		panic(err)
	}
	return &d
}

func EncodeFLNetworkCfgPerParty(args *FLNetworkCfgPerParty) []byte {
	var buff bytes.Buffer
	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeFLNetworkCfgPerParty(by []byte) *FLNetworkCfgPerParty {
	reader := bytes.NewReader(by)
	var decoder = gob.NewDecoder(reader)
	var d FLNetworkCfgPerParty
	err := decoder.Decode(&d)
	if err != nil {
		panic(err)
	}
	return &d
}
