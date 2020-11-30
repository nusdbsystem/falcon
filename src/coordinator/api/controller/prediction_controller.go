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

	e2, u := ctx.Ms.CreateService(appName, u1.ID, extInfo)

	e3, u2 := ctx.Ms.JobGetByJobID(jobId)

	ctx.Ms.Commit([]error{e1, e2, e3})


	var pInfo []config.PartyInfo

	err := json.Unmarshal([]byte(u2.PartyIds), &pInfo)
	if err != nil {
		panic("json.Unmarshal(PartyIds) error")
	}


	var iPs []string

	var partyPath []config.PartyPath
	var taskInfos config.Tasks

	for _, v := range pInfo {

		// list of ip
		iPs = append(iPs, v.IP)

		// list of ip
		partyPath = append(partyPath, v.PartyPaths)
	}

	qItem := new(config.QItem)
	qItem.IPs = iPs
	qItem.JobId = jobId
	qItem.PartyPath = partyPath
	qItem.TaskInfos = taskInfos

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