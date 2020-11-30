package controller

import (
	"coordinator/api/entity"
	"coordinator/api/models"
)

func CreateTables() {

	ms := models.InitMetaStore()

	ms.Connect()
	ms.Tx = ms.Db.Begin()

	ms.DefineTables()

	ms.Commit(nil)
	ms.DisConnect()

}

func ListenerAdd(ctx *entity.Context, listenerAddr string) {

	_, _ = ctx.Ms.ListenerAdd(listenerAddr)

}

func ListenerDelete(ctx *entity.Context, listenerAddr string) {

	_ = ctx.Ms.ListenerDelete(listenerAddr)

}
