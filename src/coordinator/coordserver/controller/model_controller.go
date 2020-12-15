package controller

import "coordinator/coordserver/entity"



func ModelUpdate(jobId, isTrained uint, ctx *entity.Context) uint {
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, u := ctx.JobDB.ModelUpdate(jobId, isTrained)
	ctx.JobDB.Commit(e)
	return u.ID

}
