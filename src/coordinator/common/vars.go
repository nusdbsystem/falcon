package common

import (
	"fmt"
	"os"
)

/**
 * @Author
 * @Description This file is only used inside the project,
				for any common required to modify according to env,
				use coordinator.properties or bash_env.sh
 * @Date 4:42 下午 1/12/20
 * @Param
 * @return
 **/


const (
	Master = "Master"
	Worker = "Worker"
	ModelService = "ModelService"

	// master& wokrer heartbeat

	MasterTimeout = 10000 //  send hb every 10 second
	WorkerTimeout = 20000 //  receive hb within every 20 second

	// router path for api
	Register         = "register"
	SubmitJob        = "submit"
	StopJob          = "stop"
	UpdateJobMaster  = "update-master"
	UpdateJobResInfo = "update-job-res"
	UpdateJobStatus  = "update-job-status"
	QueryJobStatus   = "query-job-status"

	UpdateModelServiceStatus  = "update-prediction-status"

	ListenerAdd    = "listener-add"
	ListenerDelete = "listener-del"

	// router path for listener
	SetupWorker = "setup-worker" // RFC 5789

	// shared key of map
	ListenerAddr = "listenerAddress"
	MasterAddr   = "masterAddress"

	JobId      = "job_id"
	JobErrMsg  = "error_msg"
	JobResult  = "job_result"
	JobExtInfo = "ext_info"

	JobStatus        = "status"
	SubProcessNormal = "ok"

	DslFile = "dsl"

	PreProcessing = "pre_processing"
	ModelTraining = "model_training"

	TaskType = "task-type"


	// model endpoint
	ModelUpdate  = "model-update"
	IsTrained = "is_trained"
	SvcPublishing  = "model-publish"
	SvcCreate  = "model-create"

	AppName = "app_name"
	ExtInfo = "ext_info"

	Proxy = "tcp"

	// job status
	JobInit       = 0
	JobRunning    = 1
	JobSuccessful = 2
	JobFailed     = 3
	JobKilled     = 4

	TrainExecutor = "train"
	PredictExecutor = "predict"

	DevEnv = "dev"
	ProdEnv = "prod"

	WorkerYamlCreatePath = "./scripts/_create_worker.sh"
	MasterYamlCreatePath = "./scripts/_create_master.sh"

	YamlBasePath = "./deploy/template/"

)

var (
	//////////////////////////////////////////////////////////////////////////
	// This is user defined variables, define them in coordinator.properties first, //
	// and then, add to here												//
	//////////////////////////////////////////////////////////////////////////

	// MetaStore and Database Configs
	MsEngine       = GetEnv("MS_ENGINE", "sqlite3")
	MsSqliteDb     = GetEnv("MS_SQLITE_DB", "falcon")
	MsHost         = GetEnv("MS_HOST","localhost")
	MsMysqlUser    = GetEnv("MS_MYSQL_USER", "falcon")
	MsMysqlPwd     = GetEnv("MS_MYSQL_PWD", "falcon")
	MsMysqlDb      = GetEnv("MS_MYSQL_DB", "falcon")
	MsMysqlOptions = GetEnv("MS_MYSQL_OPTIONS", "?parseTime=true")

	// find the cluster port, call internally
	MsMysqlPort    = GetEnv("MYSQL_CLUSTER_PORT", "3306")

	// redis
	RedisHost      = GetEnv("REDIS_HOST","localhost")
	RedisPwd       = GetEnv("REDIS_PWD", "falcon")

	// find the cluster port, call internally
	RedisPort       = GetEnv("REDIS_CLUSTER_PORT", "6379")

	// sys port
	MasterPort   = GetEnv("MASTER_TARGET_PORT", "31201")
	ListenerPort = GetEnv("LISTENER_TARGET_PORT", "31301")

	// envs
	Env = GetEnv("Env",DevEnv)

	// those are init by user
	ServiceNameGlobal = GetEnv("SERVICE_NAME", "")
	CoordAddrGlobal = GetEnv("COORDINATOR_IP", "")
	ListenAddrGlobal = GetEnv("LISTENER_IP", "")

	// those are init by coordinator
	ExecutorTypeGlobal = GetEnv("EXECUTOR", "")
	WorkerURLGlobal = GetEnv("WORKER_URL", "")
	MasterURLGlobal = GetEnv("MASTER_URL", "")

	CoordURLGlobal = CoordAddrGlobal + ":" + MasterPort
	ListenURLGlobal = ListenAddrGlobal + ":" + ListenerPort

	MasterQItem = GetEnv("QItem", "")
	ISMASTER = GetEnv("ISMASTER", "false")
)

// GetEnv get key environment variable if exist otherwise return defalutValue
func GetEnv(key, defaultValue string) string {
	value := os.Getenv(key)
	if len(value) == 0 {
		fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, Set to default, key: %s, default: %s >>>>>>>>>>>>>\n",key, defaultValue)
		return defaultValue
	}
	fmt.Printf("<<<<<<<<<<<<<<<< Read envs, user defined, key: %s, value: %s >>>>>>>>>>>>>\n",key, value)
	return value
}

