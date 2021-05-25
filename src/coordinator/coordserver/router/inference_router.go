package router

import (
	"bytes"
	"encoding/json"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/entity"
	"falcon_platform/logger"
	"net/http"
	"strconv"
)

type InferenceRes struct {
	InferenceStatus string `json:"inference_status"`
}

func CreateInference(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	// limit the max input length!
	_ = r.ParseMultipartForm(32 << 20)

	var buf bytes.Buffer

	err, contents := client.ReceiveFile(r, buf, common.JobFile)
	if err != nil {
		logger.Log.Println(err)
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

	status, _ := controller.CreateInference(InferenceJob, ctx)

	w.Header().Set("Content-Type", "application/json")

	resIns := InferenceRes{}

	if status == false {
		resIns.InferenceStatus = "Training process not finished"

	} else {
		resIns.InferenceStatus = "Inference job Submitted"
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

	// 1. parse the dsl info,
	logger.Log.Println("[UpdateInference]: parse the dsl info,")
	_ = r.ParseMultipartForm(32 << 20)

	var buf bytes.Buffer

	err, contents := client.ReceiveFile(r, buf, common.JobFile)
	if err != nil {
		logger.Log.Println("client.ReceiveFile file error", err)
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	var InferenceJob common.InferenceJob
	e := common.ParseInferenceJob(contents, &InferenceJob)
	if e != nil {
		http.Error(w, e.Error(), http.StatusBadRequest)
		return
	}

	// 2. get current running inference jobs under user_id and job_name
	logger.Log.Printf("[UpdateInference]: get current running inference jobs under user_id:%d and job_name:%s \n", ctx.UsrId, InferenceJob.JobName)
	InferenceIds := controller.QueryRunningInferenceJobs(InferenceJob.JobName, ctx)

	// 3. create new inference job
	logger.Log.Println("[UpdateInference]: create new inference job InferenceJob:", InferenceJob)
	status, newInfId := controller.CreateInference(InferenceJob, ctx)

	// 4. if true, delete previous running job once it is running
	if status == true {
		logger.Log.Println("[UpdateInference]: CreateInference return true, now begin to delete previous running jobs, inferenceIds", InferenceIds)
		go controller.UpdateInference(newInfId, InferenceIds, ctx)
	} else {
		logger.Log.Println("[UpdateInference]: CreateInference return false")
		panic("Unable to update")
	}

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

func InferenceUpdateMaster(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	InferenceId := r.FormValue(common.JobId)
	MasterAddr := r.FormValue(common.MasterAddrKey)

	infId, e := strconv.Atoi(InferenceId)
	if e != nil {
		panic(e)
	}

	controller.InferenceUpdateMaster(uint(infId), MasterAddr, ctx)

}
