package test

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/distributed"
	"coordinator/logger"
	"testing"
)

func TestClient(t *testing.T) {
	logger.Do, logger.F = logger.GetLogger("./TestSubProc")

	masterAddress:="127.0.0.1:41414"
	qItem:=&cache.QItem{}
	qItem.IPs=[]string{"127.0.0.1"}
	taskType:="train"

	common.CoordSvcURLGlobal = "127.0.0.1:30004"
	distributed.SetupMaster(masterAddress, qItem, taskType)

}
