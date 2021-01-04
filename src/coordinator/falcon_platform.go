package main

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/coordserver"
	"coordinator/distributed"
	"coordinator/distributed/taskmanager"
	"coordinator/distributed/worker"
	"coordinator/logger"
	"coordinator/partyserver"
	"fmt"
	"os"
	"path"
	"runtime"
	"time"
)

func init() {

	// prority: env >  user provided > default value
	runtime.GOMAXPROCS(4)
	initLogger()
	InitEnvs(common.ServiceName)

}

func initLogger() {
	var runtimeLogPath string
	if common.Env == common.DevEnv {
		common.LocalPath = os.Getenv("BASE_PATH")
		runtimeLogPath = path.Join(common.LocalPath, common.RuntimeLogs)
	} else {
		runtimeLogPath = "./logs"
	}

	fmt.Println("common.RuntimeLogs at: ", runtimeLogPath)

	_ = os.Mkdir(runtimeLogPath, os.ModePerm)
	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	var logFileName string
	logFileName = runtimeLogPath + "/" + common.ServiceName + rawTime.Format(layout) + ".logs"

	logger.Do, logger.F = logger.GetLogger(logFileName)
}

func getCoordAddr(addr string) string {
	// using service name+ port to connect to coord
	logger.Do.Printf("Read envs, User defined,   key: CoordAddr, value: %s\n", addr)
	return addr
}

func InitEnvs(svcName string) {

	if svcName == "coord" {
		// coord needs db information
		common.JobDatabase = common.GetEnv("JOB_DATABASE", "sqlite3")
		common.JobDbSqliteDb = common.GetEnv("JOB_DB_SQLITE_DB", "falcon.db")
		common.JobDbHost = common.GetEnv("JOB_DB_HOST", "localhost")
		common.JobDbMysqlUser = common.GetEnv("JOB_DB_MYSQL_USER", "falcon")
		common.JobDbMysqlPwd = common.GetEnv("JOB_DB_MYSQL_PWD", "falcon")
		common.JobDbMysqlDb = common.GetEnv("JOB_DB_MYSQL_DB", "falcon")
		common.JobDbMysqlOptions = common.GetEnv("JOB_DB_MYSQL_OPTIONS", "?parseTime=true")
		common.JobDbMysqlPort = common.GetEnv("MYSQL_CLUSTER_PORT", "30000")

		common.RedisHost = common.GetEnv("REDIS_HOST", "localhost")
		common.RedisPwd = common.GetEnv("REDIS_PWD", "falcon")
		// coord needs redis information
		common.RedisPort = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
		// find the cluster port, call internally
		common.JobDbMysqlNodePort = common.GetEnv("MYSQL_NODE_PORT", "30001")
		common.RedisNodePort = common.GetEnv("REDIS_NODE_PORT", "30003")

		// find the cluster port, call internally
		common.CoordIP = common.GetEnv("COORD_SERVER_IP", "")
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")

		common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")

		common.CoordAddr = getCoordAddr(common.CoordIP + ":" + common.CoordPort)

		if len(common.ServiceName) == 0 {
			logger.Do.Println("Error: Input Error, ServiceName not provided, is either 'coord' or 'partyserver' ")
			os.Exit(1)
		}

	} else if svcName == "partyserver" {

		// partyserver needs coord ip+port,lis port
		common.CoordIP = common.GetEnv("COORD_SERVER_IP", "")
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")
		common.PartyServerIP = common.GetEnv("PARTY_SERVER_IP", "")
		common.PartyServerBasePath = common.GetEnv("BASE_PATH", "")

		// partyserver communicate coord with ip+port
		common.CoordAddr = getCoordAddr(common.CoordIP + ":" + common.CoordPort)

		// run partyserver requires to get a new partyserver port
		common.PartyServerPort = common.GetEnv("PARTY_SERVER_NODE_PORT", "")

		common.PartyServerId = common.GetEnv("PARTY_SERVER_ID", "")

		if common.CoordIP == "" || common.PartyServerIP == "" || common.PartyServerPort == "" {
			logger.Do.Println("Error: Input Error, either CoordIP or PartyServerIP or PartyServerPort not provided")
			os.Exit(1)
		}

	} else if svcName == common.Master {

		// master needs redis information
		common.RedisHost = common.GetEnv("REDIS_HOST", "localhost")
		common.RedisPwd = common.GetEnv("REDIS_PWD", "falcon")
		common.RedisPort = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
		common.RedisNodePort = common.GetEnv("REDIS_NODE_PORT", "30003")
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")

		// master needs queue item, task type
		common.MasterQItem = common.GetEnv("ITEM_KEY", "")
		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")

		common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")

		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")

		// master communicate coord with ip+port in dev, with name+port in prod
		if common.Env == common.DevEnv {

			logger.Do.Println("CoordIP: ", common.CoordIP+":"+common.CoordPort)

			common.CoordAddr = getCoordAddr(common.CoordIP + ":" + common.CoordPort)

		} else if common.Env == common.ProdEnv {

			logger.Do.Println("CoordK8sSvcName: ", common.CoordK8sSvcName+":"+common.CoordPort)

			common.CoordAddr = getCoordAddr(common.CoordK8sSvcName + ":" + common.CoordPort)
		}

		if common.CoordAddr == "" {
			logger.Do.Println("Error: Input Error, CoordAddr not provided")
			os.Exit(1)
		}

	} else if svcName == common.TrainWorker {
		// this will be executed only in production, in dev, the common.WorkerType==""

		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerAddr = common.GetEnv("WORKER_ADDR", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")
		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")
		if common.MasterAddr == "" || common.WorkerAddr == "" {
			logger.Do.Println("Error: Input Error, either MasterAddr or WorkerAddr  not provided")
			os.Exit(1)
		}

	} else if svcName == common.InferenceWorker {

		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		// this will be executed only in production, in dev, the common.WorkerType==""

		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerAddr = common.GetEnv("WORKER_ADDR", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")
		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")
		if common.MasterAddr == "" || common.WorkerAddr == "" {
			logger.Do.Println("Error: Input Error, either MasterAddr or WorkerAddr not provided")
			os.Exit(1)
		}
	}
}

func main() {

	defer logger.HandleErrors()

	defer func() {
		_ = logger.F.Close()
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

		logger.Do.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)

		// this should be the service name, defined at runtime,
		masterAddr := common.MasterAddr

		qItem := cache.Deserialize(cache.InitRedisClient().Get(common.MasterQItem))

		workerType := common.WorkerType

		distributed.SetupMaster(masterAddr, qItem, workerType)

		km := taskmanager.InitK8sManager(true, "")
		km.DeleteService(common.WorkerK8sSvcName)
	}

	if common.ServiceName == common.TrainWorker {

		logger.Do.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)

		wk := worker.InitTrainWorker(common.MasterAddr, common.WorkerAddr)
		wk.RunWorker(wk)

		// once  worker is killed, clear the resources.
		km := taskmanager.InitK8sManager(true, "")
		km.DeleteService(common.WorkerK8sSvcName)

	}

	if common.ServiceName == common.InferenceWorker {

		logger.Do.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)

		wk := worker.InitInferenceWorker(common.MasterAddr, common.WorkerAddr)
		wk.RunWorker(wk)
	}
	// once  worker is killed, clear the resources.
	km := taskmanager.InitK8sManager(true, "")
	km.DeleteService(common.WorkerK8sSvcName)
}
