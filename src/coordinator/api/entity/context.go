package entity

import "coordinator/api/models"

type Context struct {
	Ms *models.MetaStore

	UsrId uint
}

func InitContext() *Context {
	ad := new(Context)
	ad.Ms = models.InitMetaStore()
	return ad
}
