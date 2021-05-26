package controller

import (
	"falcon_platform/common"
	"falcon_platform/coordserver/entity"
	"falcon_platform/coordserver/models"
	"strconv"
)

func CreateTables() {
	jobDB := models.InitJobDB()
	jobDB.Connect()
	jobDB.DefineTables()
}

// func CreateSpecificSysPorts(port string) {
// 	jobDB := models.InitJobDB()

// 	jobDB.Connect()
// 	tx := jobDB.DB.Begin()
// 	var e1 error
// 	portInt, _ := strconv.Atoi(port)
// 	if !jobDB.CheckPort(uint(portInt)) {
// 		e1, _ = jobDB.AddPort(tx, uint(portInt))
// 	}
// 	jobDB.Commit(tx, e1)
// }

func CreateSysPorts() {
	jobDB := models.InitJobDB()

	jobDB.Connect()
	tx := jobDB.DB.Begin()

	var e1 error
	var e2 error
	var e3 error

	mysqlPort, _ := strconv.Atoi(common.JobDbMysqlNodePort)
	if !jobDB.CheckPort(uint(mysqlPort)) {
		e1, _ = jobDB.AddPort(tx, uint(mysqlPort))
	}

	redisPort, _ := strconv.Atoi(common.RedisNodePort)
	if !jobDB.CheckPort(uint(redisPort)) {
		e2, _ = jobDB.AddPort(tx, uint(redisPort))

	}

	coordPort, _ := strconv.Atoi(common.CoordPort)
	if !jobDB.CheckPort(uint(coordPort)) {
		e3, _ = jobDB.AddPort(tx, uint(coordPort))

	}

	jobDB.Commit(tx, []error{e1, e2, e3})
}

func PartyServerAdd(ctx *entity.Context, partyserverAddr, Port string) {
	tx := ctx.JobDB.DB.Begin()
	e, _ := ctx.JobDB.PartyServerAdd(tx, partyserverAddr, Port)
	ctx.JobDB.Commit(tx, e)
}

func PartyServerDelete(ctx *entity.Context, partyserverAddr string) {
	tx := ctx.JobDB.DB.Begin()
	e := ctx.JobDB.PartyServerDelete(tx, partyserverAddr)
	ctx.JobDB.Commit(tx, e)
}
