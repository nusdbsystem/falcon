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

	common.JobDatabase = "mysql"
	common.JobDbHost = "127.0.0.1"
	common.JobDbMysqlUser = "root"
	common.JobDbMysqlPwd = "rootuser"
	common.JobDbMysqlDb = "Test"
	common.JobDbMysqlPort = "3306"
	common.JobDbMysqlOptions = "?parseTime=true"

	jobDB := models.InitJobDB()
	jobDB.Connect()
	tx := jobDB.Db.Begin()
	jobDB.DefineTables()
	jobDB.Commit(tx,nil)

	//rrr,e := jobDB.InferenceGetCurrentRunningOneWithJobName("test", 1)
	//logger.Do.Println(rrr, e)

	//eee, _ := jobDB.CreateInference(0, 123123123)
	//jobDB.Commit(eee)



	a := []uint{1,2,3,4,5,6,7,8,9,10}
	tx = jobDB.Db.Begin()
	var elist []error
	for _, v :=  range a{

		err, jobinfo := jobDB.JobInfoCreate("test", v,"","", "",0,"",0,0)
		err2, job := jobDB.JobSubmit(v, 0, jobinfo.Id)
		err3, _ := jobDB.CreateInference(0, job.JobId)
		err4, _ := jobDB.InferenceUpdateStatus(job.JobId, 1)

		elist = append(elist, err)
		elist = append(elist, err2)
		elist = append(elist, err3)
		elist = append(elist, err4)
	}
	jobDB.Commit(tx,elist)

	jobDB.Disconnect()

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
			logger.Do.Printf("SetupPartyServer: connecting to coord %s ...retry \n", common.CoordAddr)
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
