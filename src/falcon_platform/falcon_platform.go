package main

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/coordserver"
	"falcon_platform/jobmanager"
	"falcon_platform/jobmanager/worker"
	"falcon_platform/logger"
	"falcon_platform/partyserver"
	"falcon_platform/resourcemanager"
	"fmt"
	"log"
	"os"
	"runtime"
	"strconv"
	"time"
)

func init() {

	// priority: env >  user provided > default value
	runtime.GOMAXPROCS(4)

	// before init the envs, load the meta envs, in which
	// 1. Env will indicate which deploy model to use, prod devDocker or dev
	// 2. ServiceName will indicate which service to run
	// 3. LogPath will indicate where the log will be stored
	// use os to get the env, since initLogger is before initEnv
	common.Env = os.Getenv("ENV")
	common.ServiceName = os.Getenv("SERVICE_NAME")
	common.LogPath = os.Getenv("LOG_PATH")

	fmt.Println("Currently, the Env model is: ", common.Env)
	fmt.Println("Currently, the ServiceName is: ", common.ServiceName)
	fmt.Println("Currently, the LogPath is: ", common.LogPath)

	initLogger()

	// according to the running service (coord / partyserver/ master / worker) and deploy model(dev / prod / devDocker)
	// this will assign value to some global variables
	initEnv(common.ServiceName)

}

func initLogger() {
	var runtimeLogPath string

	// in dev, we have a logPath to store everything,
	// but in production, the coordinator and party server are
	// separated at different machines or clusters, we use docker,
	if common.Env == common.DevEnv {
		runtimeLogPath = common.LogPath
	} else if common.Env == common.K8sEnv {
		// the log is fixed to ./log, which is the path inside the docker
		runtimeLogPath = "./logs"
	}

	fmt.Println("[initLogger] runtimeLogPath:", runtimeLogPath)

	// create nested dirs if necessary
	err := os.MkdirAll(runtimeLogPath, os.ModePerm)
	if err != nil {
		log.Fatal(err)
	}

	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	var logFileName string
	logFileName = runtimeLogPath + "/" + common.ServiceName + "-" + rawTime.Format(layout) + ".log"

	logger.Log, logger.LogFile = logger.GetLogger(logFileName)
}

