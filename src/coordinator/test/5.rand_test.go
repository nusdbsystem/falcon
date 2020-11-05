package test

import (
	"coordinator/distributed/entitiy"
	"fmt"
	"testing"
)

func TestRandomFunc(t *testing.T) {

	a := entitiy.ShutdownReply{Ntasks: 123}

	res := entitiy.EncodeDoTaskArgsGeneral(&a)
	fmt.Println(res)

	rr := entitiy.ShutdownReply{}
	entitiy.DecodeDoTaskArgsGeneral(res, &rr)
	fmt.Println(rr.Ntasks)

}
