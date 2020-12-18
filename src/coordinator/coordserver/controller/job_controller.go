package controller

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/coordserver/entity"
	"coordinator/distributed"
	"coordinator/logger"
	"encoding/json"
)

func JobSubmit(job *common.TrainJob, ctx *entity.Context) (uint, string, uint, string, uint, uint) {

	logger.Do.Println("HTTP server: in SubmitJob, put to the JobQueue")

	// generate.sh item pushed to the queue

	addresses := common.ParseAddress(job.PartyInfo)

	// generate.sh strings used to write to db
	partyIds, err := json.Marshal(job.PartyInfo)
	TaskInfo, err := json.Marshal(job.Tasks)
	if err != nil {
		panic("json.Marshal(job.PartyIds) error")
	}

	ModelName :=  job.Tasks.ModelTraining.AlgorithmName
	ModelDecs := job.Tasks.ModelTraining.AlgorithmName
	PartyNumber := uint(len(job.PartyInfo))

	// write to db
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	err1, u1 := ctx.JobDB.JobInfoCreate(
		job.JobName,
		ctx.UsrId,
		string(partyIds),
		string(TaskInfo),
		job.JobDecs,
		PartyNumber,
		job.JobFlType,
		job.ExistingKey,
		job.TaskNum)

	err2, u2 := ctx.JobDB.JobSubmit(ctx.UsrId, common.JobInit, u1.Id)
	err3, _ := ctx.JobDB.SvcCreate(u2.JobId)

	err4, _ := ctx.JobDB.ModelCreate(u2.JobId, ModelName, ModelDecs)

	ctx.JobDB.Commit([]error{err1, err2, err3, err4})


	qItem := new(cache.QItem)
	qItem.AddrList = addresses
	qItem.JobId = u2.JobId
	qItem.JobName = job.JobName
	qItem.JobFlType = job.JobFlType
	qItem.ExistingKey = job.ExistingKey
	qItem.PartyNums = job.PartyNums
	qItem.PartyInfo = job.PartyInfo
	qItem.Tasks = job.Tasks

	go func() {
		cache.JobQueue.Push(qItem)
	}()

	return u2.JobId, u1.JobName, u2.UserID, u1.PartyIds, u1.TaskNum, u2.Status
}

func JobKill(jobId uint, ctx *entity.Context) {

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, u := ctx.JobDB.JobGetByJobID(jobId)
	ctx.JobDB.Commit(e)

	distributed.KillJob(u.MasterAddr, common.Proxy)

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e2, _ := ctx.JobDB.JobUpdateStatus(jobId, common.JobKilled)
	ctx.JobDB.Commit(e2)
}

func JobUpdateMaster(jobId uint, masterAddr string, ctx *entity.Context) {
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, _ := ctx.JobDB.SvcUpdateMaster(jobId, masterAddr)
	e2, _ := ctx.JobDB.JobUpdateMaster(jobId, masterAddr)
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
