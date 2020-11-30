package controller

import (
	"coordinator/api/entity"
	"coordinator/config"
	dist "coordinator/distributed"
	"encoding/json"
)




func CreateService(jobId uint, appName, extInfo string, ctx *entity.Context) (uint,string){

	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e1, u1 := ctx.Ms.ModelGetByID(jobId)

	e2, u := ctx.Ms.CreateService(appName, u1.ID, jobId, extInfo)

	e3, u2 := ctx.Ms.JobGetByJobID(jobId)

	ctx.Ms.Commit([]error{e1, e2, e3})


	var pInfo []config.PartyInfo
	var taskInfos config.Tasks

	err := json.Unmarshal([]byte(u2.PartyIds), &pInfo)
	err2 := json.Unmarshal([]byte(u2.TaskInfos), &taskInfos)
	if err != nil || err2!=nil {
		panic("json.Unmarshal(PartyIds or TaskInfos) error")
	}

	iPs ,partyPath, modelPath, executablePath := config.ParsePartyInfo(pInfo, taskInfos)

	qItem := new(config.QItem)
	qItem.IPs = iPs
	qItem.JobId = jobId
	qItem.PartyPath = partyPath
	qItem.TaskInfos = taskInfos
	qItem.ModelPath = modelPath
	qItem.ExecutablePath = executablePath

	go dist.SetupDist(ctx.HttpHost, ctx.HttpPort, qItem, config.PredictTaskType)

	return u.ID, u.ModelServiceName
}


func UpdateService(dsl *config.DSL, ctx *entity.Context) {

}


func QueryService(dsl *config.DSL, ctx *entity.Context) {

}


func StopService(dsl *config.DSL, ctx *entity.Context) {

}


func DeleteService(dsl *config.DSL, ctx *entity.Context) {

}


func PublishService(jobId uint, isPublish uint, ctx *entity.Context) {
	ctx.Ms.Tx = ctx.Ms.Db.Begin()

	e, _ := ctx.Ms.PublishService(jobId, isPublish)
	ctx.Ms.Commit(e)
}


func LaunchService(dsl *config.DSL, ctx *entity.Context) {
}


func ModelServiceUpdateStatus(jobId uint, status uint, ctx *entity.Context){
	ctx.Ms.Tx = ctx.Ms.Db.Begin()
	e, _ := ctx.Ms.ModelServiceUpdateStatus(jobId, status)
	ctx.Ms.Commit(e)

}