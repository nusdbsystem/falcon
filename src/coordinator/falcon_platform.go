package main

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/coordserver"
	"coordinator/distributed"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"coordinator/partyserver"
	"fmt"
	"os"
	"runtime"
	"time"
)

func init() {

	// prority: env >  user provided > default value
	runtime.GOMAXPROCS(4)
	initLogger()
	InitEnvs(common.ServiceName)

}

func initLogger(){
	// this path is fixed, used to creating folder inside container
	var fixedPath string
	if common.Env == common.DevEnv{
		common.LocalPath = os.Getenv("DATA_BASE_PATH")
		fixedPath = common.LocalPath+"runtimeLogs"
	}else{
		fixedPath ="./logs"
	}

	fmt.Println("Loging to ", fixedPath)

	_ = os.Mkdir(fixedPath, os.ModePerm)
	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	var logFileName string
	logFileName = fixedPath + "/" + common.ServiceName + rawTime.Format(layout) + "logs"

	logger.Do, logger.F = logger.GetLogger(logFileName)
}


func getCoordUrl(url string) string{
		// using service name+ port to connect to coord
	    logger.Do.Printf("<<<<<<<<<<<<<<<<< Read envs, User defined,   key: CoordinatorUrl, value: %s >>>>>>>>>>>>>\n",url)
		return url
}


func InitEnvs(svcName string){

	if svcName=="coord"{
		// coord needs db information
		common.JobDbEngine       = common.GetEnv("JOB_DB_ENGINE", "sqlite3")
		common.JobDbSqliteDb     = common.GetEnv("JOB_DB_SQLITE_DB", "falcon")
		common.JobDbHost         = common.GetEnv("JOB_DB_HOST","localhost")
		common.JobDbMysqlUser    = common.GetEnv("JOB_DB_MYSQL_USER", "falcon")
		common.JobDbMysqlPwd     = common.GetEnv("JOB_DB_MYSQL_PWD", "falcon")
		common.JobDbMysqlDb      = common.GetEnv("JOB_DB_MYSQL_DB", "falcon")
		common.JobDbMysqlOptions = common.GetEnv("JOB_DB_MYSQL_OPTIONS", "?parseTime=true")
		common.JobDbMysqlPort    = common.GetEnv("MYSQL_CLUSTER_PORT", "30000")

		common.RedisHost      = common.GetEnv("REDIS_HOST","localhost")
		common.RedisPwd       = common.GetEnv("REDIS_PWD", "falcon")
		// coord needs redis information
		common.RedisPort       = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
		// find the cluster port, call internally
		common.JobDbMysqlNodePort    = common.GetEnv("MYSQL_NODE_PORT", "30001")
		common.RedisNodePort    = common.GetEnv("REDIS_NODE_PORT", "30003")

		// find the cluster port, call internally
		common.CoordIP = common.GetEnv("COORDINATOR_IP", "")
		common.CoordPort   = common.GetEnv("COORD_TARGET_PORT", "30004")

		common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")

		common.CoordinatorUrl = getCoordUrl(common.CoordIP + ":" + common.CoordPort)

		if len(common.ServiceName) == 0{
			logger.Do.Println("Error: Input Error, ServiceName not provided, is either 'coord' or 'partyserver' ")
			os.Exit(1)
		}

	}else if svcName=="partyserver"{

		// partyserver needs coord ip+port,lis port
		common.CoordIP = common.GetEnv("COORDINATOR_IP", "")
		common.CoordPort = common.GetEnv("COORD_TARGET_PORT", "30004")
		common.PartyServerIP = common.GetEnv("PARTY_SERVER_IP", "")
		common.PartyServeBasePath = common.GetEnv("DATA_BASE_PATH", "")

		// partyserver communicate coord with ip+port
		common.CoordinatorUrl = getCoordUrl(common.CoordIP + ":" + common.CoordPort)

		// run partyserver requires to get a new partyserver port
		common.PartyServerPort = common.GetEnv("PARTY_SERVER_NODE_PORT", "")

		common.PartyServerId = common.GetEnv("PARTY_SERVER_ID", "")

		if common.CoordIP=="" || common.PartyServerIP==""||common.PartyServerPort=="" {
			logger.Do.Println("Error: Input Error, either CoordIP or PartyServerIP or PartyServerPort not provided")
			os.Exit(1)
		}

	}else if svcName==common.Master {

		// master needs redis information
		common.RedisHost      = common.GetEnv("REDIS_HOST","localhost")
		common.RedisPwd       = common.GetEnv("REDIS_PWD", "falcon")
		common.RedisPort       = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
		common.RedisNodePort    = common.GetEnv("REDIS_NODE_PORT", "30003")
		common.CoordPort = common.GetEnv("COORD_TARGET_PORT", "30004")

		// master needs queue item, task type
		common.MasterQItem =common.GetEnv("ITEM_KEY", "")
		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.MasterUrl = common.GetEnv("MASTER_URL", "")

		common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")

		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")

		// master communicate coord with ip+port in dev, with name+port in prod
		if common.Env==common.DevEnv{

			logger.Do.Println("CoordIP: ", common.CoordIP  + ":" + common.CoordPort)

			common.CoordinatorUrl = getCoordUrl(common.CoordIP + ":" + common.CoordPort)

		}else if common.Env==common.ProdEnv{

			logger.Do.Println("CoordK8sSvcName: ", common.CoordK8sSvcName  + ":" + common.CoordPort)

			common.CoordinatorUrl = getCoordUrl(common.CoordK8sSvcName  + ":" + common.CoordPort)
		}


		if common.CoordinatorUrl==""{
			logger.Do.Println("Error: Input Error, CoordinatorUrl not provided")
			os.Exit(1)
		}

	}else if svcName==common.TrainWorker{
		// this will be executed only in production, in dev, the common.WorkerType==""

		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerUrl = common.GetEnv("WORKER_URL", "")
		common.MasterUrl = common.GetEnv("MASTER_URL", "")
		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")
		if common.MasterUrl=="" || common.WorkerUrl=="" {
			logger.Do.Println("Error: Input Error, either MasterUrl or WorkerUrl  not provided")
			os.Exit(1)
		}

	}else if svcName==common.PredictWorker{

		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		// this will be executed only in production, in dev, the common.WorkerType==""

		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerUrl = common.GetEnv("WORKER_URL", "")
		common.MasterUrl = common.GetEnv("MASTER_URL", "")
		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")
		if common.MasterUrl=="" || common.WorkerUrl=="" {
			logger.Do.Println("Error: Input Error, either MasterUrl or WorkerUrl not provided")
			os.Exit(1)
		}
	}
}


