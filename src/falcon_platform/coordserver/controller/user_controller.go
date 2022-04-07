package controller

import (
	"falcon_platform/coordserver/models"
)

func CreateUser(jobDB *models.JobDB) {
	tx := jobDB.DB.Begin()

	e := jobDB.CreateAdminUser(tx)

	jobDB.Commit(tx, e)
}
