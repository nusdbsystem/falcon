package main

import (
	"falcon_platform/common"
	"falcon_platform/coordserver"
	"falcon_platform/jobmanager/worker"
	"falcon_platform/logger"
	"falcon_platform/partyserver"
	"falcon_platform/resourcemanager"
	"fmt"
	"math/rand"
	"os"
	"runtime"
	"strconv"
	"strings"
	"time"
)

//	logPath (outside, LogPath):  …/falcon_logs or ./logs  store logs
//	TaskRunTimeLogs(inner) = logPath /runtimeLogs/serviceName
//	logFile (inner)= TaskRunTimeLogs-algorithmName
//	workerLogFile(inner) = logFile /worker

func init() {

	// handle error firstly
	defer logger.HandleErrors()

	// priority: env >  user provided > default value
	runtime.GOMAXPROCS(4)

	// before init the envs, load the meta envs, in which
	// 1. Deployment will indicate which deploy method to use, localthread, docker or k8s
	// 2. ServiceName will indicate which service to run, coordServer, partyServer, master or worker
	// 3. LogPath will indicate where the log will be stored
	// use os to get the env, since initLogger is before initEnv
	common.Deployment = os.Getenv("ENV")
	if len(common.Deployment) == 0 {
		// default to thread
		common.Deployment = common.LocalThread
	}
	common.ServiceName = os.Getenv("SERVICE_NAME")
	common.LogPath = os.Getenv("LOG_PATH")

	// default
	if common.ServiceName == "" {
		common.ServiceName = "coord"
	}

	if common.Deployment == "" {
		common.Deployment = "subprocess"
	}

	if common.LogPath == "" {
		common.LogPath = "/Users/kevin/project_golang/src/github.com/falcon/src/falcon_platform/falcon_logs/coord_debug"
	}

	fmt.Println("Currently, the Deployment method is: ", common.Deployment)
	fmt.Println("Currently, the ServiceName is: ", common.ServiceName)
	fmt.Println("Currently, the LogPath is: ", common.LogPath)

	initLogger()

	// according to the running service (coord / partyserver/ master / worker) and deploy model(thread / k8s / Docker)
	// this will assign value to some global variables
	initEnv(common.ServiceName)
}

func initLogger() {

	//if common.Deployment == common.Docker {
	//	var absLogPath []rune
	//	absLogPath = []rune(common.LogPath)
	//	if absLogPath[0] == '.' {
	//		absLogPath = absLogPath[1:]
	//	}
	//	common.LogPath = string(absLogPath)
	//}

	// create nested dirs
	err := os.MkdirAll(common.LogPath, os.ModePerm)
	if err != nil {
		fmt.Println("[initLogger] runtimeLogPath Error", err)
		//time.Sleep(5 * time.Minute)
	}

	// Use layout string for time format.
	const layout = "2006-01-02T15:04:05"
	// Place now in the string.
	rawTime := time.Now()

	var logFileName string
	rand.Seed(time.Now().UnixNano())
	logFileName = common.LogPath + "/" + common.ServiceName + "-" + rawTime.Format(layout) + "-" + fmt.Sprintf("%d", rand.Intn(90000)) + ".log"
	fmt.Println("Logs will be written to ", logFileName)

	logger.Log, logger.LogFile = logger.GetLogger(logFileName)
}

