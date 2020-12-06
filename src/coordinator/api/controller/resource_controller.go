package controller

import (
	"coordinator/api/entity"
)


func AssignPort(ctx *entity.Context) uint {

	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e, ports := ctx.Ms.GetPorts()

	maxPort := findMax(ports)

	newPort := maxPort + 1

	e2, _ := ctx.Ms.AddPort(newPort)

	ctx.Ms.Commit([]error{e, e2})

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