package router

import (
	"coordinator/coordserver/controller"
	"coordinator/coordserver/entity"
	"coordinator/client"
	"coordinator/common"
	"net/http"
	"strconv"
)

func ModelUpdate(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)
	JobId := r.FormValue(common.JobId)
	IsTrained := r.FormValue(common.IsTrained)

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
