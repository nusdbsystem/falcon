package router

import (
	"bytes"
	"encoding/json"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/entity"
	"falcon_platform/exceptions"
	"falcon_platform/logger"
	"fmt"
	"net/http"
	"strconv"

	"github.com/gorilla/mux"
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

// receive a job info file, parse it, put in queue
func SubmitTrainJobFile(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	// Parse multipart form, 32 << 20 specifies a maximum
	// upload of 32 MB files.
	r.ParseMultipartForm(32 << 20)

	var buf bytes.Buffer

	err, contents := client.ReceiveFile(r, buf, common.TrainJobFileKey)
	if err != nil {
		errMsg := fmt.Sprintf("client.ReceiveFile Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusBadRequest, errMsg)
		return
	}
	logger.Log.Println("client.ReceiveFile success")

	// parse it
	var job common.TrainJob

	err = common.ParseTrainJob(contents, &job)
	if err != nil {
		errMsg := fmt.Sprintf("common.ParseJob Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusBadRequest, errMsg)
		return
	}
	logger.Log.Println("common.ParseJob success")

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

	// TODO: fix json Marshal add /" '/" to the partyIds
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
	// read the query parameters with gorilla mux
	params := mux.Vars(r)
	jobId, _ := strconv.Atoi(params["jobId"])

	controller.JobKill(uint(jobId), ctx)

	jr := JobStatusRes{
		JobId:  uint(jobId),
		Status: common.JobKilled,
	}

	js, err := json.Marshal(jr)

	if err != nil {
		exceptions.HandleHttpError(w, r, http.StatusBadRequest, err.Error())
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

	// read the query parameters with gorilla mux
	params := mux.Vars(r)
	jobId, _ := strconv.Atoi(params["jobId"])

	status := controller.JobStatusQuery(uint(jobId), ctx)

	jr := JobStatusRes{
		JobId:  uint(jobId),
		Status: status,
	}

	js, err := json.Marshal(jr)

	if err != nil {
		exceptions.HandleHttpError(w, r, http.StatusBadRequest, err.Error())
		return
	}
	_, _ = w.Write(js)

}
