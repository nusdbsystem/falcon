package entity

import (
	"coordinator/coordserver/models"
	"coordinator/common"
)

type Context struct {
	JobDB       *models.JobDB
	HttpHost string
	HttpPort string
	UsrId    uint
}

func InitContext() *Context {
	ctx := new(Context)
	ctx.JobDB = models.InitJobDB()

	ctx.HttpHost = common.CoordAddrGlobal
	ctx.HttpPort = common.CoordPort

	return ctx
}
