package router

import (
	"falcon_platform/common"
	"falcon_platform/coordserver/middleware"
	"falcon_platform/coordserver/models"
	"net/http"

	"github.com/gorilla/mux"
)

func NewRouter(JobDB *models.JobDB) *mux.Router {
	r := mux.NewRouter()

	middleware.SysLvPath = []string{common.PartyServerAdd}

	// match html views to routes
	// r.HandleFunc("/", HtmlIndex).Methods("GET")
	r.HandleFunc("/submit-train-job", HtmlSubmitTrainJob).Methods("GET")
	r.HandleFunc("/upload-train-job-file", HtmlUploadTrainJobFile).Methods("GET")

	// REST APIs
	//job
	r.HandleFunc(common.SubmitTrainJob, middleware.AddRouter(SubmitTrainJob, JobDB)).Methods("GET", "POST")
	r.HandleFunc(common.StopTrainJob+"/{jobId}", middleware.AddRouter(JobKill, JobDB)).Methods("GET")
	r.HandleFunc(common.UpdateTrainJobMaster, middleware.AddRouter(JobUpdateMaster, JobDB)).Methods("POST")
	r.HandleFunc(common.UpdateJobStatus, middleware.AddRouter(JobUpdateStatus, JobDB)).Methods("POST")
	r.HandleFunc(common.UpdateJobResInfo, middleware.AddRouter(JobUpdateResInfo, JobDB)).Methods("POST")
	r.HandleFunc(common.QueryTrainJobStatus+"/{jobId}", middleware.AddRouter(JobStatusQuery, JobDB)).Methods("GET")
	r.HandleFunc(common.RetrieveTrainJobReport+"/{jobId}", middleware.AddRouter(JobTrainingReportRetrieve, JobDB)).Methods("GET")

	//party server
	r.HandleFunc(common.PartyServerAdd, middleware.AddRouter(PartyServerAdd, JobDB)).Methods("POST")
	r.HandleFunc(common.PartyServerDelete, middleware.AddRouter(PartyServerDelete, JobDB)).Methods("POST")
	r.HandleFunc(common.GetPartyServerPort, middleware.AddRouter(GetPartyServerPort, JobDB)).Methods("GET")

	// model serving
	r.HandleFunc(common.ModelUpdate, middleware.AddRouter(ModelUpdate, JobDB)).Methods("POST")

	// prediction service
	r.HandleFunc(common.UpdateInferenceJobMaster, middleware.AddRouter(InferenceUpdateMaster, JobDB)).Methods("POST")
	r.HandleFunc(common.InferenceUpdate, middleware.AddRouter(UpdateInference, JobDB)).Methods("POST")
	r.HandleFunc(common.InferenceCreate, middleware.AddRouter(CreateInference, JobDB)).Methods("POST")
	r.HandleFunc(common.InferenceStatusUpdate, middleware.AddRouter(UpdateInferenceStatus, JobDB)).Methods("POST")

	// resource
	r.HandleFunc(common.AssignPort, middleware.AddRouter(AssignPort, JobDB)).Methods("GET")
	r.HandleFunc(common.AddPort, middleware.AddRouter(AddPort, JobDB)).Methods("POST")

	// static files
	// react build static htmls
	buildHandler := http.FileServer(http.Dir("web/build"))
	r.PathPrefix("/").Handler(buildHandler)

	return r
}
