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
	jobDB.Tx = jobDB.Db.Begin()

	jobDB.DefineTables()

	jobDB.Commit(nil)
	jobDB.Disconnect()

}

func CreateSpecificSysPorts(port string) {

	jobDB := models.InitJobDB()

	jobDB.Connect()
	jobDB.Tx = jobDB.Db.Begin()
	var e1 error
	portInt, _:= strconv.Atoi(port)
	if !jobDB.CheckPort(uint(portInt)){
		e1, _ = jobDB.AddPort(uint(portInt))
	}
	jobDB.Commit(e1)
	jobDB.Disconnect()

}

func CreateSysPorts() {

	jobDB := models.InitJobDB()

	jobDB.Connect()
	jobDB.Tx = jobDB.Db.Begin()

	var e1 error
	var e2 error
	var e3 error

	mysqlPort, _:= strconv.Atoi(common.JobDbMysqlNodePort)
	if !jobDB.CheckPort(uint(mysqlPort)){
		e1, _ = jobDB.AddPort(uint(mysqlPort))
	}

	redisPort, _:= strconv.Atoi(common.RedisNodePort)
	if !jobDB.CheckPort(uint(redisPort)){
		e2, _ = jobDB.AddPort(uint(redisPort))

	}

	coordPort, _:= strconv.Atoi(common.CoordPort)
	if !jobDB.CheckPort(uint(coordPort)){
		e3, _ = jobDB.AddPort(uint(coordPort))

	}

	jobDB.Commit([]error{e1,e2,e3})
	jobDB.Disconnect()

}

func PartyServerAdd(ctx *entity.Context, partyserverAddr,Port string) {

	_, _ = ctx.JobDB.PartyServerAdd(partyserverAddr, Port)

}

func PartyServerDelete(ctx *entity.Context, partyserverAddr string) {

	_ = ctx.JobDB.PartyServerDelete(partyserverAddr)

}
