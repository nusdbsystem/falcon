package common

import (
	"coordinator/logger"
	"fmt"
	"os"
)

/**
 * @Author
 * @Description This file is only used inside the project,
				for any common required to modify according to env,
				use userdefined.properties or bash_env.sh
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

	AssignPort  = "port-assign"
	AddPort  = "portadd"
	GetListenerPort  = "port-get"

	ListenerAdd    = "listener-add"
	ListenerDelete = "listener-del"

	// router path for listener
	SetupWorker = "setup-worker" // RFC 5789

	// shared key of map
	ListenerAddr = "listenerAddress"
	MasterAddr   = "masterAddress"
	ListenerPortKey = "listenerPort"

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

	TrainDataPath  = "train-data-path"
	TrainDataOutput  = "train-data-output"
	ModelPath = "model-path"
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

	MasterExecutor = "master"
	TrainExecutor = "train"
	PredictExecutor = "predict"

	DevEnv = "dev"
	ProdEnv = "prod"

	WorkerYamlCreatePath = "./scripts/_create_runtime_worker.sh"
	MasterYamlCreatePath = "./scripts/_create_runtime_master.sh"

	YamlBasePath = "./deploy/template/"


	MpcExe = "./semi-party.x"
	FalconTrainExe = "./falcon"

)

var (
	//////////////////////////////////////////////////////////////////////////
	// This is user defined variables, define them in userdefined.properties first, //
	// and then, add to here												//
	//////////////////////////////////////////////////////////////////////////

	// MetaStore and Database Configs
	MsEngine       = ""
	MsSqliteDb     = ""
	MsHost         = ""
	MsMysqlUser    = ""
	MsMysqlPwd     = ""
	MsMysqlDb      = ""
	MsMysqlOptions = ""

	// find the cluster port, call internally
	MsMysqlPort    = ""

	// redis
	RedisHost      = ""
	RedisPwd       = ""

	// find the cluster port, call internally
	RedisPort       = ""

	MsMysqlNodePort = ""
	RedisNodePort    = ""

	// sys port, here COORD_TARGET_PORT must equal to
	CoordPort   = ""
	ListenerPort = ""
	ListenerId = ""
	// envs
	Env = getEnv("Env", DevEnv)

	// those are init by user
	ServiceNameGlobal = getEnv("SERVICE_NAME", "coord")
	CoordAddrGlobal = ""
	ListenAddrGlobal = ""
	ListenBasePath = ""
	// those are init by coordinator
	ExecutorTypeGlobal = ""
	WorkerURLGlobal = ""
	MasterURLGlobal = ""

	ExecutorCurrentName = ""
	// this is ip + port
	ListenURLGlobal = ""

	// enable other service access master with clusterIp+clusterPort, from inside the cluster
	CoordSvcName = ""

	// for coord, node port is the same as cluster port, so all use coorport
	// this is service name + port
	CoordSvcURLGlobal = ""

	MasterQItem = ""

	TaskDataPath = ""
	TaskDataOutput = ""
	TaskModelPath = ""
	TaskRuntimeLogs = ""
)

// GetEnv get key environment variable if exist otherwise return defalutValue
func GetEnv(key, defaultValue string) string {
	/**
	 * @Author
	 * @Description init the runtime env,
	 * @Date 1:33 下午 9/12/20
	 * @Param
	 * @return
	 **/
	value := os.Getenv(key)
	if len(value) == 0 {
		logger.Do.Printf("<<<<<<<<<<<<<<<<< Read envs, Set to default, key: %s, default: %s >>>>>>>>>>>>>\n",key, defaultValue)
		return defaultValue
	}
	logger.Do.Printf("<<<<<<<<<<<<<<<<< Read envs, User defined,   key: %s, value: %s >>>>>>>>>>>>>\n",key, value)
	return value
}


func getEnv(key, defaultValue string) string {
	/**
	 * @Author
	 * @Description init the base env, for env and serviceName
	 * @Date 1:32 下午 9/12/20
	 * @Param
	 * @return
	 **/
	value := os.Getenv(key)
	if len(value) == 0 {
		fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, Set to default, key: %s, default: %s >>>>>>>>>>>>>\n",key, defaultValue)
		return defaultValue
	}
	fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, User defined,   key: %s, value: %s >>>>>>>>>>>>>\n",key, value)
	return value
}

