package test

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/distributed"
	"coordinator/logger"
	"testing"
)

func TestClient(t *testing.T) {
	logger.Log, logger.LogFile = logger.GetLogger("./TestSubProc")

	masterAddr := "127.0.0.1:41414"
	qItem := &cache.QItem{}
	qItem.AddrList = []string{"127.0.0.1"}
	workerType := common.TrainWorker

	common.CoordAddr = "127.0.0.1:30004"
	distributed.SetupMaster(masterAddr, qItem, workerType)

}
