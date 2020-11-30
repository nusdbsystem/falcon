package controller

import "coordinator/api/entity"



func ModelUpdate(jobId, isTrained uint, ctx *entity.Context) uint {
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, u := ctx.Ms.ModelUpdate(jobId, isTrained)
	ctx.Ms.Commit(e)
	return u.ID

}
