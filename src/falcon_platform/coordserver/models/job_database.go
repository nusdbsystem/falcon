package models

import (
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"gorm.io/driver/mysql"
	"gorm.io/driver/sqlite"
	"gorm.io/gorm"
	"path"
	"time"
)

type JobDB struct {
	engine   string
	host     string
	user     string
	password string
	database string
	addr     string
	dia      gorm.Dialector
	DB       *gorm.DB
}

// NOTE: Jinzhu decided to eliminate the Close() method on version 1.20
// because GORM supports connection pooling
// Jinzhu reply: http://disq.us/p/2ayeg92:
// "GORM support database pool, your application should share one database instance.
// `db.Close` is rarely necessary so I decided to remove the `Close` method in V2 to reduce the misuse,
// check out the `Generic database interface` if it is necessary for you."

// NOTE: By default, all GORM write operations run inside a transaction to ensure data consistency
// The returned DB is safe for concurrent use by multiple goroutines and maintains its own pool of idle connections.
// Thus, the Open function should be called just once. It is rarely necessary to close a DB.
// ref1: https://stackoverflow.com/a/61823123
// ref2: https://stackoverflow.com/a/56192146

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
		jobDB.dia = mysql.Open(jobDB.addr)
	} else if jobDB.engine == common.DBSqlite3 {
		// for quick dev test, save the sqlite3 db in timestamped falcon_logs
		// if use outside of falcon_logs, make sure to reset the db file
		jobDB.addr = path.Join(common.CoordBasePath, common.JobDbSqliteDb)
		jobDB.dia = sqlite.Open(jobDB.addr)
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
		db, err = gorm.Open(jobDB.dia, &gorm.Config{})
		if err != nil {
			logger.Log.Println(err)
			logger.Log.Println("JobDB: connecting Db...retry")
			time.Sleep(time.Second * 5)
			NTimes--
		} else {
			jobDB.DB = db
			return
		}
	}
	return
}

func (jobDB *JobDB) DefineTables() {

	// NOTE: AutoMigrate will
	// create tables, missing foreign keys, constraints, columns and indexes
	// It WON’T delete unused columns to protect your data.
	// reference: https://gorm.io/docs/migration.html

	e := jobDB.DB.AutoMigrate(
		// System Usage
		&PortRecord{},

		// Train Jobs
		&TrainJobRecord{},
		&JobInfoRecord{},
		&TaskRecord{},

		// Inference and Model Serving
		&ModelRecord{},
		&InferenceJobRecord{},

		// Party Server
		&PartyServer{},

		// Executor
		&ExecutionRecord{},
		&ServiceRecord{},

		// User
		&User{},
	)
	if e != nil {
		logger.Log.Println("Creating tables error")
		panic(e)
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
				logger.Log.Println("Sql error", ev)
				tx.Rollback()
				panic(ev)
			}
		}
		tx.Commit()
	case error:
		res, _ := el.(error)
		if res != nil {
			logger.Log.Println("Sql error", res)
			tx.Rollback()
			panic(res)
		}
		tx.Commit()
	default:
		panic("Commit Not supported type")
	}
}
