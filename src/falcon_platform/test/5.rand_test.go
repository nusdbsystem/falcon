package test

import (
	"encoding/json"
	"falcon_platform/common"
	"falcon_platform/exceptions"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"fmt"
	"log"
	"testing"

	"google.golang.org/protobuf/proto"
)

func TestRandomFunc(t *testing.T) {

	a := entity.ShutdownReply{}

	res := entity.EncodeDslObj4SinglePartyGeneral(&a)
	logger.Log.Println(res)

	rr := entity.ShutdownReply{}
	entity.DecodeDslObj4SinglePartyGeneral(res, &rr)
	logger.Log.Println()

}

type student struct {
	//tagjson序列化后是小写
	Name   string `json:"name"`
	Age    int    `json:"age"`
	Hobbit string `json:"hobbit"`
}

func unjsonSlice() {
	var sli []student
	fmt.Println("unjson before： ", sli)
	var str = "[{\"addr\":\"beijing111\",\"age\":1231,\"name\":\"typ111\"},{\"addr\":\"beijing222\",\"age\":1222," +
		"\"name\":\"typ222\"}]"
	err := json.Unmarshal([]byte(str), &sli)
	if err != nil {
		fmt.Println("unjson 失败")
	}
	fmt.Println("unjson after： ", sli)
}

func TestRandomFunc2(t *testing.T) {

	p := common.NetworkConfig{
		IPs:        []string{"1", "123", "123", "3"},
		PortArrays: []*common.PortArray{},
	}

	p1 := &common.PortArray{Ports: []int32{1, 2, 34, 5}}
	p2 := &common.PortArray{Ports: []int32{1, 2, 34, 5}}
	p3 := &common.PortArray{Ports: []int32{1, 2, 34, 5}}

	p.PortArrays = append(p.PortArrays, p1)
	p.PortArrays = append(p.PortArrays, p2)
	p.PortArrays = append(p.PortArrays, p3)

	out, err := proto.Marshal(&p)
	if err != nil {
		log.Fatalln("Failed to encode addr book:", err)
	}
	fmt.Println(string(out))

	px := &common.NetworkConfig{}
	if err := proto.Unmarshal(out, px); err != nil {
		log.Fatalln("Failed to parse addr book:", err)
	}

	fmt.Println(px)

}

func TestErrors(t *testing.T) {

	if exceptions.CallingError().Error() == exceptions.CallingErr {
		fmt.Println("asdf")
	}

}
