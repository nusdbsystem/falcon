package router

import (
	"bytes"
	"encoding/json"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/entity"
	"falcon_platform/exceptions"
	"fmt"
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

	err, contents := client.ReceiveFile(r, buf, common.JobFile)
	if err != nil {
		errMsg := fmt.Sprintf("ReceiveFile Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, errMsg)
		return
	}

	// parse it
	var job common.TrainJob

	e := common.ParseTrainJob(contents, &job)

	if e != nil {
		errMsg := fmt.Sprintf("ParseJob Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, errMsg)
		return
	}

	// submit it
	JobId, JobName, UserId, PartyIds, TaskNum, Status := controller.JobSubmit(&job, ctx)

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
		errMsg := fmt.Sprintf("JSON Marshal Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, errMsg)
		return
	}
	//json.NewEncoder(w).Encode(js)
	_, _ = w.Write(js)

}

func JobKill(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	JobId := r.FormValue(common.JobId)
	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}
	controller.JobKill(uint(jobId), ctx)

	jr := JobStatusRes{
		JobId:  uint(jobId),
		Status: common.JobKilled,
	}

	js, err := json.Marshal(jr)

	if err != nil {
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, err.Error())
		return
	}
	_, _ = w.Write(js)

}

func JobUpdateMaster(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	JobId := r.FormValue(common.JobId)
	MasterAddr := r.FormValue(common.MasterAddrKey)

	jobId, e := strconv.Atoi(JobId)
	if e != nil {
		panic(e)
	}

	controller.JobUpdateMaster(uint(jobId), MasterAddr, ctx)

}

func JobUpdateStatus(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
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

	controller.JobUpdateStatus(uint(jobId), uint(jobStatus), ctx)
}

func JobUpdateResInfo(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)
	JobId := r.FormValue(common.JobId)
	JobErrMsg := r.FormValue(common.JobErrMsg)
	JobResult := r.FormValue(common.JobResult)
	JobExtInfo := r.FormValue(common.JobExtInfo)

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
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, err.Error())
		return
	}
	_, _ = w.Write(js)

}
