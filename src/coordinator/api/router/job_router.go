package router

import (
	"bytes"
	"coordinator/api/controller"
	"coordinator/api/entity"
	"coordinator/client"
	"coordinator/config"
	"encoding/json"
	"log"
	"net/http"
	"strconv"
)

type JobSubmitRes struct {
	JobId    uint   `json:"job_id"`
	JobName  string `json:"job_name"`
	UserId   uint   `json:"user_id"`
	PartyIds string `json:"party_ids"`
	TaskNum  uint   `json:"task_num,uint"`
	Status   uint   `json:"status"`
}

type JobStatusRes struct {
	JobId  uint `json:"job_id"`
	Status uint `json:"status"`
}

type JobIdGet struct {
	JobId string `json:"job_id"`
}

// receive a job info file,parse it, put ti queue
func JobSubmit(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	// limit the max input length!
	_ = r.ParseMultipartForm(32 << 20)

	var buf bytes.Buffer

	err, contents := client.ReceiveFile(r, buf, config.DslFile)
	if err != nil {
		log.Println(err)
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// parse it
	var dsl config.DSL

	e := config.ParseDsl(contents, &dsl)

	if e != nil {
		http.Error(w, e.Error(), http.StatusBadRequest)
		return
	}

	// submit it
	JobId, JobName, UserId, PartyIds, TaskNum, Status := controller.JobSubmit(&dsl, ctx)

	buf.Reset()

	w.Header().Set("Content-Type", "application/json")

	resIns := JobSubmitRes{
		JobId,
		JobName,
		UserId,
		PartyIds,
		TaskNum,
		Status}

	js, err := json.Marshal(resIns)

	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	//json.NewEncoder(w).Encode(js)
	_, _ = w.Write(js)

}

func JobKill(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	JobId := r.FormValue(config.JobId)
	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}
	controller.JobKill(uint(jobId), ctx)

	jr := JobStatusRes{
		JobId:  uint(jobId),
		Status: config.JobKilled,
	}

	js, err := json.Marshal(jr)

	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	_, _ = w.Write(js)

}

func JobUpdateMaster(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	JobId := r.FormValue(config.JobId)
	MasterAddr := r.FormValue(config.MasterAddr)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}

	controller.JobUpdateMaster(uint(jobId), MasterAddr, ctx)

}

func JobUpdateStatus(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)
	JobId := r.FormValue(config.JobId)
	JobStatus := r.FormValue(config.JobStatus)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}
	jobStatus, e2 := strconv.Atoi(JobStatus)
	if e2 != nil {
		panic(e2)
	}

	controller.JobUpdateStatus(uint(jobId), uint(jobStatus), ctx)
}

func JobUpdateResInfo(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)
	JobId := r.FormValue(config.JobId)
	JobErrMsg := r.FormValue(config.JobErrMsg)
	JobResult := r.FormValue(config.JobResult)
	JobExtInfo := r.FormValue(config.JobExtInfo)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}

	controller.JobUpdateResInfo(uint(jobId), JobErrMsg, JobResult, JobExtInfo, ctx)
}

func JobStatusQuery(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	var getBody JobIdGet

	err := json.NewDecoder(r.Body).Decode(&getBody)

	if err != nil {
		panic(err)
	}

	jobId, e := strconv.Atoi(getBody.JobId)
	if e != nil {
		panic(e)
	}

	status := controller.JobStatusQuery(uint(jobId), ctx)

	jr := JobStatusRes{
		JobId:  uint(jobId),
		Status: status,
	}

	js, err := json.Marshal(jr)

	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	_, _ = w.Write(js)

}
