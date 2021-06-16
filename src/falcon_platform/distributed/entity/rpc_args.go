package entity

import (
	"bytes"
	"encoding/gob"
	"falcon_platform/common"
	"falcon_platform/logger"
	"reflect"
)

type DoTaskArgs struct {
	IP          string
	PartyID     uint
	PartyNums   uint
	JobFlType   string
	PartyInfo   common.PartyInfo
	ExistingKey uint
	TaskInfo    common.Tasks
	NetWorkFile string

	// this 2 for mpc
	MpcIP   string
	MpcPort uint
}

// RegisterArgs is the argument passed when a worker registers with the master.
// the worker's UNIX-domain socket name, i.e. its RPC addr
type RegisterArgs struct {
	// this is worker addr
	WorkerAddr   string
	PartyID      string
	WorkerAddrId string // = WorkerAddr:PartyID
}

type ShutdownReply struct {
	// todo, no need to collect shutdown reply message?
}

type DoTaskReply struct {
	// indicate if the job is killed
	Killed bool

	// indicate if the job has error
	RuntimeError bool

	// indicate if the job has error
	RpcCallError bool

	TaskMsg TaskMsg
}

type TaskMsg struct {
	RuntimeMsg string
	RpcCallMsg string
}

func argTypeRegister() {
	gob.Register([]interface{}{})
	gob.Register(map[string]interface{}{})
}

func EncodeDoTaskArgs(args *DoTaskArgs) []byte {

	argTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	if err := encoder.Encode(&args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeDoTaskArgs(by []byte) *DoTaskArgs {
	argTypeRegister()

	reader := bytes.NewReader(by)

	var decoder = gob.NewDecoder(reader)
	var d DoTaskArgs

	err := decoder.Decode(&d)

	if err != nil {
		panic(err)
	}
	return &d
}

func EncodeDoTaskArgsGeneral(args interface{}) []byte {

	argTypeRegister()

	var buff bytes.Buffer

	var encoder = gob.NewEncoder(&buff)

	switch t := args.(type) {
	default:
		logger.Log.Println(t)
	case *ShutdownReply:
		args = args.(*ShutdownReply)
	case *DoTaskArgs:
		args = args.(*DoTaskArgs)
	case *DoTaskReply:
		args = args.(*DoTaskReply)
	}

	if err := encoder.Encode(args); err != nil {
		panic(err)
	}
	converted := buff.Bytes()
	return converted
}

func DecodeDoTaskArgsGeneral(by []byte, reply interface{}) {

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
		case *DoTaskArgs:
			var d DoTaskArgs
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
