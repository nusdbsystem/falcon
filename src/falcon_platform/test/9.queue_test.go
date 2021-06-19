package test

import (
	"falcon_platform/cache"
	"fmt"
	"testing"
)

func TestDslQueue(t *testing.T) {

	a := cache.DslObj{}

	res := cache.Serialize(&a)

	res2 := cache.Deserialize(res)

	fmt.Println(res)

	fmt.Println("---------------------------------------------------------------")

	fmt.Println(res2.PartyAddrList)
	fmt.Println(res2.JobId)

}
