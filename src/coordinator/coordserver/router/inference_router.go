package router

import (
	"bytes"
	"coordinator/coordserver/controller"
	"coordinator/coordserver/entity"
	"coordinator/client"
	"coordinator/common"
	"coordinator/distributed"
	"coordinator/logger"
	"net/http"
	"strconv"
	"encoding/json"
	"time"
)


type InferenceRes struct {
	InferenceStatus   	string   `json:"inference_status"`
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

	status,_ := controller.CreateInference(InferenceJob, ctx)

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

	// 1. parse the dsl info,
	_ = r.ParseMultipartForm(32 << 20)

	var buf bytes.Buffer

	err, contents := client.ReceiveFile(r, buf, common.JobFile)
	if err != nil {
		logger.Do.Println("client.ReceiveFile file error",err)
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

	InferenceIds := controller.QueryRunningInferenceJobs(InferenceJob.JobName,ctx)

	// 3. create new inference job
	status, newInfId := controller.CreateInference(InferenceJob, ctx)

	// 4. if true, delete previous running job once it is running
	if status == true{

		go func(){

			MaxCheck := 10
			loop:
				for{
					if MaxCheck <0{
						logger.Do.Println("UpdateInference: Update Failed, latest job is nor running")
						break
					}

					e, u := ctx.JobDB.InferenceGetByID(newInfId)
					if e!=nil{
						panic(e)}

					// if the latest inference is running, stop the old one
					if u.Status == common.JobRunning{
						for _, infId := range InferenceIds{

							e, u := ctx.JobDB.InferenceGetByID(infId)
							if e!=nil{
								panic(e)}

							distributed.KillJob(u.MasterAddr, common.Proxy)

							tx := ctx.JobDB.Db.Begin()
							e, _ = ctx.JobDB.InferenceUpdateStatus(tx, infId, common.JobKilled)
							ctx.JobDB.Commit(tx, e)
						}
						logger.Do.Println("UpdateInference: Update successfully")
						break loop
					}else{
						MaxCheck--
					}
					// check db every 3 second
					time.Sleep(time.Second*3)
				}
		}()

	}else{
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