func initEnv(svcName string) {

	common.IsDebug = common.GetEnv("IS_DEBUG", "debug-off")
	common.TestImgName = common.GetEnv(
		"TEST_IMG_NAME",
		"redis:5.0.3-alpine3.8")

	common.FalconDockerImgName = common.GetEnv(
		"FALCON_WORKER_IMAGE",
		"lemonwyc/falcon-lime:paper-parallel")

	switch svcName {
	case common.CoordinatorRole:
		// find the cluster port, call internally
		common.CoordIP = common.GetEnv("COORD_SERVER_IP", "127.0.0.1")
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")
		common.CoordAddr = common.CoordIP + ":" + common.CoordPort
		common.CoordBasePath = common.GetEnv("COORD_SERVER_BASEPATH", "./falcon_logs")
		// coord http server number of consumers
		common.NbConsumers = common.GetEnv("N_CONSUMER", "3")
		// get the env for Job DB in Coord server
		common.JobDatabase = common.GetEnv("JOB_DATABASE", "sqlite3")
		// get the env needed for different db type
		if common.JobDatabase == common.DBSqlite3 {
			common.JobDbSqliteDb = common.GetEnv("JOB_DB_SQLITE_DB", "falcon.db")
		} else if common.JobDatabase == common.DBMySQL {
			common.JobDbHost = common.GetEnv("JOB_DB_HOST", "localhost")
			common.JobDbMysqlUser = common.GetEnv("JOB_DB_MYSQL_USER", "falcon")
			common.JobDbMysqlPwd = common.GetEnv("JOB_DB_MYSQL_PWD", "falcon")
			common.JobDbMysqlDb = common.GetEnv("JOB_DB_MYSQL_DB", "falcon")
			common.JobDbMysqlOptions = common.GetEnv("JOB_DB_MYSQL_OPTIONS", "?parseTime=true")
			common.JobDbMysqlPort = common.GetEnv("MYSQL_CLUSTER_PORT", "3306")
		}

		if common.Deployment == common.K8S {
			// find the cluster port, call internally
			common.JobDbMysqlNodePort = common.GetEnv("MYSQL_NODE_PORT", "30001")

			// coord needs redis information
			common.RedisHost = common.GetEnv("REDIS_HOST", "localhost")
			common.RedisPwd = common.GetEnv("REDIS_PWD", "falcon")
			common.RedisPort = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
			common.RedisNodePort = common.GetEnv("REDIS_NODE_PORT", "30003")
			common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")
		}
		if len(common.ServiceName) == 0 {
			logger.Log.Println("Error: Input Error, ServiceName not provided, is either 'coord' or 'partyserver' ")
			time.Sleep(5 * time.Minute)
		}
	case common.PartyServerRole:
		// party server needs coord IP+port,lis port
		common.CoordIP = common.GetEnv("COORD_SERVER_IP", "")
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")
		common.PartyServerIP = common.GetEnv("PARTY_SERVER_IP", "")
		// party server communicate coord with IP+port
		common.CoordAddr = common.CoordIP + ":" + common.CoordPort
		// run partyserver requires to get a new partyserver port
		common.PartyServerPort = common.GetEnv("PARTY_SERVER_NODE_PORT", "")
		partyId, err := strconv.Atoi(common.GetEnv("PARTY_ID", ""))
		common.PartyID = common.PartyIdType(partyId)
		if err != nil {
			logger.Log.Println("Error: Input Error, PARTY_ID must be integer")
			time.Sleep(5 * time.Minute)
		}

		// This should be the absolute path /falcon_logs
		//common.PartyServerBasePath = common.GetEnv("PARTY_SERVER_BASEPATH", "")

		// get the MPC exe path
		common.MpcExePath = common.GetEnv(
			"MPC_EXE_PATH",
			"/opt/falcon/third_party/MP-SPDZ/semi-party.x")
		// get the FL engine exe path
		common.FLEnginePath = common.GetEnv(
			"FL_ENGINE_PATH",
			"/opt/falcon/build/src/executor/falcon")

		tmp := strings.Split(common.MpcExePath, "/")
		basePath := tmp[:len(tmp)-1]
		basePath = append(basePath, common.MpcExecutorPortFilePrefix)
		common.MpcExecutorPortFileBasePath = strings.Join(basePath, "/")
		logger.Log.Println("common.MpcExecutorPortFileBasePath is", common.MpcExecutorPortFileBasePath)

		if common.CoordIP == "" || common.PartyServerIP == "" || common.PartyServerPort == "" {
			logger.Log.Println("Error: Input Error, either CoordIP or PartyServerIP or PartyServerPort not provided")
			time.Sleep(5 * time.Minute)
		}

		common.PartyServerClusterIPs = strings.Split(
			common.GetEnv("PARTY_SERVER_CLUSTER_IPS", ""),
			" ")

		common.PartyServerClusterLabels = strings.Split(
			common.GetEnv("PARTY_SERVER_CLUSTER_LABEL", ""),
			" ")

		logger.Log.Println("common.PartyServerClusterIPs is", common.PartyServerClusterIPs)
		logger.Log.Println("common.PartyServerClusterIPs is", common.PartyServerClusterLabels)

		if len(common.PartyServerClusterIPs) != len(common.PartyServerClusterLabels) {
			logger.Log.Println("Error: Input Error, " +
				"size of PartyServerClusterIPs and size of PartyServerClusterLabels are not match")
			time.Sleep(5 * time.Minute)
		}

	//////////////////////////////////////////////////////////////////////////
	//			start tasks, called internally 	, and in k8s or docker      	//
	// 	      master and worker are isolate process, this is the entry      //
	//////////////////////////////////////////////////////////////////////////
	case common.JobManagerMasterRole:
		common.CoordPort = common.GetEnv("COORD_SERVER_PORT", "30004")

		// master needs dslqueue item, task type
		common.MasterDslObjKey = common.GetEnv("ITEM_KEY", "")
		common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")

		if common.Deployment == common.LocalThread {
			// master communicate coord with IP+port in local thread

			logger.Log.Println("CoordUrl: ", common.CoordIP+":"+common.CoordPort)
			common.CoordAddr = common.CoordIP + ":" + common.CoordPort

		} else if common.Deployment == common.K8S {

			// master needs redis information
			common.RedisHost = common.GetEnv("REDIS_HOST", "localhost")
			common.RedisPwd = common.GetEnv("REDIS_PWD", "falcon")
			common.RedisPort = common.GetEnv("REDIS_CLUSTER_PORT", "30002")
			common.RedisNodePort = common.GetEnv("REDIS_NODE_PORT", "30003")

			// k8
			common.CoordK8sSvcName = common.GetEnv("COORD_SVC_NAME", "")
			common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")

			// master communicate coord with IP+port in local thread, with name+port in k8s
			logger.Log.Println("CoordK8sSvcName: ", common.CoordK8sSvcName+":"+common.CoordPort)

			common.CoordAddr = common.CoordK8sSvcName + ":" + common.CoordPort
		}

		if common.CoordAddr == "" {
			logger.Log.Println("Error: Input Error, CoordAddr not provided")
			time.Sleep(5 * time.Minute)
		}
	case common.TrainWorker:
		// this will be executed only in k8s or docker, in local thread, the common.WorkerType==""

		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		//common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerAddr = common.GetEnv("WORKER_ADDR", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")

		workerId, _ := strconv.Atoi(common.GetEnv("WORKER_ID", ""))
		common.WorkerID = common.WorkerIdType(workerId)

		GroupId, _ := strconv.Atoi(common.GetEnv("GROUP_ID", ""))
		common.GroupID = common.GroupIdType(GroupId)

		partyId, _ := strconv.Atoi(common.GetEnv("PARTY_ID", ""))
		common.PartyID = common.PartyIdType(partyId)

		common.DistributedRole, _ = strconv.Atoi(common.GetEnv("DISTRIBUTED_ROLE", ""))

		// get the MPC exe path
		common.MpcExePath = common.GetEnv(
			"MPC_EXE_PATH",
			"/opt/falcon/third_party/MP-SPDZ/semi-party.x")
		// get the FL engine exe path
		common.FLEnginePath = common.GetEnv(
			"FL_ENGINE_PATH",
			"/opt/falcon/build/src/executor/falcon")

		tmp := strings.Split(common.MpcExePath, "/")
		basePath := tmp[:len(tmp)-1]
		basePath = append(basePath, common.MpcExecutorPortFilePrefix)
		common.MpcExecutorPortFileBasePath = strings.Join(basePath, "/")
		logger.Log.Println("common.MpcExecutorPortFileBasePath is", common.MpcExecutorPortFileBasePath)

		if common.MasterAddr == "" || common.WorkerAddr == "" {
			logger.Log.Println("Error: Input Error, either MasterAddr or WorkerAddr  not provided")
			time.Sleep(5 * time.Minute)
		}

		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")

	case common.InferenceWorker:
		// this will be executed only in k8s or docker, in local thread, the common.WorkerType==""

		common.TaskDataPath = common.GetEnv("TASK_DATA_PATH", "")
		common.TaskModelPath = common.GetEnv("TASK_MODEL_PATH", "")
		common.TaskDataOutput = common.GetEnv("TASK_DATA_OUTPUT", "")
		common.TaskRuntimeLogs = common.GetEnv("RUN_TIME_LOGS", "")

		//common.WorkerType = common.GetEnv("EXECUTOR_TYPE", "")
		common.WorkerAddr = common.GetEnv("WORKER_ADDR", "")
		common.MasterAddr = common.GetEnv("MASTER_ADDR", "")

		workerId, _ := strconv.Atoi(common.GetEnv("WORKER_ID", ""))
		common.WorkerID = common.WorkerIdType(workerId)
		common.DistributedRole, _ = strconv.Atoi(common.GetEnv("DISTRIBUTED_ROLE", ""))

		// get the MPC exe path
		common.MpcExePath = common.GetEnv(
			"MPC_EXE_PATH",
			"/opt/falcon/third_party/MP-SPDZ/semi-party.x")
		// get the FL engine exe path
		common.FLEnginePath = common.GetEnv(
			"FL_ENGINE_PATH",
			"/opt/falcon/build/src/executor/falcon")

		tmp := strings.Split(common.MpcExePath, "/")
		basePath := tmp[:len(tmp)-1]
		basePath = append(basePath, common.MpcExecutorPortFilePrefix)
		common.MpcExecutorPortFileBasePath = strings.Join(basePath, "/")
		logger.Log.Println("common.MpcExecutorPortFileBasePath is", common.MpcExecutorPortFileBasePath)

		if common.MasterAddr == "" || common.WorkerAddr == "" {
			logger.Log.Println("Error: Input Error, either MasterAddr or WorkerAddr not provided")
			time.Sleep(5 * time.Minute)
		}

		common.WorkerK8sSvcName = common.GetEnv("EXECUTOR_NAME", "")
	}
}

