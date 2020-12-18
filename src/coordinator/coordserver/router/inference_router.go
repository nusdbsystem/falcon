package router

import (
	"bytes"
	"coordinator/coordserver/controller"
	"coordinator/coordserver/entity"
	"coordinator/client"
	"coordinator/common"
	"coordinator/logger"
	"net/http"
	"strconv"
	"encoding/json"

)


type InferenceRes struct {
	InferenceStatus   	string   `json:"inference_status"`
}


func PublishInference(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	// label the model is trained
	client.ReceiveForm(r)
	JobId := r.FormValue(common.JobId)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}

	controller.PublishInference(uint(jobId), 1, ctx)
}



func CreateInference(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	// limit the max input length!
	_ = r.ParseMultipartForm(32 << 20)

	var buf bytes.Buffer

	err, contents := client.ReceiveFile(r, buf, common.JobFile)
	if err != nil {
		logger.Do.Println(err)
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// parse it
	var InferenceJob common.InferenceJob

	e := common.ParseInferenceJob(contents, &InferenceJob)

	if e != nil {
		http.Error(w, e.Error(), http.StatusBadRequest)
		return
	}

	status := controller.CreateInference(InferenceJob, ctx)

	w.Header().Set("Content-Type", "application/json")

	resIns := InferenceRes{}

	if status == false{
		resIns.InferenceStatus="Training process not finished"

	}else{
		resIns.InferenceStatus="Inference job Submitted"
	}

	js, err := json.Marshal(resIns)

	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	//json.NewEncoder(w).Encode(js)
	_, _ = w.Write(js)

}


func UpdateInference(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}


func QueryInference(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}


func StopInference(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

}


func UpdateInferenceStatus(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)
	JobId := r.FormValue(common.JobId)
	JobStatus := r.FormValue(common.JobStatus)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}
	jobStatus, e2 := strconv.Atoi(JobStatus)
	if e2 != nil {
		panic(e2)
	}

	controller.InferenceUpdateStatus(uint(jobId), uint(jobStatus), ctx)

}
