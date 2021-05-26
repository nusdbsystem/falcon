package models

import (
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"path"

	"gorm.io/driver/sqlite"
	"gorm.io/gorm"
)

type JobDB struct {
	engine   string
	host     string
	user     string
	password string
	database string
	addr     string
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

	if jobDB.engine == common.DBsqlite3 {
		// github.com/mattn/go-sqlite3
		db, err = gorm.Open(sqlite.Open(jobDB.addr), &gorm.Config{})
		// NOTE: You can also use file::memory:?cache=shared instead of a path to a file.
		// This will tell SQLite to use a temporary database in system memory.
	}

	if err != nil {
		panic("failed to connect database")
	}

	jobDB.DB = db
}

func (jobDB *JobDB) DefineTables() {

	// NOTE: AutoMigrate will
	// create tables, missing foreign keys, constraints, columns and indexes
	// It WON’T delete unused columns to protect your data.
	// reference: https://gorm.io/docs/migration.html

	jobDB.DB.AutoMigrate(
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
