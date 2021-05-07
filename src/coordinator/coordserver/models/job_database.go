package models

import (
	"coordinator/common"
	"coordinator/logger"
	"fmt"
	"path"
	"time"

	"github.com/jinzhu/gorm"

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
}

func InitJobDB() *JobDB {
	jobDB := new(JobDB)
	jobDB.engine = common.JobDatabase

	if jobDB.engine == common.DBMySQL {
		jobDB.host = common.JobDbHost
		jobDB.user = common.JobDbMysqlUser
		jobDB.password = common.JobDbMysqlPwd
		jobDB.database = common.JobDbMysqlDb

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
	} else if jobDB.engine == common.DBsqlite3 {
		// for quick dev test, save the sqlite3 db in timestamped dev_test
		// if use outside of dev_test, make sure to reset the db file
		jobDB.addr = path.Join(common.CoordBasePath, common.JobDbSqliteDb)
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

func (jobDB *JobDB) Commit(tx *gorm.DB, el interface{}) {

	if el == nil {
		tx.Commit()
		return
	}

	switch el.(type) {
	case []error:
		res, _ := el.([]error)
		for _, ev := range res {
			if ev != nil {
				logger.Do.Println("Sql error", ev)
				tx.Rollback()
				panic(ev)
			}
		}
		tx.Commit()
	case error:
		res, _ := el.(error)
		if res != nil {
			logger.Do.Println("Sql error", res)
			tx.Rollback()
			panic(res)
		}
		tx.Commit()
	default:
		panic("Commit Not supported type")
	}
}

////////////////////////////////////
/////////// Test falcon_sql ////////////
////////////////////////////////////
