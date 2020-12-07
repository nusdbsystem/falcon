package common

import (
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

	WorkerYamlCreatePath = "./scripts/_create_runtime_worker.sh"
	MasterYamlCreatePath = "./scripts/_create_runtime_master.sh"

	YamlBasePath = "./deploy/template/"

)

var (
	//////////////////////////////////////////////////////////////////////////
	// This is user defined variables, define them in userdefined.properties first, //
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
	MsMysqlPort    = GetEnv("MYSQL_CLUSTER_PORT", "30000")

	// redis
	RedisHost      = GetEnv("REDIS_HOST","localhost")
	RedisPwd       = GetEnv("REDIS_PWD", "falcon")

	// find the cluster port, call internally
	RedisPort       = GetEnv("REDIS_CLUSTER_PORT", "30002")

	MsMysqlNodePort    = GetEnv("MYSQL_NODE_PORT", "30001")
	RedisNodePort    = GetEnv("REDIS_NODE_PORT", "30003")

	// sys port, here COORD_TARGET_PORT must equal to
	CoordPort   = GetEnv("COORD_TARGET_PORT", "30004")
	ListenerPort = GetEnv("LISTENER_TARGET_PORT", "30005")

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

	// this is ip + port
	CoordURLGlobal = CoordAddrGlobal + ":" + CoordPort
	ListenURLGlobal = ListenAddrGlobal + ":" + ListenerPort

	// enable other service access master with clusterIp+clusterPort, from inside the cluster
	CoordSvcName = GetEnv("CoordSvcName", "")

	// for coord, node port is the same as cluster port, so all use coorport
	// this is service name + port
	CoordSvcURLGlobal = getCoordUrl()

	MasterQItem = GetEnv("QItem", "")
	ISMASTER = GetEnv("ISMASTER", "false")

	//DATA_BASE_PATH = GetEnv("DATA_BASE_PATH", "./")
)

// GetEnv get key environment variable if exist otherwise return defalutValue
func GetEnv(key, defaultValue string) string {
	value := os.Getenv(key)
	if len(value) == 0 {
		fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, Set to default, key: %s, default: %s >>>>>>>>>>>>>\n",key, defaultValue)
		return defaultValue
	}
	fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, User defined,   key: %s, value: %s >>>>>>>>>>>>>\n",key, value)
	return value
}


func getCoordUrl() string{
	if Env==ProdEnv || CoordSvcName!=""{
		// using service name+ port to connect to coord
		fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, User defined,   key: CoordSvcURLGlobal, value: %s >>>>>>>>>>>>>\n", CoordSvcName + ":" + CoordPort)
		return CoordSvcName + ":" + CoordPort
	}
	if Env==DevEnv || CoordSvcName==""{
		fmt.Printf("<<<<<<<<<<<<<<<<< Read envs, User defined,   key: CoordSvcURLGlobal, value: %s >>>>>>>>>>>>>\n", CoordURLGlobal)
		// using  ip+ port to connect to coord
		return CoordURLGlobal
	}
	panic("No CoordSvcURLGlobal assigned, master may not connect to coordinator")
}

