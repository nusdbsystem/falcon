package test

import (
	"coordinator/api/models"
	"coordinator/logger"
	"fmt"
	"testing"
)


func TestDb(t *testing.T){

	logger.Do, logger.F = logger.GetLogger("./TestSubProc")

	ms := models.InitMetaStore()
	ms.Connect()
	ms.Tx = ms.Db.Begin()
	ms.DefineTables()

	_,_=ms.AddPort(uint(30001))
	res := ms.CheckPort(uint(1123))
	fmt.Println(ms.GetPorts())

	fmt.Println(res)

}
