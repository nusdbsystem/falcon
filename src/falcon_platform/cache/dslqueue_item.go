package cache

import (
	"bytes"
	"encoding/gob"
	"falcon_platform/common"
)

// parsed from TranJob object, add PartyAddrList and JobId
type DslObj struct {

	// party-addr [ip+port..]
	PartyAddrList []string
	// train job_id, or inference jobId, index of the job in db,
	JobId uint

	// those are the same as TranJob object
	JobName         string // party-urls [ip+port..]
	JobFlType       string
	ExistingKey     uint
	PartyNums       uint
	DistributedTask common.DistributedTask
	PartyInfoList   []common.PartyInfo
	Tasks           common.Tasks
}

func argTypeRegister() {
	gob.Register([]interface{}{})
	gob.Register(map[string]interface{}{})
}

func Serialize(dslOjb *DslObj) string {
	argTypeRegister()
	var buffer bytes.Buffer

	var encoder = gob.NewEncoder(&buffer)

	if err := encoder.Encode(dslOjb); err != nil {
		panic(err)
	}
	return string(buffer.Bytes())
}

func Deserialize(qs string) (dslOjb *DslObj) {
	argTypeRegister()
	reader := bytes.NewReader([]byte(qs))

	var decoder = gob.NewDecoder(reader)
	var d DslObj

	err := decoder.Decode(&d)

	if err != nil {
		panic(err)
	}
	return &d
}
