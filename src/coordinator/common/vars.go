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
	ListenerAddr = "listenerAddr"
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

	MasterTaskType = "master"
	TrainTaskType = "train"
	PredictTaskType = "predict"

	DevEnv = "dev"
	ProdEnv = "prod"

	CoordinatorYaml = "./deploy/template/"
	ListenerYaml = "./deploy/template/"

	TrainYaml = "./deploy/template/"
	PredictorYaml = "./deploy/template/"

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

	// sys port
	MasterPort   = GetEnv("MASTER_TARGET_PORT", "31201")
	ListenerPort = GetEnv("LISTENER_TARGET_PORT", "31301")

	// envs
	Env = GetEnv("Env",DevEnv)
)

// GetEnv get key environment variable if exist otherwise return defalutValue
func GetEnv(key, defaultValue string) string {
	value := os.Getenv(key)
	if len(value) == 0 {
		fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, set to default, key: %s, default: %s >>>>>>>>>>>>>\n",key, defaultValue)
		return defaultValue
	}
	fmt.Printf("<<<<<<<<<<<<<<<< Read envs,key: %s, value: %s >>>>>>>>>>>>>\n",key, value)
	return value
}