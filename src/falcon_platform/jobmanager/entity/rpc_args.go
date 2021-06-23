package entity

import (
	"bytes"
	"encoding/gob"
	"encoding/json"
	"falcon_platform/common"
	"falcon_platform/logger"
	"reflect"
)

// parsed from DslObj, sent to each worker with all related information
// add NetWorkFile, MpcIP, MpcPort
type DslObj4SingleParty struct {

	// those are the same as TranJob object or DslObj
	JobFlType   string
	ExistingKey uint
	PartyNums   uint
	Tasks       common.Tasks

	// Only one party's information included
	PartyInfo common.PartyInfo

	// Proto file for falconMl task communication
	NetWorkFile string

	// This 2 for mpc
	// all mpc process share's party-0's ip
	MpcIP string
	// all mpc process use the same port
	MpcPort uint
}

// RegisterArgs is the argument passed when a worker registers with the master.
// the worker's UNIX-domain socket name, i.e. its RPC addr
type RegisterArgs struct {
	// this is worker addr
	WorkerAddr string
	// the same as the ID of the party
	WorkerID string
}

type ShutdownReply struct {
	// todo, no need to collect shutdown reply message?
}

type DoTaskReply struct {
	// base info of this rpc call
	RpcCallMethod string
	WorkerUrl     string

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

func EncodeDslObj4SingleParty(args *DslObj4SingleParty) []byte {

	argTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(&args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeDslObj4SingleParty(by []byte) (*DslObj4SingleParty, error) {
	argTypeRegister()
	reader := bytes.NewReader(by)
	var decoder = gob.NewDecoder(reader)
	var d DslObj4SingleParty
	err := decoder.Decode(&d)
	return &d, err
}

func EncodeDslObj4SinglePartyGeneral(args interface{}) []byte {

	argTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	switch t := args.(type) {
	default:
		logger.Log.Println(t)
	case *ShutdownReply:
		args = args.(*ShutdownReply)
	case *DslObj4SingleParty:
		args = args.(*DslObj4SingleParty)
	case *DoTaskReply:
		args = args.(*DoTaskReply)
	}

	if err := encoder.Encode(args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeDslObj4SinglePartyGeneral(by []byte, reply interface{}) {

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
		case *DslObj4SingleParty:
			var d DslObj4SingleParty
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
