package entity

import (
	"falcon_platform/common"
	"falcon_platform/coordserver/models"
)

type Context struct {
	JobDB    *models.JobDB
	HttpHost string
	HttpPort string
	UsrId    uint
}

func InitContext() *Context {
	ctx := new(Context)
	ctx.JobDB = models.InitJobDB()

	ctx.HttpHost = common.CoordIP
	ctx.HttpPort = common.CoordPort

	return ctx
}
