package comms_pattern

import (
	"falcon_platform/common"
	"log"
)

// PartyNetworkConfig of each party
type PartyNetworkConfig interface {
	Constructor(partyNum int, workerId common.WorkerIdType, taskClassIDName string, role int, workerNum int) []byte
}

// JobNetworkConfig for each job
type JobNetworkConfig interface {
	Constructor(encodeStr [][]byte, PartyNums uint, masLogger *log.Logger)

	//GetPartyIdToIndex each job has a mapper to map each partyId to internel partyIndex
	GetPartyIdToIndex() map[common.PartyIdType]int

	//GetRequiredWorkers each job has a required workers, labelled with partyID-workerID.
	GetRequiredWorkers() map[common.PartyIdType]map[common.WorkerIdType]bool

	// SerializeNetworkCfg serialize this instance
	SerializeNetworkCfg() string
}
