package test

import (
	"coordinator/api/models"
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

	ms := models.InitMetaStore()
	ms.Connect()
	ms.Tx = ms.Db.Begin()
	ms.DefineTables()

	var err error
	var u *models.PortRecord

	NTimes := 20
	for {
		if NTimes<0{
			panic("\"SetupListener: connecting to coord Db...retry\"")
		}
		err, u = ms.AddPort(uint(30001))
		if err != nil{
			logger.Do.Println(err)
			logger.Do.Printf("SetupListener: connecting to coord %s ...retry \n", common.CoordSvcURLGlobal)
			time.Sleep(time.Second*5)
			NTimes--
		}else{
			logger.Do.Println("AssignPort: AssignSuccessful port is ", u.Port)
			break
		}
	}


	res := ms.CheckPort(uint(1123))
	fmt.Println(ms.GetPorts())

	fmt.Println(res)

}
