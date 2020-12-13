package test

import (
	"coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/logger"
	"encoding/json"
	"fmt"
	"google.golang.org/protobuf/proto"
	"log"
	"testing"
)

func TestRandomFunc(t *testing.T) {

	a := entitiy.ShutdownReply{Ntasks: 123}

	res := entitiy.EncodeDoTaskArgsGeneral(&a)
	logger.Do.Println(res)

	rr := entitiy.ShutdownReply{}
	entitiy.DecodeDoTaskArgsGeneral(res, &rr)
	logger.Do.Println(rr.Ntasks)

}

type student struct {
	//tagjson序列化后是小写
	Name string 	`json:"name"`
	Age int			`json:"age"`
	Hobbit string	`json:"hobbit"`
}


func unjsonSlice(){
	var sli  []student
	fmt.Println("unjson before： ",sli)
	var str = "[{\"addr\":\"beijing111\",\"age\":1231,\"name\":\"typ111\"},{\"addr\":\"beijing222\",\"age\":1222," +
		"\"name\":\"typ222\"}]"
	err := json.Unmarshal([]byte(str), &sli)
	if err != nil {
		fmt.Println("unjson 失败")
	}
	fmt.Println("unjson after： ",sli)
}


func TestRandomFunc2(t *testing.T) {


	p := common.NetworkConfig{
		Ip:    []string{"1","123","123","3"},
		Port:  []*common.Port{},
	}

	p1 := &common.Port{Port: []int32{1,2,34,5}}
	p2 := &common.Port{Port: []int32{1,2,34,5}}
	p3 := &common.Port{Port: []int32{1,2,34,5}}

	p.Port = append(p.Port, p1)
	p.Port = append(p.Port, p2)
	p.Port = append(p.Port, p3)

	out, err := proto.Marshal(&p)
	if err != nil {
		log.Fatalln("Failed to encode address book:", err)
	}
	fmt.Println(string(out))

	px := &common.NetworkConfig{}
	if err := proto.Unmarshal(out, px); err != nil {
		log.Fatalln("Failed to parse address book:", err)
	}

	fmt.Println(px)

}
