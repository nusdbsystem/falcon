package entity

import (
	"bytes"
	"encoding/gob"
	"encoding/json"
	"falcon_platform/common"
	fl_comms "falcon_platform/jobmanager/fl_comms_pattern"
	"falcon_platform/logger"
	"fmt"
	"strconv"
	"strings"
)

// WorkerInfo is the argument passed when a worker registers with the master.
// the worker's UNIX-domain socket name, i.e. its RPC addr
type WorkerInfo struct {
	// this is worker addr
	Addr string
	// the same as the ID of the party
	PartyID common.PartyIdType

	WorkerID common.WorkerIdType

	// inter partyID
	PartyIndex int
}

type ShutdownReply struct {
	// todo, no need to collect shutdown reply message?
}

type DoTaskReply struct {
	// rpcbase info of this rpc call
	RpcCallMethod string
	WorkerAddr    string

	// indicate if the job has error
	RuntimeError bool
	// indicate if rpc call meet error
	RpcCallError bool
	// if there are error, store error msg, otherwise empty string
	TaskMsg TaskMsg
}

type TaskMsg struct {
	RuntimeMsg string
	RpcCallMsg string
}

type RetrieveModelReportReply struct {
	DoTaskReply
	ModelReport string
	// if the worker is passive, it's d
	ContainsModelReport bool
}

type TaskContext struct {
	TaskName        common.FalconTask
	FLNetworkConfig *fl_comms.FLNetworkConfig
	Job             *common.TrainJob
	Wk              *WorkerInfo
	MpcAlgName      string
}

// Marshal list to string
func MarshalStatus(trainStatuses *DoTaskReply) string {
	jb, e := json.Marshal(trainStatuses)
	if e != nil {
		logger.Log.Fatalln(e)
	}
	return string(jb)
}

func ArgTypeRegister() {
	gob.Register([]interface{}{})
	gob.Register(map[string]interface{}{})
}

func EncodeWorkerInfo(args *WorkerInfo) string {
	encodedStr := args.Addr + // 0,1
		":" +
		fmt.Sprintf("%d", args.PartyID) + // 2
		":" +
		fmt.Sprintf("%d", args.WorkerID) // 3

	return encodedStr
}

func DecodeWorkerInfo(encodedStr string) *WorkerInfo {

	workerTmpList := strings.Split(encodedStr, ":")

	tmpAddr := workerTmpList[0] + ":" + workerTmpList[1]
	tmpPartyID, _ := strconv.Atoi(workerTmpList[2])
	tmpWorkerID, _ := strconv.Atoi(workerTmpList[3])

	args := new(WorkerInfo)
	args.Addr = tmpAddr
	args.PartyID = common.PartyIdType(tmpPartyID)
	args.WorkerID = common.WorkerIdType(tmpWorkerID)
	return args
}

func SerializeTask(args *TaskContext) []byte {

	ArgTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(&args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DeserializeTask(by []byte) (*TaskContext, error) {
	ArgTypeRegister()
	reader := bytes.NewReader(by)
	var decoder = gob.NewDecoder(reader)
	var d TaskContext
	err := decoder.Decode(&d)
	return &d, err
}
