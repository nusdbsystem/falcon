package router

import (
	"coordinator/api/controller"
	"coordinator/api/entity"
	"coordinator/client"
	"coordinator/config"
	"net/http"
	"strconv"
)



func PublishService(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)
	JobId := r.FormValue(config.JobId)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}

	controller.PublishService(uint(jobId), 1, ctx)

}



func CreateService(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)
	JobId := r.FormValue(config.JobId)
	appName := r.FormValue(config.AppName)
	extInfo := r.FormValue(config.ExtInfo)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}

	controller.CreateService(uint(jobId), appName, extInfo, ctx)

}


func UpdateService(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}


func QueryService(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}


func StopService(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}


func DeleteService(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}



func LaunchService(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}

