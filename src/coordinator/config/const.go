package config

const (

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

	// sys point
	MasterPort   = "6573"
	ListenerPort = "6574"

	Proxy = "tcp"

	// job status
	JobInit       = 0
	JobRunning    = 1
	JobSuccessful = 2
	JobFailed     = 3
	JobKilled     = 4

	TrainTaskType = "train"
	PredictTaskType = "train"
	// MetaStore and Database Configs
	MS_ENGINE        = "sqlite3" // or mysql
	MS_HOST          = "localhost"
	MS_SQLITE_DB     = "./falcon.db"
	MS_MYSQL_USER    = "root"
	MS_MYSQL_PWD     = "rootuser"
	MS_MYSQL_DB      = "Test"
	MS_MYSQL_OPTIONS = "?parseTime=true"

)
