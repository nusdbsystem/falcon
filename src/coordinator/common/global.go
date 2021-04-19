package common

import (
	"coordinator/logger"
	"os"
)

const (
	// TODO: are these the names assigned for master and workers?
	Master          = "Master"
	TrainWorker     = "TrainWorker"
	InferenceWorker = "InferenceWorker"

	// master& worker heartbeat

	MasterTimeout = 10000 //  send heartbeat every 10 second
	WorkerTimeout = 20000 //  receive heartbeat within every 20 second

	// router path for coordServer
	Register                 = "register"
	SubmitJob                = "submit"
	StopJob                  = "stop"
	UpdateTrainJobMaster     = "update-train-master"
	UpdateInferenceJobMaster = "update-inference-master"
	UpdateJobResInfo         = "update-job-res"
	UpdateJobStatus          = "update-job-status"
	QueryJobStatus           = "query-job-status"

	InferenceStatusUpdate = "update-prediction-service-status"

	AssignPort         = "port-assign"
	AddPort            = "portadd"
	GetPartyServerPort = "port-get"

	PartyServerAdd    = "party-server-add"
	PartyServerDelete = "party-server-del"

	// router path for partyserver
	SetupWorker = "setup-worker" // RFC 5789

	// shared key of map
	PartyServerAddrKey = "psAddr"
	MasterAddrKey      = "masterAddr"
	PartyServerPortKey = "psPort"

	JobId      = "job_id"
	JobErrMsg  = "error_msg"
	JobResult  = "job_result"
	JobExtInfo = "ext_info"

	JobStatus        = "status"
	SubProcessNormal = "ok"

	JobFile = "job"

	TaskTypeKey = "task-type"

	TrainDataPath   = "train-data-path"
	TrainDataOutput = "train-data-output"
	ModelPath       = "model-path"

	// model endpoint
	ModelUpdate     = "model-update"
	IsTrained       = "is_trained"
	InferenceUpdate = "prediction-service-update"
	InferenceCreate = "prediction-service-create"

	JobName = "job_name"
	ExtInfo = "ext_info"

	Network = "tcp"

	// job status
	// TODO: replace int with msgs
	JobInit       = 0
	JobRunning    = 1
	JobSuccessful = 2
	JobFailed     = 3
	JobKilled     = 4

	WorkerYamlCreatePath = "./scripts/_create_runtime_worker.sh"
	MasterYamlCreatePath = "./scripts/_create_runtime_master.sh"

	YamlBasePath = "./deploy/template/"

	HorizontalFl = "horizontal"
	VerticalFl   = "vertical"

	// algorithms

	LogisticRegressionKey = "logistic_regression"
	RuntimeLogs           = "runtime_logs"
)

var (
	// For user defined variables, define them in userdefined.properties first,
	// and then, add to here

	// meta env vars
	Env         = ""
	ServiceName = ""

	// JobDB and Database Configs
	JobDatabase       = ""
	JobDbSqliteDb     = ""
	JobDbHost         = ""
	JobDbMysqlUser    = ""
	JobDbMysqlPwd     = ""
	JobDbMysqlDb      = ""
	JobDbMysqlOptions = ""

	// find the cluster port, call internally
	JobDbMysqlPort = ""

	// redis
	RedisHost = ""
	RedisPwd  = ""

	// find the cluster port, call internally
	RedisPort          = ""
	JobDbMysqlNodePort = ""
	RedisNodePort      = ""

	// sys port, here COORD_TARGET_PORT must equal to
	CoordIP   = ""
	CoordPort = ""

	// number of consumers used in coord http server
	NbConsumers = ""

	PartyServerIP   = ""
	PartyServerPort = ""
	PartyID         = ""

	LocalPath = ""

	PartyServerBasePath = ""

	// those are init by coordinator
	WorkerType = ""
	WorkerAddr = ""
	MasterAddr = ""

	// this is the worker's k8s service name, only used in production
	WorkerK8sSvcName = ""
	// enable other service access master with clusterIP+clusterPort, from inside the cluster
	CoordK8sSvcName = ""

	// for coord, node port is the same as cluster port, so all use coorport
	// this is service name + port
	CoordAddr = ""

	MasterQItem = ""

	// paths used in training
	TaskDataPath    = ""
	TaskDataOutput  = ""
	TaskModelPath   = ""
	TaskRuntimeLogs = ""

	// paths for MPC and FL Engine
	MpcExePath   = ""
	FLEnginePath = ""
)

// GetEnv get key environment variable if exist otherwise return defalutValue
func GetEnv(key, defaultValue string) string {
	value := os.Getenv(key)
	if len(value) == 0 {
		logger.Do.Printf("Set env var to default {%s: %s}\n", key, defaultValue)
		return defaultValue
	}
	logger.Do.Printf("Read user defined env var {%s: %s}\n", key, value)
	return value
}
