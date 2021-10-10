package common

// For user defined variables, define them in userdefined.properties first,
// and then, add to here, Since userdefined.properties is environment varialbes
// and vars are read from envs

// read from user input, is debug or not
var (
	IsDebug string
)

// system init config
var (
	// Deployment
	Deployment string
	// which service to run
	ServiceName string
	// logPath
	LogPath string
)

// vars of coordinator
var (
	// Coord user-defined variables
	CoordIP       string
	CoordPort     string
	CoordBasePath string
	// later concatenated by falcon_platform.go
	CoordAddr string
	// number of consumers used in coord http server
	NbConsumers string
	// enable other service access master with clusterIP+clusterPort, from inside the cluster
	CoordK8sSvcName string
)

// vars of PartyServer
var (
	// user define ip, PartyServer will run at this server
	PartyServerIP   string
	PartyServerPort string
	PartyID         PartyIdType
	//PartyType           string
	PartyServerBasePath string

	// when party has a cluster, list all nodes' Ip address
	PartyServerClusterIPs []string
	// when party has a cluster, list all nodes' labels, used to manually schedule
	PartyServerClusterLabels []string
)

// db config
var (
	// JobDB and Database Configs
	JobDatabase   string
	JobDbSqliteDb string
	// for prod use of mysql
	JobDbHost         string
	JobDbMysqlUser    string
	JobDbMysqlPwd     string
	JobDbMysqlDb      string
	JobDbMysqlOptions string

	// find the cluster port, call internally
	JobDbMysqlPort string

	// used in k8s
	JobDbMysqlNodePort string
)

// cache config
var (
	// redis
	RedisHost string
	RedisPwd  string
	// find the cluster port, call internally
	RedisPort string
	// used in k8s
	RedisNodePort string
)

var (
	WorkerID WorkerIdType
	// used in distributed training, ps, worker, or centralized
	DistributedRole int

	// those are init by falcon_platform
	// train or inference
	WorkerType string
	WorkerAddr string
	MasterAddr string

	// paths used in training
	TaskDataPath    string
	TaskDataOutput  string
	TaskModelPath   string
	TaskRuntimeLogs string

	// this is the worker's k8s service name,
	WorkerK8sSvcName string

	// key of DslObj in cache
	MasterDslObjKey string
)

// falcon executor related vars
var (
	// paths for MPC and FL Engine
	MpcExePath   string
	FLEnginePath string

	MpcExecutorPortFileBasePath string

	// docker image names
	FalconDockerImgName string
	TestImgName         string
)
