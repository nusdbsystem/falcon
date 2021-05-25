package controller

import (
	"falcon_platform/coordserver/entity"
	"falcon_platform/coordserver/models"
	"falcon_platform/logger"
)

func AssignPort(ctx *entity.Context) uint {

	ports := ctx.JobDB.GetPorts()

	tx := ctx.JobDB.Db.Begin()

	maxPort := findMax(ports)

	var err error
	var u *models.PortRecord
	i := 1
	for {
		err, u = ctx.JobDB.AddPort(tx, maxPort+uint(i))
		if err != nil {
			logger.Log.Println("AssignPort, error ", err)
			logger.Log.Println("AssignPort: retry...")
			i++
		} else {
			logger.Log.Println("AssignPort: AssignSuccessful port is ", u.Port)
			break
		}
	}

	ctx.JobDB.Commit(tx, err)

	return u.Port
}

func AddPort(newPort uint, ctx *entity.Context) uint {

	tx := ctx.JobDB.Db.Begin()

	e, _ := ctx.JobDB.AddPort(tx, newPort)

	ctx.JobDB.Commit(tx, e)

	return newPort

}

func findMax(l []uint) uint {
	var res uint

	for j := 0; j < len(l)-1; j++ {

		if l[j] > l[j+1] {
			res = l[j]
		} else {
			res = l[j+1]
		}
	}
	return res
}

func GetPartyServerPort(PartyServerAddr string, ctx *entity.Context) string {

	e, u := ctx.JobDB.PartyServerGet(PartyServerAddr)
	if e != nil {
		panic(e)
	}
	return u.Port

}
