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
	"falcon_platform/utils"
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"github.com/gorilla/mux"
)

type JobSubmitRes struct {
	JobId   uint   `json:"job_id"`
	JobName string `json:"job_name"`
	UserId  uint   `json:"user_id"`
	TaskNum uint   `json:"task_num,uint"`
	Status  string `json:"status"`
}

type JobStatusRes struct {
	JobId  uint   `json:"job_id"`
	Status string `json:"status"`
}

type JobIdGet struct {
	JobId string `json:"job_id"`
}

type JobModelReportRes struct {
	JobId                uint   `json:"job_id"`
	EvaluationReportPath string `json:"evaluation_report_path"`
}

// receive a job from jsonbody or file, parse it, put in dslqueue
func SubmitTrainJob(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	// declare the job as train job type
	var job common.TrainJob

	contentType := r.Header.Get("Content-Type")
	contentType = strings.TrimSpace(strings.Split(contentType, ";")[0])

	if contentType == common.JsonContentType {
		// 2. check json body first
		jsonErr := json.NewDecoder(r.Body).Decode(&job)
		if jsonErr != nil {
			exceptions.HandleHttpError(w, r, http.StatusBadRequest, jsonErr.Error())
			return
		}

		logger.Log.Println("[Coordinator]: Submit train job with json body")

	} else if contentType == common.MultipartContentType {
		var buf bytes.Buffer
		// Parse multipart form, 32 << 20 specifies a maximum,upload of 32 MB files.
		_ = r.ParseMultipartForm(32 << 20)
		multiPartyFormErr, contents := client.ReceiveFile(r, buf, common.TrainJobFileKey)
		if multiPartyFormErr != nil {
			exceptions.HandleHttpError(w, r, http.StatusBadRequest, multiPartyFormErr.Error())
			buf.Reset()
			return
		}

		logger.Log.Println("[Coordinator]: Submit train job with multipart form")
		// 2. parser to object
		err := common.ParseTrainJob(contents, &job)
		if err != nil {
			exceptions.HandleHttpError(w, r, http.StatusBadRequest, err.Error())
			buf.Reset()
			return
		}

		buf.Reset()

	} else {
		exceptions.HandleHttpError(w, r, http.StatusBadRequest,
			"Can not read job dsl form either json body or multi party, Set the "+
				"\"Content-type: application/json\" or \"Content-Type: multipart/form-data\"")
		return
	}

	// 3. submit job with parsed object
	JobId, JobName, UserId, TaskNum, Status := controller.JobSubmit(&job, ctx)

	// 4. return to client

	resIns := JobSubmitRes{
		JobId,
		JobName,
		UserId,
		TaskNum,
		Status}

	err := json.NewEncoder(w).Encode(resIns)

	if err != nil {
		errMsg := fmt.Sprintf("JSON Marshal Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, errMsg)
		return
	}
}

func JobKill(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	// read the query parameters with gorilla mux
	params := mux.Vars(r)
	jobId, _ := strconv.Atoi(params["jobId"])
	var jr JobStatusRes
	status, e := controller.JobStatusQuery(uint(jobId), ctx)
	if e != nil {
		exceptions.HandleHttpError(w, r, http.StatusBadRequest, e.Error())
		return
	}
	if status == common.JobFailed || status == common.JobKilled || status == common.JobSuccessful {
		jr = JobStatusRes{
			JobId:  uint(jobId),
			Status: "Job already stopped with status: " + status,
		}
	} else {
		err := controller.JobKill(uint(jobId), ctx)
		if err != nil {
			exceptions.HandleHttpError(w, r, http.StatusBadRequest, err.Error())
			return
		}
		jr = JobStatusRes{
			JobId:  uint(jobId),
			Status: common.JobKilled,
		}
	}

	err := json.NewEncoder(w).Encode(jr)

	if err != nil {
		errMsg := fmt.Sprintf("JSON Marshal Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, errMsg)
		return
	}

}

func JobUpdateMaster(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	JobId := r.FormValue(common.JobId)
	MasterAddr := r.FormValue(common.MasterAddrKey)

	jobId, _ := strconv.Atoi(JobId)

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

	controller.JobUpdateStatus(uint(jobId), JobStatus, ctx)
}

func JobUpdateResInfo(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)
	JobId := r.FormValue(common.JobId)
	JobErrMsg := r.FormValue(common.JobErrMsg)
	JobResult := r.FormValue(common.JobResult)
	JobExtInfo := r.FormValue(common.JobExtInfo)

	jobId, _ := strconv.Atoi(JobId)

	controller.JobUpdateResInfo(uint(jobId), JobErrMsg, JobResult, JobExtInfo, ctx)
}

func JobStatusQuery(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	// read the query parameters with gorilla mux
	params := mux.Vars(r)
	jobId, _ := strconv.Atoi(params["jobId"])

	status, err := controller.JobStatusQuery(uint(jobId), ctx)

	if err != nil {
		exceptions.HandleHttpError(w, r, http.StatusBadRequest, err.Error())
		return
	}

	jr := JobStatusRes{
		JobId:  uint(jobId),
		Status: status,
	}

	err = json.NewEncoder(w).Encode(jr)

	if err != nil {
		errMsg := fmt.Sprintf("JSON Marshal Error %s", err)
		exceptions.HandleHttpError(w, r, http.StatusInternalServerError, errMsg)
		return
	}

}

func JobTrainingReportRetrieve(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	//params := mux.Vars(r)
	//jobId, _ := strconv.Atoi(params["jobId"])

	filename := "/opt/falcon/src/falcon_platform/web/build/static/media/model_report"
	reportStr, _ := utils.ReadFile(filename)

	_, _ = w.Write([]byte(reportStr))

	//res := JobModelReportRes{
	//	JobId:                uint(jobId),
	//	EvaluationReportPath: filename,
	//}
	//err := json.NewEncoder(w).Encode(res)
	//
	//if err != nil {
	//	errMsg := fmt.Sprintf("JSON Marshal Error %s", err)
	//	exceptions.HandleHttpError(w, r, http.StatusInternalServerError, errMsg)
	//	return
	//}

}
