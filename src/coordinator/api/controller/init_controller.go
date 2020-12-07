package controller

import (
	"coordinator/api/entity"
	"coordinator/api/models"
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

func CreateSysPorts() {

	ms := models.InitMetaStore()

	ms.Connect()
	ms.Tx = ms.Db.Begin()

	var e1 error
	var e2 error
	var e3 error
	var e4 error

	mysqlPort, _:= strconv.Atoi(common.MsMysqlNodePort)
	if !ms.CheckPort(uint(mysqlPort)){
		e1, _ = ms.AddPort(uint(mysqlPort))
	}

	redisPort, _:= strconv.Atoi(common.RedisNodePort)
	if !ms.CheckPort(uint(redisPort)){
		e2, _ = ms.AddPort(uint(redisPort))

	}

	masterPort, _:= strconv.Atoi(common.CoordPort)
	if !ms.CheckPort(uint(masterPort)){
		e3, _ = ms.AddPort(uint(masterPort))

	}

	listenerPort, _:= strconv.Atoi(common.ListenerPort)
	if !ms.CheckPort(uint(listenerPort)){
		e4, _ = ms.AddPort(uint(listenerPort))
	}

	ms.Commit([]error{e1,e2,e3,e4})
	ms.DisConnect()

}

func ListenerAdd(ctx *entity.Context, listenerAddr string) {

	_, _ = ctx.Ms.ListenerAdd(listenerAddr)

}

func ListenerDelete(ctx *entity.Context, listenerAddr string) {

	_ = ctx.Ms.ListenerDelete(listenerAddr)

}
