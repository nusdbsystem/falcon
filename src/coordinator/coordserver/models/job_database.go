package models

import (
	"coordinator/common"
	"coordinator/logger"
	"fmt"
	"github.com/jinzhu/gorm"
	"time"

	_ "gorm.io/driver/mysql"
	_ "gorm.io/driver/sqlite"
)

type JobDB struct {
	engine   string
	host     string
	user     string
	password string
	database string
	addr     string
	Db       *gorm.DB
	Tx       *gorm.DB
}

func InitJobDB() *JobDB {
	jobDB := new(JobDB)
	jobDB.engine = common.JobDbEngine
	jobDB.host = common.JobDbHost
	jobDB.user = common.JobDbMysqlUser
	jobDB.password =  common.JobDbMysqlPwd
	jobDB.database = common.JobDbMysqlDb

	if jobDB.engine == "mysql" {

		mysqlUrl := fmt.Sprintf(
			"%s:%s@tcp(%s:%s)/%s%s",
			jobDB.user,
			jobDB.password,
			jobDB.host,
			common.JobDbMysqlPort,
			jobDB.database,
			common.JobDbMysqlOptions,
		)
		jobDB.addr = mysqlUrl
	} else if jobDB.engine == "sqlite3" {
		jobDB.addr = common.LocalPath + common.JobDbSqliteDb
	}
	return jobDB
}

// connect to db, and begin the transaction
func (jobDB *JobDB) Connect() {

	var db *gorm.DB
	var err error
	NTimes := 20

	for {
		if NTimes < 0 {
			break
		}
		db, err = gorm.Open(jobDB.engine, jobDB.addr)
		if err != nil {
			logger.Do.Println(err)
			logger.Do.Println("JobDB: connecting Db...retry")
			time.Sleep(time.Second * 5)
			NTimes--
		} else {
			jobDB.Db = db
			return
		}
	}
	return
}

// disconnect , should call after jobDB.Commit
func (jobDB *JobDB) Disconnect() {

	e := jobDB.Db.Close()
	if e != nil {
		logger.Do.Println("closeDb error")
	}
}

func (jobDB *JobDB) DefineTables() {

	if jobDB.Db.HasTable(&TrainJobRecord{}) {
		jobDB.Db.AutoMigrate(&TrainJobRecord{})
	} else {
		jobDB.Db.CreateTable(&TrainJobRecord{})
	}

	if jobDB.Db.HasTable(&JobInfoRecord{}) {
		jobDB.Db.AutoMigrate(&JobInfoRecord{})
	} else {
		jobDB.Db.CreateTable(&JobInfoRecord{})
	}

	if jobDB.Db.HasTable(&TaskRecord{}) {
		jobDB.Db.AutoMigrate(&TaskRecord{})
	} else {
		jobDB.Db.CreateTable(&TaskRecord{})
	}

	if jobDB.Db.HasTable(&ExecutionRecord{}) {
		jobDB.Db.AutoMigrate(&ExecutionRecord{})
	} else {
		jobDB.Db.CreateTable(&ExecutionRecord{})
	}

	if jobDB.Db.HasTable(&ModelRecord{}) {
		jobDB.Db.AutoMigrate(&ModelRecord{})
	} else {
		jobDB.Db.CreateTable(&ModelRecord{})
	}

	if jobDB.Db.HasTable(&ServiceRecord{}) {
		jobDB.Db.AutoMigrate(&ServiceRecord{})
	} else {
		jobDB.Db.CreateTable(&ServiceRecord{})
	}

	if jobDB.Db.HasTable(&ModelRecord{}) {
		jobDB.Db.AutoMigrate(&ModelRecord{})
	} else {
		jobDB.Db.CreateTable(&ModelRecord{})
	}

	if jobDB.Db.HasTable(&User{}) {
		jobDB.Db.AutoMigrate(&User{})
	} else {
		jobDB.Db.CreateTable(&User{})
	}

	if jobDB.Db.HasTable(&PartyServer{}) {
		jobDB.Db.AutoMigrate(&PartyServer{})
	} else {
		jobDB.Db.CreateTable(&PartyServer{})
	}

	if jobDB.Db.HasTable(&InferenceJobRecord{}) {
		jobDB.Db.AutoMigrate(&InferenceJobRecord{})
	} else {
		jobDB.Db.CreateTable(&InferenceJobRecord{})
	}

	if jobDB.Db.HasTable(&TestTable{}) {
		jobDB.Db.AutoMigrate(&TestTable{})
	} else {
		jobDB.Db.CreateTable(&TestTable{})
	}

	if jobDB.Db.HasTable(&PortRecord{}) {
		jobDB.Db.AutoMigrate(&PortRecord{})
	} else {
		jobDB.Db.CreateTable(&PortRecord{})
	}

}

func (jobDB *JobDB) Commit(el interface{}) {

	switch el.(type) {
	case []error:
		res, _ := el.([]error)
		for _, ev := range res {
			if ev != nil {
				logger.Do.Println("Sql error", ev)
				jobDB.Tx.Rollback()
				panic(ev)
			}
		}
		jobDB.Tx.Commit()
	case error:
		res, _ := el.(error)
		if res != nil {
			logger.Do.Println("Sql error", res)
			jobDB.Tx.Rollback()
			panic(res)
		}
		jobDB.Tx.Commit()
	}
}

////////////////////////////////////
/////////// Test falcon_sql ////////////
////////////////////////////////////

func (jobDB *JobDB) InsertTest(name string) (error, *TestTable) {
	u := &TestTable{Name: name}

	err := jobDB.Db.Create(u).Error
	return err, u
}
