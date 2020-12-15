package test

import (
	"coordinator/coordserver/models"
	"coordinator/common"
	"coordinator/logger"
	"fmt"
	"testing"
	"time"
)


func TestDb(t *testing.T){

	logger.Do, logger.F = logger.GetLogger("./TestSubProc")

	common.MsEngine = "mysql"
	common.MsHost = "127.0.0.1"
	common.MsMysqlUser = "falcon"
	common.MsMysqlPwd = "falcon"
	common.MsMysqlDb = "falcon"
	common.MsEngine = "mysql"
	common.MsMysqlPort = "30001"
	common.MsMysqlOptions = "?parseTime=true"

	jobDB := models.InitJobDB()
	jobDB.Connect()
	jobDB.Tx = jobDB.Db.Begin()
	jobDB.DefineTables()

	var err error
	var u *models.PortRecord

	NTimes := 20
	for {
		if NTimes<0{
			panic("\"SetupPartyServer: connecting to coord Db...retry\"")
		}
		err, u = jobDB.AddPort(uint(30001))
		if err != nil{
			logger.Do.Println(err)
			logger.Do.Printf("SetupPartyServer: connecting to coord %s ...retry \n", common.CoordSvcURLGlobal)
			time.Sleep(time.Second*5)
			NTimes--
		}else{
			logger.Do.Println("AssignPort: AssignSuccessful port is ", u.Port)
			break
		}
	}


	res := jobDB.CheckPort(uint(1123))
	fmt.Println(jobDB.GetPorts())

	fmt.Println(res)

}
