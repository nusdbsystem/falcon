package controller

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/coordserver/entity"
	dist "coordinator/distributed"
	"coordinator/logger"
	"encoding/json"
)



func CreateService(jobId uint, appName, extInfo string, ctx *entity.Context) (uint,string){

	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e1, u1 := ctx.Ms.ModelGetByID(jobId)

	e2, u := ctx.Ms.CreateService(appName, u1.ID, jobId, extInfo)

	e3, u2 := ctx.Ms.JobGetByJobID(jobId)

	ctx.Ms.Commit([]error{e1, e2, e3})


	var pInfo []common.PartyInfo
	var taskInfos common.Tasks

	err := json.Unmarshal([]byte(u2.PartyIds), &pInfo)
	err2 := json.Unmarshal([]byte(u2.TaskInfos), &taskInfos)
	if err != nil || err2!=nil {
		panic("json.Unmarshal(PartyIds or TaskInfos) error")
	}

	iPs := common.ParseIps(pInfo)

	qItem := new(cache.QItem)
	qItem.IPs = iPs
	qItem.JobId = jobId
	qItem.PartyInfos = pInfo
	qItem.Tasks = taskInfos

	go func(){
		defer logger.HandleErrors()
		dist.SetupDist(qItem, common.PredictExecutor)
	}()

	return u.ID, u.ModelServiceName
}


func UpdateService(job *common.Job, ctx *entity.Context) {

}


func QueryService(job *common.Job, ctx *entity.Context) {

}


func StopService(job *common.Job, ctx *entity.Context) {

}


func DeleteService(job *common.Job, ctx *entity.Context) {

}


func PublishService(jobId uint, isPublish uint, ctx *entity.Context) {
	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e, _ := ctx.Ms.PublishService(jobId, isPublish)
	ctx.Ms.Commit(e)
}


func LaunchService(job *common.Job, ctx *entity.Context) {
}


func ModelServiceUpdateStatus(jobId uint, status uint, ctx *entity.Context){
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, _ := ctx.Ms.ModelServiceUpdateStatus(jobId, status)
	ctx.Ms.Commit(e)

}