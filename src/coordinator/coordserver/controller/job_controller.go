package controller

import (
	"coordinator/coordserver/entity"
	"coordinator/cache"
	"coordinator/common"
	"coordinator/distributed"
	"coordinator/logger"
	"encoding/json"
)

func JobSubmit(dsl *common.DSL, ctx *entity.Context) (uint, string, uint, string, uint, uint) {

	logger.Do.Println("HTTP server: in SubmitJob, put to the JobQueue")

	// generate.sh item pushed to the queue

	iPs := common.ParseIps(dsl.PartyInfos)

	// generate.sh strings used to write to db
	partyIds, err := json.Marshal(dsl.PartyInfos)
	taskInfos, err := json.Marshal(dsl.Tasks)
	if err != nil {
		panic("json.Marshal(dsl.PartyIds) error")
	}

	ModelName :=  dsl.Tasks.ModelTraining.AlgorithmName
	ModelDecs := dsl.Tasks.ModelTraining.AlgorithmName
	PartyNumber := uint(len(dsl.PartyInfos))
	ExtInfo := dsl.Tasks.ModelTraining.AlgorithmName

	// write to db
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	errs, u := ctx.Ms.JobSubmit(dsl.JobName, ctx.UsrId, string(partyIds), string(taskInfos), dsl.JobDecs, dsl.TaskNum, common.JobInit)
	err2, _ := ctx.Ms.SvcCreate(u.JobId)

	err3, _ := ctx.Ms.ModelCreate(
		u.JobId,
		ModelName,
		ModelDecs,
		PartyNumber,
		string(partyIds),
		ExtInfo)

	ctx.Ms.Commit([]error{errs, err2, err3})


	qItem := new(cache.QItem)
	qItem.IPs = iPs
	qItem.JobId = u.JobId
	qItem.JobName = dsl.JobName
	qItem.JobFlType = dsl.JobFlType
	qItem.ExistingKey = dsl.ExistingKey
	qItem.PartyNums = dsl.PartyNums
	qItem.PartyInfos = dsl.PartyInfos
	qItem.Tasks = dsl.Tasks

	go func() {
		cache.JobQueue.Push(qItem)
	}()

	return u.JobId, u.JobName, u.UserID, u.PartyIds, u.TaskNum, u.Status
}

func JobKill(jobId uint, ctx *entity.Context) {

	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, u := ctx.Ms.JobGetByJobID(jobId)
	ctx.Ms.Commit(e)

	distributed.KillJob(u.MasterAddress, common.Proxy)

	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e2, _ := ctx.Ms.JobUpdateStatus(jobId, common.JobKilled)
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
