package test

import (
	"falcon_platform/logger"
	"testing"
)

func TestClient(t *testing.T) {
	logger.Log, logger.LogFile = logger.GetLogger("./TestSubProc")

	//masterAddr := "127.0.0.1:41414"
	//dslOjb := &cache.DslObj{}
	//dslOjb.PartyAddrList = []string{"127.0.0.1"}
	//workerType := common.TrainWorker
	//
	//common.CoordAddr = "127.0.0.1:30004"
	//jobmanager.ManageJobLifeCycle(masterAddr, dslOjb, workerType)

}
