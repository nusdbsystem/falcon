package cache

import (
	"bytes"
	"coordinator/common"
	"encoding/gob"
)

type QItem struct {
	IPs       		[]string
	JobId     		uint
	JobName    		string
	JobFlType  		string
	ExistingKey  	uint
	PartyNums  		uint
	PartyInfos 		[]common.PartyInfo
	Tasks      		common.Tasks
}

func argTypeRegister() {
	gob.Register([]interface{}{})
	gob.Register(map[string]interface{}{})
}

func Serialize(qItem *QItem) string{
	argTypeRegister()
	var buffer bytes.Buffer

	var encoder = gob.NewEncoder(&buffer)

	if err := encoder.Encode(qItem); err!=nil{
		panic(err)
	}
	return string(buffer.Bytes())

}


func Deserialize(qs string) (qItem *QItem){
	argTypeRegister()
	reader := bytes.NewReader([]byte(qs))

	var decoder = gob.NewDecoder(reader)
	var d QItem

	err := decoder.Decode(&d)

	if err != nil {
		panic(err)
	}
	return &d
}