func main() {

	// For convenience,
	// this is the main common entry of 4 services,
	// use different environment to select which service to run
	// catch global error at main

	// 1. ServiceName=coord to run coord server
	// 2. ServiceName=partyserver to run party server
	// 3. ServiceName=Master to run Master server
	// 4. ServiceName=TrainWorker to run TrainWorker server
	// 5. ServiceName=InferenceWorker to run InferenceWorker server

	// close log second
	defer func() {
		logger.Log.Println("Main close logFile")
		_ = logger.LogFile.Close()
	}()
	// handle error firstly
	defer logger.HandleErrors()

	switch common.ServiceName {
	case common.CoordinatorRole:
		logger.Log.Println("Launch falcon_platform, Service is ", common.ServiceName)
		nConsumer, _ := strconv.Atoi(common.NbConsumers)
		coordserver.RunCoordServer(nConsumer)

	case common.PartyServerRole:
		// start work in remote machine automatically
		logger.Log.Println("Launch falcon_platform, Service is ", common.ServiceName)
		partyserver.RunPartyServer()

	//////////////////////////////////////////////////////////////////////////
	//			start tasks, called internally 	, and in k8s or docker      //
	// 	      master and worker are isolate process, this is the entry      //
	//////////////////////////////////////////////////////////////////////////

	//those 3 are only called inside cluster,
	case common.JobManagerMasterRole:
		logger.Log.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)
		//// this should be the service name, defined at runtime,
		//masterAddr := common.MasterAddr
		//dslOjb := cache.Deserialize(cache.InitRedisClient().Get(common.MasterDslObjKey))
		//workerType := common.WorkerType
		//jobmanager.ManageJobLifeCycle(masterAddr, dslOjb, workerType)
		//// kill the related service after finish training or prediction.
		//km := resourcemanager.InitK8sManager(true, "")
		//km.DeleteResource(common.WorkerK8sSvcName)

	case common.TrainWorker:
		logger.Log.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)
		// init the train worker with addresses of master and worker, also the partyID
		// this is the entry of prod model, in prod, only use NativeProcessMngr right now
		wk := worker.InitTrainWorker(common.MasterAddr, common.WorkerAddr, common.PartyID,
			common.WorkerID, common.GroupID, uint(common.DistributedRole))

		wk.RunWorker(wk)

		if common.Deployment == common.K8S {
			// once  worker is killed by master, clear the resources.
			km := resourcemanager.InitK8sManager(true, "")
			km.DeleteResource(common.WorkerK8sSvcName)
		}

	case common.InferenceWorker:
		logger.Log.Println("Launching falcon_platform, the common.WorkerType", common.WorkerType)
		// init the train worker with addresses of master and worker, also the partyID
		// this is the entry of prod model, in prod, only use NativeProcessMngr right now
		wk := worker.InitInferenceWorker(common.MasterAddr, common.WorkerAddr, common.PartyID,
			common.WorkerID, uint(common.DistributedRole))

		wk.RunWorker(wk)

		if common.Deployment == common.K8S {
			// once worker is killed by master, clear the resources.
			km := resourcemanager.InitK8sManager(true, "")
			km.DeleteResource(common.WorkerK8sSvcName)
		}
	}

	//time.Sleep(time.Minute * 60)
	os.Exit(0)
}
