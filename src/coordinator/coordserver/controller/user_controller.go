package controller

import (
	"coordinator/coordserver/models"
)

func CreateUser() {

	jobDB := models.InitJobDB()

	jobDB.Connect()
	tx := jobDB.Db.Begin()

	e := jobDB.CreateAdminUser(tx)

	jobDB.Commit(tx, e)
	jobDB.Disconnect()

}
