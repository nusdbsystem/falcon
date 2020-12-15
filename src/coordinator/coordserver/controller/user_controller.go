package controller

import (
	"coordinator/coordserver/models"
)

func CreateUser() {

	jobDB := models.InitJobDB()

	jobDB.Connect()
	jobDB.Tx = jobDB.Db.Begin()

	e := jobDB.CreateAdminUser()

	jobDB.Commit(e)
	jobDB.DisConnect()

}
