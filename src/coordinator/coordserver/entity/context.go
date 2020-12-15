package entity

import (
	"coordinator/coordserver/models"
	"coordinator/common"
)

type Context struct {
	Ms       *models.MetaStore
	HttpHost string
	HttpPort string
	UsrId    uint
}

func InitContext() *Context {
	ad := new(Context)
	ad.Ms = models.InitMetaStore()

	ad.HttpHost = common.CoordAddrGlobal
	ad.HttpPort = common.CoordPort

	return ad
}
