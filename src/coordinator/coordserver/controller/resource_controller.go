package controller

import (
	"coordinator/api/entity"
	"coordinator/api/models"
	"coordinator/logger"
)


func AssignPort(ctx *entity.Context) uint {

	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e, ports := ctx.Ms.GetPorts()

	maxPort := findMax(ports)

	var err error
	var u *models.PortRecord
	i:=1
	for {
		err, u = ctx.Ms.AddPort(maxPort + uint(i))
		if err != nil{
			logger.Do.Println("AssignPort, error ", err)
			logger.Do.Println("AssignPort: retry...")
			i++
		}else{
			logger.Do.Println("AssignPort: AssignSuccessful port is ", u.Port)
			break
		}
	}

	ctx.Ms.Commit([]error{e, err})

	return u.Port
}

func AddPort(newPort uint, ctx *entity.Context) uint {

	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e2, _ := ctx.Ms.AddPort(newPort)

	ctx.Ms.Commit([]error{e2})

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


func GetListenerPort(ListenerAddr string, ctx *entity.Context) string {

	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e, u := ctx.Ms.ListenerGet(ListenerAddr)

	ctx.Ms.Commit([]error{e})

	return u.Port

}