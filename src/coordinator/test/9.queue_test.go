package test

import (
	"coordinator/cache"
	"fmt"
	"testing"
)

func TestQueue(t *testing.T) {

	a := cache.QItem{}

	res := cache.Serialize(&a)

	res2 := cache.Deserialize(res)

	fmt.Println(res)

	fmt.Println("---------------------------------------------------------------")

	fmt.Println(res2.IPs)
	fmt.Println(res2.JobId)
	fmt.Println(res2.PartyPath)
	fmt.Println(res2.TaskInfos)

}
