package controller

import (
	"coordinator/api/entity"
	"coordinator/config"
	"coordinator/distributed"
	"encoding/json"
	"fmt"
)

func JobSubmit(dsl *config.DSL, ctx *entity.Context) (uint, string, uint, string, uint, uint) {

	fmt.Println("HTTP server: in SubmitJob, put to the JobQueue")

	// write to db
	partyIds, err := json.Marshal(dsl.PartyInfos)
	if err != nil {
		panic("json.Marshal(dsl.PartyIds) error")
	}
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	errs, u := ctx.Ms.JobSubmit(dsl.JobName, ctx.UsrId, string(partyIds), dsl.JobDecs, dsl.TaskNum, config.JobInit)
	err2, _ := ctx.Ms.SvcCreate(u.JobId)

	ctx.Ms.Commit([]error{errs, err2})

	// generate item pushed to the queue
	var qItem config.QItem
	var iPs []string
	var taskInfos config.Tasks
	var partyPath []config.PartyPath

	for _, v := range dsl.PartyInfos {

		// list of ip
		iPs = append(iPs, v.IP)

		// list of ip
		partyPath = append(partyPath, v.PartyPaths)
	}

	taskInfos = dsl.Tasks

	qItem.IPs = iPs
	qItem.JobId = u.JobId
	qItem.PartyPath = partyPath
	qItem.TaskInfos = taskInfos

	go func() {
		entity.JobQueue.Push(&qItem)
	}()

	return u.JobId, u.JobName, u.UserID, u.PartyIds, u.TaskNum, u.Status
}

func JobKill(jobId uint, ctx *entity.Context) {

	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, u := ctx.Ms.JobGetByJobID(jobId)
	ctx.Ms.Commit(e)

	distributed.KillJob(u.MasterAddress, config.Proxy)

	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e2, _ := ctx.Ms.JobUpdateStatus(jobId, config.JobKilled)
	ctx.Ms.Commit(e2)
}

func JobUpdateMaster(jobId uint, masterAddr string, ctx *entity.Context) {
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, _ := ctx.Ms.SvcUpdateMaster(jobId, masterAddr)
	e2, _ := ctx.Ms.JobUpdateMaster(jobId, masterAddr)
	ctx.Ms.Commit([]error{e, e2})
}

func JobUpdateResInfo(jobId uint, jobErrMsg, jobResult, jobExtInfo string, ctx *entity.Context) {
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, _ := ctx.Ms.JobUpdateResInfo(jobId, jobErrMsg, jobResult, jobExtInfo)
	ctx.Ms.Commit(e)
}

func JobUpdateStatus(jobId uint, status uint, ctx *entity.Context) {
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, _ := ctx.Ms.JobUpdateStatus(jobId, status)
	ctx.Ms.Commit(e)
}

func JobStatusQuery(jobId uint, ctx *entity.Context) uint {
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, u := ctx.Ms.JobGetByJobID(jobId)
	ctx.Ms.Commit(e)
	return u.Status

}
