package router

import (
	"falcon_platform/common"
	"falcon_platform/coordserver/middleware"
	"net/http"

	"github.com/gorilla/mux"
)

func NewRouter() *mux.Router {
	r := mux.NewRouter()

	middleware.SysLvPath = []string{common.PartyServerAdd}

	// match html views to routes
	// r.HandleFunc("/", HtmlIndex).Methods("GET")
	r.HandleFunc("/submit-train-job", HtmlSubmitTrainJob).Methods("GET")
	r.HandleFunc("/upload-train-job-file", HtmlUploadTrainJobFile).Methods("GET")

	// REST APIs
	//job
	r.HandleFunc(common.SubmitTrainJob, middleware.AddRouter(SubmitTrainJob)).Methods("GET", "POST")
	r.HandleFunc(common.StopTrainJob+"/{jobId}", middleware.AddRouter(JobKill)).Methods("GET")
	r.HandleFunc(common.UpdateTrainJobMaster, middleware.AddRouter(JobUpdateMaster)).Methods("POST")
	r.HandleFunc(common.UpdateJobStatus, middleware.AddRouter(JobUpdateStatus)).Methods("POST")
	r.HandleFunc(common.UpdateJobResInfo, middleware.AddRouter(JobUpdateResInfo)).Methods("POST")
	r.HandleFunc(common.QueryTrainJobStatus+"/{jobId}", middleware.AddRouter(JobStatusQuery)).Methods("GET")

	//party server
	r.HandleFunc(common.PartyServerAdd, middleware.AddRouter(PartyServerAdd)).Methods("POST")
	r.HandleFunc(common.PartyServerDelete, middleware.AddRouter(PartyServerDelete)).Methods("POST")
	r.HandleFunc(common.GetPartyServerPort, middleware.AddRouter(GetPartyServerPort)).Methods("GET")

	// model serving
	r.HandleFunc(common.ModelUpdate, middleware.AddRouter(ModelUpdate)).Methods("POST")

	// prediction service
	r.HandleFunc(common.UpdateInferenceJobMaster, middleware.AddRouter(InferenceUpdateMaster)).Methods("POST")
	r.HandleFunc(common.InferenceUpdate, middleware.AddRouter(UpdateInference)).Methods("POST")
	r.HandleFunc(common.InferenceCreate, middleware.AddRouter(CreateInference)).Methods("POST")
	r.HandleFunc(common.InferenceStatusUpdate, middleware.AddRouter(UpdateInferenceStatus)).Methods("POST")

	// resource
	r.HandleFunc(common.AssignPort, middleware.AddRouter(AssignPort)).Methods("GET")
	r.HandleFunc(common.AddPort, middleware.AddRouter(AddPort)).Methods("POST")

	// static files
	// react build static htmls
	buildHandler := http.FileServer(http.Dir("web/build"))
	r.PathPrefix("/").Handler(buildHandler)

	return r
}
