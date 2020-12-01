package config

import "os"

/**
 * @Author
 * @Description This file is only used inside the project,
				for any config required to modify according to env,
				use user_config.sh or bash_env.sh
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

	TrainTaskType = "train"
	PredictTaskType = "predict"

	DevEnv = "dev"
	ProdEnv = "prod"

	CoordinatorYaml = "./deploy/yamlfiles/"
	ListenerYaml = "./deploy/yamlfiles/"

	TrainYaml = "./deploy/yamlfiles/"
	PredictorYaml = "./deploy/yamlfiles/"

)

var (
	//////////////////////////////////////////////////////////////////////////
	// This is user defined variables, define them in user_config.sh first, //
	// and then, add to here												//
	//////////////////////////////////////////////////////////////////////////

	// MetaStore and Database Configs
	MsEngine       = os.Getenv("MS_ENGINE")
	MsSqliteDb     = os.Getenv("MS_SQLITE_DB")
	MsHost         = os.Getenv("MS_HOST")
	MsMysqlUser    = os.Getenv("MS_MYSQL_USER")
	MsMysqlPwd     = os.Getenv("MS_MYSQL_PWD")
	MsMysqlDb      = os.Getenv("MS_MYSQL_DB")
	MsMysqlOptions = os.Getenv("MS_MYSQL_OPTIONS")

	// sys port
	MasterPort   = os.Getenv("MasterPort")
	ListenerPort = os.Getenv("ListenerPort")

	// envs
	Env = os.Getenv("Env")

)
