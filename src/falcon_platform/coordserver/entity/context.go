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

func InitContext(db *models.JobDB) *Context {
	ctx := new(Context)
	ctx.JobDB = db

	ctx.HttpHost = common.CoordIP
	ctx.HttpPort = common.CoordPort

	return ctx
}
