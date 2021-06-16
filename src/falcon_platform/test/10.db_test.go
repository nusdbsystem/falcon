package test

import (
	"falcon_platform/common"
	"falcon_platform/logger"
	"testing"
)

func TestDb(t *testing.T) {

	logger.Log, logger.LogFile = logger.GetLogger("./TestSubProc")

	common.JobDatabase = "mysql"
	common.JobDbHost = "127.0.0.1"
	common.JobDbMysqlUser = "root"
	common.JobDbMysqlPwd = "rootuser"
	common.JobDbMysqlDb = "Test"
	common.JobDbMysqlPort = "3306"
	common.JobDbMysqlOptions = "?parseTime=true"

}
