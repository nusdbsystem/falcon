package test

import (
	"coordinator/distributed/utils"
	"log"
	"testing"
)

func TestDist(t *testing.T) {

	res, _ := utils.GetFreePort()
	res2, _ := utils.GetFreePorts(3)
	log.Println(res, res2)
}
