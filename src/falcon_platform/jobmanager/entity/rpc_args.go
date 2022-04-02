package entity

import (
	"bytes"
	"encoding/gob"
	"encoding/json"
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"reflect"
	"strconv"
	"strings"
)

// parsed from DslObj, sent to each worker with all related information
// add NetWorkFile, MpcIP, MpcPort
type DslObj4SingleWorker struct {

	// those are the same as TranJob object or DslObj
	JobFlType      string
	ExistingKey    uint
	PartyNums      uint
	Tasks          common.Tasks
	WorkerPreGroup int

	// Only one party's information included
	PartyInfo common.PartyInfo

	// Proto file for falconMl task communication
	ExecutorPairNetworkCfg string

	// distributed training information
	DistributedTask common.DistributedTask

	// Proto file for distributed falconMl task communication
	DistributedExecutorPairNetworkCfg string

	// This is for mpc

	// mpc accepts a file defined as falcon/data/networks/mpc_executor.txt, mpc_mpc_ports_inputs.txt
	// this is the content of the file, each worker will write it to local disk
	MpcPairNetworkCfg string

	MpcExecutorNetworkCfg string
}

// WorkerInfo is the argument passed when a worker registers with the master.
// the worker's UNIX-domain socket name, i.e. its RPC addr
type WorkerInfo struct {
	// this is worker addr
	Addr string
	// the same as the ID of the party
	PartyID common.PartyIdType

	WorkerID common.WorkerIdType

	GroupID common.GroupIdType
}

type ShutdownReply struct {
	// todo, no need to collect shutdown reply message?
}

type DoTaskReply struct {
	// base info of this rpc call
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

type GeneralTask struct {
	TaskName common.FalconTask
	AlgCfg   string `default:""`
}

// Marshal list to string
func MarshalStatus(trainStatuses *DoTaskReply) string {
	jb, e := json.Marshal(trainStatuses)
	if e != nil {
		logger.Log.Fatalln(e)
	}
	return string(jb)
}

func argTypeRegister() {
	gob.Register([]interface{}{})
	gob.Register(map[string]interface{}{})
}

func EncodeWorkerInfo(args *WorkerInfo) string {
	encodedStr := args.Addr + // 0,1
		":" +
		fmt.Sprintf("%d", args.PartyID) + // 2
		":" +
		fmt.Sprintf("%d", args.WorkerID) + // 3
		":" +
		fmt.Sprintf("%d", args.GroupID) // 4

	return encodedStr
}

func DecodeWorkerInfo(encodedStr string) *WorkerInfo {

	workerTmpList := strings.Split(encodedStr, ":")

	tmpAddr := workerTmpList[0] + ":" + workerTmpList[1]
	tmpPartyID, _ := strconv.Atoi(workerTmpList[2])
	tmpWorkerID, _ := strconv.Atoi(workerTmpList[3])
	tmpWorkerGroupID, _ := strconv.Atoi(workerTmpList[4])

	args := new(WorkerInfo)
	args.Addr = tmpAddr
	args.PartyID = common.PartyIdType(tmpPartyID)
	args.WorkerID = common.WorkerIdType(tmpWorkerID)
	args.GroupID = common.GroupIdType(tmpWorkerGroupID)
	return args
}

func EncodeDslObj4SingleWorker(args *DslObj4SingleWorker) []byte {

	argTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(&args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeDslObj4SingleWorker(by []byte) (*DslObj4SingleWorker, error) {
	argTypeRegister()
	reader := bytes.NewReader(by)
	var decoder = gob.NewDecoder(reader)
	var d DslObj4SingleWorker
	err := decoder.Decode(&d)
	return &d, err
}

func EncodeGeneralTask(args *GeneralTask) []byte {

	argTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(&args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeGeneralTask(by []byte) (*GeneralTask, error) {
	argTypeRegister()
	reader := bytes.NewReader(by)
	var decoder = gob.NewDecoder(reader)
	var d GeneralTask
	err := decoder.Decode(&d)
	return &d, err
}

func EncodeDslObj4SingleWorkerGeneral(args interface{}) []byte {

	argTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	switch t := args.(type) {
	default:
		logger.Log.Println(t)
	case *ShutdownReply:
		args = args.(*ShutdownReply)
	case *DslObj4SingleWorker:
		args = args.(*DslObj4SingleWorker)
	case *DoTaskReply:
		args = args.(*DoTaskReply)
	}

	if err := encoder.Encode(args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeDslObj4SingleWorkerGeneral(by []byte, reply interface{}) {

	v := reflect.ValueOf(reply).Elem()

	func(by []byte) {

		argTypeRegister()

		reader := bytes.NewReader(by)

		var decoder = gob.NewDecoder(reader)

		switch t := reply.(type) {
		default:
			logger.Log.Println(t)
		case *ShutdownReply:

			var d ShutdownReply
			err := decoder.Decode(&d)
			if err != nil {
				panic(err)
			}

			var elem = reflect.ValueOf(d)
			var relType = reflect.TypeOf(d)

			for i := 0; i < elem.NumField(); i++ {
				name := relType.Field(i).Name
				v.FieldByName(name).Set(elem.FieldByName(name))
			}
		case *DslObj4SingleWorker:
			var d DslObj4SingleWorker
			err := decoder.Decode(&d)
			if err != nil {
				panic(err)
			}

			var elem = reflect.ValueOf(d)
			var relType = reflect.TypeOf(d)

			for i := 0; i < elem.NumField(); i++ {
				name := relType.Field(i).Name
				v.FieldByName(name).Set(elem.FieldByName(name))
			}
		case *DoTaskReply:
			var d DoTaskReply
			err := decoder.Decode(&d)
			if err != nil {
				panic(err)
			}

			var elem = reflect.ValueOf(d)
			var relType = reflect.TypeOf(d)

			for i := 0; i < elem.NumField(); i++ {
				name := relType.Field(i).Name
				v.FieldByName(name).Set(elem.FieldByName(name))
			}
		}

	}(by)
}
