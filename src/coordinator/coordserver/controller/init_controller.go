package controller

import (
	"coordinator/coordserver/entity"
	"coordinator/coordserver/models"
	"coordinator/common"
	"strconv"
)

func CreateTables() {

	ms := models.InitMetaStore()

	ms.Connect()
	ms.Tx = ms.Db.Begin()

	ms.DefineTables()

	ms.Commit(nil)
	ms.DisConnect()

}

func CreateSpecificSysPorts(port string) {

	ms := models.InitMetaStore()

	ms.Connect()
	ms.Tx = ms.Db.Begin()
	var e1 error
	portInt, _:= strconv.Atoi(port)
	if !ms.CheckPort(uint(portInt)){
		e1, _ = ms.AddPort(uint(portInt))
	}
	ms.Commit(e1)
	ms.DisConnect()

}

func CreateSysPorts() {

	ms := models.InitMetaStore()

	ms.Connect()
	ms.Tx = ms.Db.Begin()

	var e1 error
	var e2 error
	var e3 error

	mysqlPort, _:= strconv.Atoi(common.MsMysqlNodePort)
	if !ms.CheckPort(uint(mysqlPort)){
		e1, _ = ms.AddPort(uint(mysqlPort))
	}

	redisPort, _:= strconv.Atoi(common.RedisNodePort)
	if !ms.CheckPort(uint(redisPort)){
		e2, _ = ms.AddPort(uint(redisPort))

	}

	coordPort, _:= strconv.Atoi(common.CoordPort)
	if !ms.CheckPort(uint(coordPort)){
		e3, _ = ms.AddPort(uint(coordPort))

	}

	ms.Commit([]error{e1,e2,e3})
	ms.DisConnect()

}

func PartyServerAdd(ctx *entity.Context, partyserverAddr,Port string) {

	_, _ = ctx.Ms.PartyServerAdd(partyserverAddr, Port)

}

func PartyServerDelete(ctx *entity.Context, partyserverAddr string) {

	_ = ctx.Ms.PartyServerDelete(partyserverAddr)

}
