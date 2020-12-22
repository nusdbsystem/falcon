package controller

import (
	"coordinator/coordserver/entity"
	"coordinator/coordserver/models"
	"coordinator/common"
	"strconv"
)

func CreateTables() {

	jobDB := models.InitJobDB()

	jobDB.Connect()
	tx := jobDB.Db.Begin()

	jobDB.DefineTables()

	jobDB.Commit(tx,nil)
	jobDB.Disconnect()

}

func CreateSpecificSysPorts(port string) {

	jobDB := models.InitJobDB()

	jobDB.Connect()
	tx := jobDB.Db.Begin()
	var e1 error
	portInt, _:= strconv.Atoi(port)
	if !jobDB.CheckPort(uint(portInt)){
		e1, _ = jobDB.AddPort(tx,uint(portInt))
	}
	jobDB.Commit(tx, e1)
	jobDB.Disconnect()

}

func CreateSysPorts() {

	jobDB := models.InitJobDB()

	jobDB.Connect()
	tx := jobDB.Db.Begin()

	var e1 error
	var e2 error
	var e3 error

	mysqlPort, _:= strconv.Atoi(common.JobDbMysqlNodePort)
	if !jobDB.CheckPort(uint(mysqlPort)){
		e1, _ = jobDB.AddPort(tx,uint(mysqlPort))
	}

	redisPort, _:= strconv.Atoi(common.RedisNodePort)
	if !jobDB.CheckPort(uint(redisPort)){
		e2, _ = jobDB.AddPort(tx,uint(redisPort))

	}

	coordPort, _:= strconv.Atoi(common.CoordPort)
	if !jobDB.CheckPort(uint(coordPort)){
		e3, _ = jobDB.AddPort(tx,uint(coordPort))

	}

	jobDB.Commit(tx, []error{e1,e2,e3})
	jobDB.Disconnect()

}

func PartyServerAdd(ctx *entity.Context, partyserverAddr,Port string) {
	tx := ctx.JobDB.Db.Begin()
	e, _ := ctx.JobDB.PartyServerAdd(tx,partyserverAddr, Port)
	ctx.JobDB.Commit(tx, e)


}

func PartyServerDelete(ctx *entity.Context, partyserverAddr string) {
	tx := ctx.JobDB.Db.Begin()
	e := ctx.JobDB.PartyServerDelete(tx,partyserverAddr)
	ctx.JobDB.Commit(tx, e)

}
