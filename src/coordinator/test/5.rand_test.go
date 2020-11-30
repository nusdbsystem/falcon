package test

import (
	"coordinator/distributed/entitiy"
	"encoding/json"
	"fmt"
	"log"
	"testing"
)

func TestRandomFunc(t *testing.T) {

	a := entitiy.ShutdownReply{Ntasks: 123}

	res := entitiy.EncodeDoTaskArgsGeneral(&a)
	log.Println(res)

	rr := entitiy.ShutdownReply{}
	entitiy.DecodeDoTaskArgsGeneral(res, &rr)
	log.Println(rr.Ntasks)

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
	svcName := "asdf"
	fmt.Println(svcName+".DoTask")


}
