package controller

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/coordserver/entity"
	"coordinator/distributed"
	"coordinator/logger"
	"encoding/json"
)

func JobSubmit(job *common.Job, ctx *entity.Context) (uint, string, uint, string, uint, uint) {

	logger.Do.Println("HTTP server: in SubmitJob, put to the JobQueue")

	// generate.sh item pushed to the queue

	iPs := common.ParseIps(job.PartyInfo)

	// generate.sh strings used to write to db
	partyIds, err := json.Marshal(job.PartyInfo)
	taskInfos, err := json.Marshal(job.Tasks)
	if err != nil {
		panic("json.Marshal(job.PartyIds) error")
	}

	ModelName :=  job.Tasks.ModelTraining.AlgorithmName
	ModelDecs := job.Tasks.ModelTraining.AlgorithmName
	PartyNumber := uint(len(job.PartyInfo))
	ExtInfo := job.Tasks.ModelTraining.AlgorithmName

	// write to db
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	errs, u := ctx.JobDB.JobSubmit(job.JobName, ctx.UsrId, string(partyIds), string(taskInfos), job.JobDecs, job.TaskNum, common.JobInit)
	err2, _ := ctx.JobDB.SvcCreate(u.JobId)

	err3, _ := ctx.JobDB.ModelCreate(
		u.JobId,
		ModelName,
		ModelDecs,
		PartyNumber,
		string(partyIds),
		ExtInfo)

	ctx.JobDB.Commit([]error{errs, err2, err3})


	qItem := new(cache.QItem)
	qItem.IPs = iPs
	qItem.JobId = u.JobId
	qItem.JobName = job.JobName
	qItem.JobFlType = job.JobFlType
	qItem.ExistingKey = job.ExistingKey
	qItem.PartyNums = job.PartyNums
	qItem.PartyInfo = job.PartyInfo
	qItem.Tasks = job.Tasks

	go func() {
		cache.JobQueue.Push(qItem)
	}()

	return u.JobId, u.JobName, u.UserID, u.PartyIds, u.TaskNum, u.Status
}

func JobKill(jobId uint, ctx *entity.Context) {

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, u := ctx.JobDB.JobGetByJobID(jobId)
	ctx.JobDB.Commit(e)

	distributed.KillJob(u.MasterUrl, common.Proxy)

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e2, _ := ctx.JobDB.JobUpdateStatus(jobId, common.JobKilled)
	ctx.JobDB.Commit(e2)
}

func JobUpdateMaster(jobId uint, masterUrl string, ctx *entity.Context) {
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, _ := ctx.JobDB.SvcUpdateMaster(jobId, masterUrl)
	e2, _ := ctx.JobDB.JobUpdateMaster(jobId, masterUrl)
	ctx.JobDB.Commit([]error{e, e2})
}

func JobUpdateResInfo(jobId uint, jobErrMsg, jobResult, jobExtInfo string, ctx *entity.Context) {
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, _ := ctx.JobDB.JobUpdateResInfo(jobId, jobErrMsg, jobResult, jobExtInfo)
	ctx.JobDB.Commit(e)
}

func JobUpdateStatus(jobId uint, status uint, ctx *entity.Context) {
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, _ := ctx.JobDB.JobUpdateStatus(jobId, status)
	ctx.JobDB.Commit(e)
}

func JobStatusQuery(jobId uint, ctx *entity.Context) uint {
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, u := ctx.JobDB.JobGetByJobID(jobId)
	ctx.JobDB.Commit(e)
	return u.Status

}
