package controller

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/coordserver/entity"
	"coordinator/distributed"
	"coordinator/logger"
	"encoding/json"
)

func JobSubmit(job *common.DSL, ctx *entity.Context) (uint, string, uint, string, uint, uint) {

	logger.Do.Println("HTTP server: in SubmitJob, put to the JobQueue")

	// generate.sh item pushed to the queue

	iPs := common.ParseIps(job.PartyInfos)

	// generate.sh strings used to write to db
	partyIds, err := json.Marshal(job.PartyInfos)
	taskInfos, err := json.Marshal(job.Tasks)
	if err != nil {
		panic("json.Marshal(job.PartyIds) error")
	}

	ModelName :=  job.Tasks.ModelTraining.AlgorithmName
	ModelDecs := job.Tasks.ModelTraining.AlgorithmName
	PartyNumber := uint(len(job.PartyInfos))
	ExtInfo := job.Tasks.ModelTraining.AlgorithmName

	// write to db
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	errs, u := ctx.Ms.JobSubmit(job.JobName, ctx.UsrId, string(partyIds), string(taskInfos), job.JobDecs, job.TaskNum, common.JobInit)
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
	qItem.JobName = job.JobName
	qItem.JobFlType = job.JobFlType
	qItem.ExistingKey = job.ExistingKey
	qItem.PartyNums = job.PartyNums
	qItem.PartyInfos = job.PartyInfos
	qItem.Tasks = job.Tasks

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
