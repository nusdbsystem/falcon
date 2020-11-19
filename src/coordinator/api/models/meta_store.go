package models

import (
	"github.com/jinzhu/gorm"
	"log"

	//_ "gorm.io/driver/sqlite"
	_ "gorm.io/driver/mysql"
)

type MetaStore struct {
	engine   string
	host     string
	user     string
	password string
	database string
	url      string
	Db       *gorm.DB
	Tx       *gorm.DB
}

func InitDefaultMetaStore() *MetaStore {
	ms := InitMetaStore(
		"mysql",
		"localhost",
		"root",
		"rootuser",
		"Test",
		"?parseTime=true")
	return ms
}

func InitMetaStore(engine, host, user, password, database, options string) *MetaStore {
	ms := new(MetaStore)
	ms.engine = engine
	ms.host = host
	ms.user = user
	ms.password = password
	ms.database = database

	if ms.engine == "mysql" {
		ms.url = ms.user + ":" + ms.password + "@tcp(" + ms.host + ":3306)/" + ms.database + options
	} else if ms.engine == "sqlite3" {
		ms.url = "./falcon.db"
	}
	return ms
}

// connect to db, and begin the transaction
func (ms *MetaStore) Connect() {
	db, err := gorm.Open(ms.engine, ms.url)
	if err != nil {
		log.Println(err)
		return
	}
	ms.Db = db
}

// disconnect , should call after ms.Commit
func (ms *MetaStore) DisConnect() {

	e := ms.Db.Close()
	if e != nil {
		log.Println("closeDb error")
	}
}

func (ms *MetaStore) DefineTables() {

	if ms.Db.HasTable(&JobRecord{}) {
		ms.Db.AutoMigrate(&JobRecord{})
	} else {
		ms.Db.CreateTable(&JobRecord{})
	}

	if ms.Db.HasTable(&TaskRecord{}) {
		ms.Db.AutoMigrate(&TaskRecord{})
	} else {
		ms.Db.CreateTable(&TaskRecord{})
	}

	if ms.Db.HasTable(&ExecutionRecord{}) {
		ms.Db.AutoMigrate(&ExecutionRecord{})
	} else {
		ms.Db.CreateTable(&ExecutionRecord{})
	}

	if ms.Db.HasTable(&ModelRecord{}) {
		ms.Db.AutoMigrate(&ModelRecord{})
	} else {
		ms.Db.CreateTable(&ModelRecord{})
	}

	if ms.Db.HasTable(&ServiceRecord{}) {
		ms.Db.AutoMigrate(&ServiceRecord{})
	} else {
		ms.Db.CreateTable(&ServiceRecord{})
	}

	if ms.Db.HasTable(&ModelRecord{}) {
		ms.Db.AutoMigrate(&ModelRecord{})
	} else {
		ms.Db.CreateTable(&ModelRecord{})
	}

	if ms.Db.HasTable(&User{}) {
		ms.Db.AutoMigrate(&User{})
	} else {
		ms.Db.CreateTable(&User{})
	}

	if ms.Db.HasTable(&Listeners{}) {
		ms.Db.AutoMigrate(&Listeners{})
	} else {
		ms.Db.CreateTable(&Listeners{})
	}

	if ms.Db.HasTable(&TestTable{}) {
		ms.Db.AutoMigrate(&TestTable{})
	} else {
		ms.Db.CreateTable(&TestTable{})
	}
}

func (ms *MetaStore) Commit(el interface{}) {

	switch el.(type) {
	case []error:
		res, _ := el.([]error)
		for _, ev := range res {
			if ev != nil {
				log.Println("Sql error", ev)
				ms.Tx.Rollback()
				panic(ev)
			}
		}
		ms.Tx.Commit()
	case error:
		res, _ := el.(error)
		if res != nil {
			log.Println("Sql error", res)
			ms.Tx.Rollback()
			panic(res)
		}
		ms.Tx.Commit()
	}
}

////////////////////////////////////
/////////// Test falcon_sql ////////////
////////////////////////////////////

func (ms *MetaStore) InsertTest(name string) (error, *TestTable) {
	u := &TestTable{Name: name}

	err := ms.Db.Create(u).Error
	return err, u
}