func initEnv(svcName string) {

	common.FalconExecutorImage = common.GetEnv(
		"FALCON_WORKER_IMAGE",
		"nailidocker/falcon_executor:latest")

	switch svcName {
	case "coord":
		// find the cluster port, call internally
		common.CoordIP = common.GetEnv("COORD_SERVER_IP", "")
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")
		common.CoordAddr = common.CoordIP + ":" + common.CoordPort
		common.CoordBasePath = common.GetEnv("COORD_SERVER_BASEPATH", "./dev_test")
		// coord http server number of consumers
		common.NbConsumers = common.GetEnv("N_CONSUMER", "3")
		// get the env for Job DB in Coord server
		common.JobDatabase = common.GetEnv("JOB_DATABASE", "sqlite3")
		// get the env needed for different db type
		if common.JobDatabase == common.DBsqlite3 {
			common.JobDbSqliteDb = common.GetEnv("JOB_DB_SQLITE_DB", "falcon.db")
		} else if common.JobDatabase == common.DBMySQL {
			common.JobDbHost = common.GetEnv("JOB_DB_HOST", "localhost")
			common.JobDbMysqlUser = common.GetEnv("JOB_DB_MYSQL_USER", "falcon")
			common.JobDbMysqlPwd = common.GetEnv("JOB_DB_MYSQL_PWD", "falcon")
			common.JobDbMysqlDb = common.GetEnv("JOB_DB_MYSQL_DB", "falcon")
			common.JobDbMysqlOptions = common.GetEnv("JOB_DB_MYSQL_OPTIONS", "?parseTime=true")
			common.JobDbMysqlPort = common.GetEnv("MYSQL_CLUSTER_PORT", "3306")
		}
		// env for prod
		if common.Env == common.K8sEnv {
			// find the cluster port, call internally

			common.JobDbMysqlNodePort = common.GetEnv("MYSQL_NODE_PORT", "30001")

			common.RedisHost = common.GetEnv("REDIS_HOST", "localhost")
			common.RedisPwd = common.GetEnv("REDIS_PWD", "falcon")
			// coord needs redis information
			common.RedisPort = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
			common.RedisNodePort = common.GetEnv("REDIS_NODE_PORT", "30003")
			common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")
		}
		if len(common.ServiceName) == 0 {
			logger.Log.Println("Error: Input Error, ServiceName not provided, is either 'coord' or 'partyserver' ")
			os.Exit(1)
		}
	case "partyserver":
		// party server needs coord IP+port,lis port
		common.CoordIP = common.GetEnv("COORD_SERVER_IP", "")
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")
		common.PartyServerIP = common.GetEnv("PARTY_SERVER_IP", "")
		// party server communicate coord with IP+port
		common.CoordAddr = common.CoordIP + ":" + common.CoordPort
		// run partyserver requires to get a new partyserver port
		common.PartyServerPort = common.GetEnv("PARTY_SERVER_NODE_PORT", "")
		common.PartyID = common.GetEnv("PARTY_ID", "")
		common.PartyServerBasePath = common.GetEnv("PARTY_SERVER_BASEPATH", "./dev_test")
		// get the MPC exe path
		common.MpcExePath = common.GetEnv(
			"MPC_EXE_PATH",
			"/opt/falcon/third_party/MP-SPDZ/semi-party.x")
		// get the FL engine exe path
		common.FLEnginePath = common.GetEnv(
			"FL_ENGINE_PATH",
			"/opt/falcon/build/src/executor/falcon")
		if common.CoordIP == "" || common.PartyServerIP == "" || common.PartyServerPort == "" {
			logger.Log.Println("Error: Input Error, either CoordIP or PartyServerIP or PartyServerPort not provided")
			os.Exit(1)
		}
	case common.Master:
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")

		// master needs dslqueue item, task type
		common.MasterDslObj = common.GetEnv("ITEM_KEY", "")
		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")

		if common.Env == common.DevEnv {
			// master communicate coord with IP+port in dev

			logger.Log.Println("CoordUrl: ", common.CoordIP+":"+common.CoordPort)
			common.CoordAddr = common.CoordIP + ":" + common.CoordPort

		} else if common.Env == common.K8sEnv {

			// master needs redis information
			common.RedisHost = common.GetEnv("REDIS_HOST", "localhost")
			common.RedisPwd = common.GetEnv("REDIS_PWD", "falcon")
			common.RedisPort = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
			common.RedisNodePort = common.GetEnv("REDIS_NODE_PORT", "30003")

			// prod using k8
			common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")
			common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")

			// master communicate coord with IP+port in dev, with name+port in prod
			logger.Log.Println("CoordK8sSvcName: ", common.CoordK8sSvcName+":"+common.CoordPort)

			common.CoordAddr = common.CoordK8sSvcName + ":" + common.CoordPort
		}

		if common.CoordAddr == "" {
			logger.Log.Println("Error: Input Error, CoordAddr not provided")
			os.Exit(1)
		}
	case common.TrainWorker:
		// this will be executed only in production, in dev, the common.WorkerType==""

		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerAddr = common.GetEnv("WORKER_ADDR", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")
		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")

		// get the MPC exe path
		common.MpcExePath = common.GetEnv(
			"MPC_EXE_PATH",
			"/opt/falcon/third_party/MP-SPDZ/semi-party.x")
		// get the FL engine exe path
		common.FLEnginePath = common.GetEnv(
			"FL_ENGINE_PATH",
			"/opt/falcon/build/src/executor/falcon")

		if common.MasterAddr == "" || common.WorkerAddr == "" {
			logger.Log.Println("Error: Input Error, either MasterAddr or WorkerAddr  not provided")
			os.Exit(1)
		}
	case common.InferenceWorker:
		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		// this will be executed only in production, in dev, the common.WorkerType==""

		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerAddr = common.GetEnv("WORKER_ADDR", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")
		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")

		// get the MPC exe path
		common.MpcExePath = common.GetEnv(
			"MPC_EXE_PATH",
			"/opt/falcon/third_party/MP-SPDZ/semi-party.x")
		// get the FL engine exe path
		common.FLEnginePath = common.GetEnv(
			"FL_ENGINE_PATH",
			"/opt/falcon/build/src/executor/falcon")

		if common.MasterAddr == "" || common.WorkerAddr == "" {
			logger.Log.Println("Error: Input Error, either MasterAddr or WorkerAddr not provided")
			os.Exit(1)
		}
	}
}

func main() {

	// For convenience,
	// this is the main common entry of 4 services,
	// use different environment to select which service to run

	// 1. ServiceName=coord to run coord server
	// 2. ServiceName=partyserver to run party server
	// 3. ServiceName=Master to run Master server
	// 4. ServiceName=TrainWorker to run TrainWorker server
	// 5. ServiceName=InferenceWorker to run InferenceWorker server

	defer logger.HandleErrors()

	defer func() {
		_ = logger.LogFile.Close()
	}()

	switch common.ServiceName {
	case "coord":
		logger.Log.Println("Launch falcon_platform, the common.ServiceName", common.ServiceName)
		nConsumer, _ := strconv.Atoi(common.NbConsumers)

		coordserver.SetupCoordServer(nConsumer)
	case "partyserver":
		// start work in remote machine automatically
		logger.Log.Println("Launch falcon_platform, the common.ServiceName", common.ServiceName)
		partyserver.SetupPartyServer()

	//////////////////////////////////////////////////////////////////////////
	//			start tasks, called internally 	, and in production      	//
	// 	      master and worker are isolate process, this is the entry      //
	//////////////////////////////////////////////////////////////////////////

	//those 3 is only called internally
	case common.Master:
		logger.Log.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)
		// this should be the service name, defined at runtime,
		masterAddr := common.MasterAddr
		dslOjb := cache.Deserialize(cache.InitRedisClient().Get(common.MasterDslObj))
		workerType := common.WorkerType
		jobmanager.SetupMaster(masterAddr, dslOjb, workerType)
		// kill the related service after finish training or prediction.
		km := resourcemanager.InitK8sManager(true, "")
		km.DeleteService(common.WorkerK8sSvcName)

	case common.TrainWorker:
		logger.Log.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)
		// init the train worker with addresses of master and worker, also the partyID
		// this is the entry of prod model, in prod, only use NativeProcessMngr right now
		wk := worker.InitTrainWorker(common.MasterAddr, common.WorkerAddr, common.PartyID)
		wk.RunWorker(wk)
		// once  worker is killed by master, clear the resources.
		km := resourcemanager.InitK8sManager(true, "")
		km.DeleteService(common.WorkerK8sSvcName)

	case common.InferenceWorker:
		logger.Log.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)
		// init the train worker with addresses of master and worker, also the partyID
		// this is the entry of prod model, in prod, only use NativeProcessMngr right now
		wk := worker.InitInferenceWorker(common.MasterAddr, common.WorkerAddr, common.PartyID)
		wk.RunWorker(wk)
	}

	// once worker is killed by master, clear the resources.
	km := resourcemanager.InitK8sManager(true, "")
	km.DeleteService(common.WorkerK8sSvcName)
}
