package controller

import "coordinator/coordserver/entity"



func ModelUpdate(jobId, isTrained uint, ctx *entity.Context) uint {
	tx := ctx.JobDB.Db.Begin()
	e, u := ctx.JobDB.ModelUpdate(tx, jobId, isTrained)
	ctx.JobDB.Commit(tx, e)
	return u.ID

}
