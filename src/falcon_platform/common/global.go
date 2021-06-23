package common

import (
	"falcon_platform/logger"
	"os"
)

const (
	Master          = "Master"
	TrainWorker     = "TrainWorker"
	InferenceWorker = "InferenceWorker"

	// master& worker heartbeat
	MasterTimeout = 10000 //  send heartbeat every 10 second
	WorkerTimeout = 20000 //  receive heartbeat within every 20 second
)

const (
	// router path for coordServer
	SubmitTrainJob = "/api/submit-train-job"
	StopTrainJob           = "/api/stop-train-job"
	UpdateTrainJobMaster   = "/api/update-train-master"
	UpdateJobResInfo       = "/api/update-job-res"
	UpdateJobStatus        = "/api/update-job-status"
	QueryTrainJobStatus    = "/api/query-train-job-status"
	RetrieveTrainJobReport = "/api/get-evaluation-report-by-train-job-id"

	UpdateInferenceJobMaster = "/api/update-inference-master"
	InferenceStatusUpdate    = "/api/update-prediction-service-status"
	InferenceUpdate          = "/api/prediction-service-update"
	InferenceCreate          = "/api/prediction-service-create"
	ModelUpdate              = "/api/model-update"

	AssignPort         = "/api/port-assign"
	AddPort            = "/api/portadd"
	GetPartyServerPort = "/api/port-get"

	PartyServerAdd    = "/api/party-server-add"
	PartyServerDelete = "/api/party-server-del"

	// router path for partyserver
	SetupWorker = "/api/setup-worker"
)

const (
	// shared key of map
	PartyServerAddrKey = "psAddr"
	MasterAddrKey      = "masterAddr"
	PartyServerPortKey = "psPort"

	JobId      = "job_id"
	JobErrMsg  = "error_msg"
	JobResult  = "job_result"
	JobExtInfo = "ext_info"

	JobStatus = "status"

	TrainJobFileKey = "train-job-file"

	TaskTypeKey = "task-type"

	TrainDataPath   = "train-data-path"
	TrainDataOutput = "train-data-output"
	ModelPath       = "model-path"
	IsTrained       = "is_trained"

	JobName = "job_name"
	ExtInfo = "ext_info"

	Network = "tcp"
)

const (
	// job status
	JobInit    = "initializing"
	JobRunning = "running"
	// those 3 are job result
	JobSuccessful = "finished"
	JobFailed     = "failed"
	JobKilled     = "killed"

	// one job contains multi tasks, this is task status
	TaskRunning    = "task-running"
	TaskSuccessful = "task-finished"
	TaskFailed     = "task-failed"

	// for common.Env, which is control running in production or dev
	DevEnv = "local"
	K8sEnv = "k8s"

	// for DB engine names
	DBsqlite3 = "sqlite3"
	DBMySQL   = "mysql"

	WorkerYamlCreatePath = "./scripts/_create_runtime_worker.sh"
	MasterYamlCreatePath = "./scripts/_create_runtime_master.sh"

	YamlBasePath = "./deploy/template/"

	HorizontalFl = "horizontal"
	VerticalFl   = "vertical"

	// algorithms
	RuntimeLogs = "runtime_logs"

	// train sub tasks
	MpcSubTask                 = "mpc"
	PreProcSubTask             = "pre_processing"
	ModelTrainSubTask          = "model_training"
	RetrieveModelReportSubTask = "model_report_retrieve"

	ActiveParty  = "active"
	PassiveParty = "passive"
)

// algorithms names
const (
	LogisticRegressAlgName = "logistic_regression"
	DecisionTreeAlgName    = "decision_tree"
)

// Content type

const (
	JsonContentType      = "application/json"
	MultipartContentType = "multipart/form-data"
)

var (
	// For user defined variables, define them in userdefined.properties first,
	// and then, add to here

	// meta env vars
	Env         = ""
	ServiceName = ""
	LogPath     = ""

	// Coord user-defined variables
	CoordIP       = ""
	CoordPort     = ""
	CoordBasePath = ""
	// later concatenated by falcon_platform.go
	CoordAddr = ""
	// number of consumers used in coord http server
	NbConsumers = ""
	// enable other service access master with clusterIP+clusterPort, from inside the cluster
	CoordK8sSvcName = ""

	// PartyServer user-define variables
	PartyServerIP       = ""
	PartyServerPort     = ""
	PartyID             = ""
	PartyType           = ""
	PartyServerBasePath = ""

	// JobDB and Database Configs
	// for dev use of sqlite3
	JobDatabase   = ""
	JobDbSqliteDb = ""
	// for prod use of mysql
	JobDbHost         = ""
	JobDbMysqlUser    = ""
	JobDbMysqlPwd     = ""
	JobDbMysqlDb      = ""
	JobDbMysqlOptions = ""

	// find the cluster port, call internally
	JobDbMysqlPort     = ""
	JobDbMysqlNodePort = ""

	// redis
	RedisHost = ""
	RedisPwd  = ""
	// find the cluster port, call internally
	RedisPort     = ""
	RedisNodePort = ""

	// those are init by falcon_platform
	WorkerType = ""
	WorkerAddr = ""
	MasterAddr = ""

	// this is the worker's k8s service name, only used in production
	WorkerK8sSvcName = ""

	MasterDslObj = ""

	// paths used in training
	TaskDataPath    = ""
	TaskDataOutput  = ""
	TaskModelPath   = ""
	TaskRuntimeLogs = ""

	// paths for MPC and FL Engine
	MpcExePath   = ""
	FLEnginePath = ""

	// docker image names
	FalconPlatformImage = ""
	FalconExecutorImage = ""
)

// GetEnv get key environment variable if exist otherwise return defalutValue
func GetEnv(key, defaultValue string) string {
	value := os.Getenv(key)
	if len(value) == 0 {
		logger.Log.Printf("Set env var to default {%s: %s}\n", key, defaultValue)
		return defaultValue
	}
	logger.Log.Printf("Read user defined env var {%s: %s}\n", key, value)
	return value
}
