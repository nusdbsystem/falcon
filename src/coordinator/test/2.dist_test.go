package test

import (
	"coordinator/distributed/utils"
	"coordinator/logger"
	"testing"
)

func TestDist(t *testing.T) {

	res, _ := utils.GetFreePort()
	res2, _ := utils.GetFreePorts(3)
	logger.Do.Println(res, res2)
}
