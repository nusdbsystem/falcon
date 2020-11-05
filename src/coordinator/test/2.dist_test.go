package test

import (
	"coordinator/distributed/utils"
	"fmt"
	"testing"
)

func TestDist(t *testing.T) {

	res, _ := utils.GetFreePort()
	res2, _ := utils.GetFreePorts(3)
	fmt.Println(res, res2)
}
