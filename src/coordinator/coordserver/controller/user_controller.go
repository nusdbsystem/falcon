package controller

import (
	"coordinator/api/models"
)

func CreateUser() {

	ms := models.InitMetaStore()

	ms.Connect()
	ms.Tx = ms.Db.Begin()

	e := ms.CreateAdminUser()

	ms.Commit(e)
	ms.DisConnect()

}
