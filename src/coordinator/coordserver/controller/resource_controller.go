package controller

import (
	"coordinator/coordserver/entity"
	"coordinator/coordserver/models"
	"coordinator/logger"
)


func AssignPort(ctx *entity.Context) uint {

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()

	e, ports := ctx.JobDB.GetPorts()

	maxPort := findMax(ports)

	var err error
	var u *models.PortRecord
	i:=1
	for {
		err, u = ctx.JobDB.AddPort(maxPort + uint(i))
		if err != nil{
			logger.Do.Println("AssignPort, error ", err)
			logger.Do.Println("AssignPort: retry...")
			i++
		}else{
			logger.Do.Println("AssignPort: AssignSuccessful port is ", u.Port)
			break
		}
	}

	ctx.JobDB.Commit([]error{e, err})

	return u.Port
}

func AddPort(newPort uint, ctx *entity.Context) uint {

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()

	e2, _ := ctx.JobDB.AddPort(newPort)

	ctx.JobDB.Commit([]error{e2})

	return newPort

}


func findMax(l []uint) uint {
	var res uint

	for j:=0; j<len(l)-1; j++{

		if l[j] > l[j+1]{
			res = l[j]
		}else{
			res = l[j+1]
		}
	}
	return res
}


func GetPartyServerPort(PartyServerAddr string, ctx *entity.Context) string {

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()

	e, u := ctx.JobDB.PartyServerGet(PartyServerAddr)

	ctx.JobDB.Commit([]error{e})

	return u.Port

}