func main() {

	defer logger.HandleErrors()

	defer func(){
		_=logger.F.Close()
	}()


	if common.ServiceName == "coord" {
		logger.Do.Println("Launch falcon_platform, the common.ServiceName", common.ServiceName)

		coordserver.SetupHttp(3)
	}

	// start work in remote machine automatically
	if common.ServiceName == "partyserver" {

		logger.Do.Println("Launch falcon_platform, the common.ServiceName", common.ServiceName)

		partyserver.SetupPartyServer()
	}


	//////////////////////////////////////////////////////////////////////////
	//						 start tasks, called internally 				//
	// 																	    //
	//////////////////////////////////////////////////////////////////////////

	//those 2 is only called internally

	if common.ServiceName == common.Master {

		logger.Do.Println("Lunching falcon_platform, the common.WorkerType", common.WorkerType)

		// this should be the service name, defined at runtime,
		masterUrl := common.MasterUrl

		qItem := cache.Deserialize(cache.InitRedisClient().Get(common.MasterQItem))

		workerType := common.WorkerType

		distributed.SetupMaster(masterUrl, qItem, workerType)

	}

	if common.ServiceName == common.TrainWorker {

		logger.Do.Println("Lunching falcon_platform, the common.WorkerType", common.WorkerType)

		wk := worker.InitTrainWorker(common.MasterUrl, common.WorkerUrl)
		wk.RunWorker(wk)
	}

	if common.ServiceName == common.PredictWorker {

		logger.Do.Println("Lunching falcon_platform, the common.WorkerType", common.WorkerType)

		wk := worker.InitPredictWorker(common.MasterUrl, common.WorkerUrl)
		wk.RunWorker(wk)

	}
}
