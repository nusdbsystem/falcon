package router

import (
	"coordinator/api/controller"
	"coordinator/api/entity"
	"coordinator/client"
	"coordinator/config"
	"net/http"
	"strconv"
)

func ModelUpdate(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)
	JobId := r.FormValue(config.JobId)
	IsTrained := r.FormValue(config.IsTrained)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}

	isTrained, e := strconv.Atoi(IsTrained)
	if e != nil {
		panic(e)
	}

	controller.ModelUpdate(uint(jobId), uint(isTrained),  ctx)
}
