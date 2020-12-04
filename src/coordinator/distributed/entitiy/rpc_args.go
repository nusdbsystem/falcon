package entitiy

import (
	"bytes"
	"coordinator/common"
	"coordinator/logger"
	"encoding/gob"
	"reflect"
)

type DoTaskArgs struct {
	IP             string
	PartyPath      common.PartyPath
	TaskInfos      common.Tasks
	ModelPath      []string
	ExecutablePath []string
}

// RegisterArgs is the argument passed when a worker registers with the master.
// the worker's UNIX-domain socket name, i.e. its RPC address
type RegisterArgs struct {
	WorkerAddr string
}

type ShutdownReply struct {
	Ntasks int
}

type DoTaskReply struct {
	Killed  bool
	Errs    map[string]string
	ErrLogs map[string]string
	OutLogs map[string]string
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
		logger.Do.Println(t)
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
			logger.Do.Println(t)
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
