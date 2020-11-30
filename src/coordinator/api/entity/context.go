package entity

import "coordinator/api/models"

type Context struct {
	Ms       *models.MetaStore
	HttpHost string
	HttpPort string
	UsrId    uint
}

func InitContext(httpAddr ...string) *Context {
	ad := new(Context)
	ad.Ms = models.InitMetaStore()
	ad.HttpHost = httpAddr[0]
	ad.HttpPort = httpAddr[1]
	return ad
}
