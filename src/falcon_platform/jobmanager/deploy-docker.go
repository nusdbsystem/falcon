package jobmanager

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"os/exec"
	"strings"
)

// run master in a docker
func DeployMasterDocker(dslOjb *cache.DslObj, workerType string) {
	// todo, only for testing, no need run master in docker, still run master in thread
	DeployMasterThread(dslOjb, workerType)
}

/**
 * @Description: run worker in docker container, for testing distributed training.
				 run the following cmd:

	target: port inside docker
	published: port of the host

	docker run --name centralized-worker-1-job-1-train-party-1
				-p 13990-14010:13990-14010
				-p 4990-5010:4990-5010
				--publish published=51487,target=51487
				--publish published=51488,target=51488
				--mount type=bind,source=/Users/nailixing/Downloads/falconExp/data,destination=/dataPath
				--mount type=bind,source=/Users/nailixing/Downloads/falconExp/dataoutput,destination=/dataOutputPath
				--mount type=bind,source=/Users/nailixing/Downloads/falconExp/model,destination=/modelPath
				--mount type=bind,source=/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/falcon_logs/runtime_logs/centralized-worker-1-job-1-train-party-1,destination=/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/falcon_logs/runtime_logs/centralized-worker-1-job-1-train-party-1
				--env MASTER_ADDR=127.0.0.1:51484
				--env RUN_TIME_LOGS=/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/falcon_logs/runtime_logs/centralized-worker-1-job-1-train-party-1
				--env EXECUTOR_TYPE=TrainWorker
				--env WORKER_ADDR=127.0.0.1:51487 --env TASK_DATA_PATH=/dataPath
				--env TASK_MODEL_PATH=/modelPath --env TASK_DATA_OUTPUT=/dataOutputPath
				--env MPC_EXE_PATH=/opt/falcon/third_party/MP-SPDZ/semi-party.x
				--env FL_ENGINE_PATH=/opt/falcon/build/src/executor/falcon
				--env WORKER_ID=1
				--env DISTRIBUTED_ROLE=2
				-e constraint:labels.name==party-server
				redis:5.0.3-alpine3.8

	docker service create --name centralized-worker-1-job-1-train-party-1
						  --replicas 1 -p 13990-14010:13990-14010
						  -p 4990-5010:4990-5010
						  --publish published=51487,target=51487
						  --publish published=51488,target=51488
						  --mount type=bind,source=/Users/nailixing/Downloads/falconExp/data,destination=/dataPath
						  --mount type=bind,source=/Users/nailixing/Downloads/falconExp/dataoutput,destination=/dataOutputPath
						  --mount type=bind,source=/Users/nailixing/Downloads/falconExp/model,destination=/modelPath
						  --mount type=bind,source=/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/falcon_logs/runtime_logs/centralized-worker-1-job-1-train-party-1,destination=/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/falcon_logs/runtime_logs/centralized-worker-1-job-1-train-party-1
						  --env MASTER_ADDR=127.0.0.1:51484 --env RUN_TIME_LOGS=/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/falcon_logs/runtime_logs/centralized-worker-1-job-1-train-party-1
						  --env EXECUTOR_TYPE=TrainWorker
						  --env WORKER_ADDR=127.0.0.1:51487
						  --env TASK_DATA_PATH=/dataPath
					      --env TASK_MODEL_PATH=/modelPath
						  --env TASK_DATA_OUTPUT=/dataOutputPath
						  --env MPC_EXE_PATH=/opt/falcon/third_party/MP-SPDZ/semi-party.x
						  --env FL_ENGINE_PATH=/opt/falcon/build/src/executor/falcon
						  --env WORKER_ID=1
						  --env DISTRIBUTED_ROLE=2
						  --constraint node.labels.name==party-server
						  redis:5.0.3-alpine3.8

 * @Date 2:14 下午 1/12/20
 * @Param
 	masterAddr： IP of the masterAddr addr
	workerType： train or inference worker
	jobId： jobId
	dataPath： data folder path of this party
	modelPath： the path to save trained model
	dataOutput： path to store processed data
	workerID： id of the worker, mainly used to distinguish workers in distributed training
	distributedRole： 0: ps, 1: worker
	nodeLabel: deploy the worker container to the server whose label is "nodeLabel"
 **/
func DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string,
	resourceSVC *common.ResourceSVC, distributedRole uint, nodeLabel string, stage string) {

	stageName := strings.Join(strings.Split(stage, "_")[1:], "-")

	workerAddr := resourceSVC.ToAddr(resourceSVC.WorkerPort)

	// 1. generate service name
	var serviceName string
	roleName := common.DistributedRoleToName(distributedRole)
	var localTaskRuntimeLogs string
	if workerType == common.TrainWorker {
		serviceName =
			"pty" + fmt.Sprintf("%d-", common.PartyID) +
				roleName + fmt.Sprintf("%d", resourceSVC.WorkerId) +
				"-job" + jobId +
				"-tr-" + stageName
		localTaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName

		logger.Log.Println("[JobManager]: Current in docker, TrainWorker, svcName", serviceName)

	} else if workerType == common.InferenceWorker {
		serviceName =
			"party" + fmt.Sprintf("%d-", common.PartyID) +
				roleName + fmt.Sprintf("%d", resourceSVC.WorkerId) +
				"-job" + jobId +
				"-predict-" + stageName
		localTaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName

		logger.Log.Println("[JobManager]: Current in docker, InferenceWorker, svcName", serviceName)
	}

	logger.Log.Println("[JobManager]: localTaskRuntimeLogs is ", localTaskRuntimeLogs)
	//ee := os.MkdirAll(localTaskRuntimeLogs, os.ModePerm)
	//if ee != nil {
	//	logger.Log.Fatalln("[JobManager]: Creating TaskRuntimeLogs folder error", ee)
	//}

	var usedImage string
	if common.IsDebug == common.DebugOn {
		fmt.Println(common.TestImgName)
		usedImage = common.TestImgName
	} else {
		usedImage = common.FalconDockerImgName
	}

	var absLogPath []rune
	absLogPath = []rune(common.LogPath)
	if absLogPath[0] == '.' {
		absLogPath = absLogPath[1:]
	}

	var dockerCmd *exec.Cmd

	// 2. generate cmd
	// retrieve envs from the current partyServer envs, and assign them to worker container
	// such that worker process can read them from container
	// port mapping
	//currentPath, _ := os.Getwd()
	//currentTime := time.Now().Unix()

	if distributedRole == common.CentralizedWorker {
		// if CentralizedWorker, 3 port needed, one communicate with master, one for falcon, one for mpc
		logger.Log.Println("[JobManager]: CentralizedWorker, map 2 ports to host")

		logger.Log.Printf("[JobManager]: CentralizedWorker, WorkerPort is %d, ExecutorExecutorPort is %d,\n",
			resourceSVC.WorkerPort, resourceSVC.ExecutorExecutorPort)

		dockerCmd = exec.Command(
			"docker", "service", "create",
			"--name", fmt.Sprintf("%s", serviceName),
			"--network", "host",
			"--replicas", "1",
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", dataPath)+
				fmt.Sprintf(",destination=%s", common.DataPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", dataOutput)+
				fmt.Sprintf(",destination=%s", common.DataOutputPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", modelPath)+
				fmt.Sprintf(",destination=%s", common.ModelPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", string(absLogPath))+
				fmt.Sprintf(",destination=%s", string(absLogPath)),
			"--env", fmt.Sprintf("ENV=%s", common.Docker),
			"--env", fmt.Sprintf("SERVICE_NAME=%s", workerType),
			"--env", fmt.Sprintf("LOG_PATH=%s", common.LogPath),
			"--env", fmt.Sprintf("TASK_DATA_PATH=%s", common.DataPathContainer),
			"--env", fmt.Sprintf("TASK_MODEL_PATH=%s", common.ModelPathContainer),
			"--env", fmt.Sprintf("TASK_DATA_OUTPUT=%s", common.DataOutputPathContainer),
			"--env", fmt.Sprintf("RUN_TIME_LOGS=%s", localTaskRuntimeLogs),
			"--env", fmt.Sprintf("WORKER_ADDR=%s", workerAddr),
			"--env", fmt.Sprintf("MASTER_ADDR=%s", masterAddr),
			"--env", fmt.Sprintf("WORKER_ID=%s", fmt.Sprintf("%d", resourceSVC.WorkerId)),
			"--env", fmt.Sprintf("GROUP_ID=%s", fmt.Sprintf("%d", resourceSVC.GroupId)),
			"--env", fmt.Sprintf("PARTY_ID=%s", fmt.Sprintf("%d", common.PartyID)),
			"--env", fmt.Sprintf("DISTRIBUTED_ROLE=%s", fmt.Sprintf("%d", distributedRole)),
			"--env", fmt.Sprintf("EXECUTOR_TYPE=%s", workerType),
			"--env", fmt.Sprintf("MPC_EXE_PATH=%s", common.MpcExePath),
			"--env", fmt.Sprintf("FL_ENGINE_PATH=%s", common.FLEnginePath),
			"--constraint", fmt.Sprintf("node.labels.name==%s", nodeLabel),
			"--restart-condition", "on-failure",
			usedImage,
		)

	} else if distributedRole == common.DistributedWorker {
		// if DistributedWorker, 4 port needed, one communicate with master, one for falcon, one for mpc, one for ps
		logger.Log.Println("[JobManager]: DistributedWorker, map 3 ports to host")

		logger.Log.Printf("[JobManager]: DistributedWorker, WorkerPort is %d, ExecutorExecutorPort is %d, ExecutorPSPort is %d\n",
			resourceSVC.WorkerPort, resourceSVC.ExecutorExecutorPort, resourceSVC.ExecutorPSPort)

		dockerCmd = exec.Command(
			"docker", "service", "create",
			"--name", fmt.Sprintf("%s", serviceName),
			"--network", "host",
			"--replicas", "1",
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", dataPath)+
				fmt.Sprintf(",destination=%s", common.DataPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", dataOutput)+
				fmt.Sprintf(",destination=%s", common.DataOutputPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", modelPath)+
				fmt.Sprintf(",destination=%s", common.ModelPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", string(absLogPath))+
				fmt.Sprintf(",destination=%s", string(absLogPath)),
			"--env", fmt.Sprintf("ENV=%s", common.LocalThread),
			"--env", fmt.Sprintf("SERVICE_NAME=%s", workerType),
			"--env", fmt.Sprintf("LOG_PATH=%s", common.LogPath),
			"--env", fmt.Sprintf("TASK_DATA_PATH=%s", common.DataPathContainer),
			"--env", fmt.Sprintf("TASK_MODEL_PATH=%s", common.ModelPathContainer),
			"--env", fmt.Sprintf("TASK_DATA_OUTPUT=%s", common.DataOutputPathContainer),
			"--env", fmt.Sprintf("RUN_TIME_LOGS=%s", localTaskRuntimeLogs),
			"--env", fmt.Sprintf("WORKER_ADDR=%s", workerAddr),
			"--env", fmt.Sprintf("MASTER_ADDR=%s", masterAddr),
			"--env", fmt.Sprintf("WORKER_ID=%s", fmt.Sprintf("%d", resourceSVC.WorkerId)),
			"--env", fmt.Sprintf("GROUP_ID=%s", fmt.Sprintf("%d", resourceSVC.GroupId)),
			"--env", fmt.Sprintf("PARTY_ID=%s", fmt.Sprintf("%d", common.PartyID)),
			"--env", fmt.Sprintf("DISTRIBUTED_ROLE=%s", fmt.Sprintf("%d", distributedRole)),
			"--env", fmt.Sprintf("EXECUTOR_TYPE=%s", workerType),
			"--env", fmt.Sprintf("MPC_EXE_PATH=%s", common.MpcExePath),
			"--env", fmt.Sprintf("FL_ENGINE_PATH=%s", common.FLEnginePath),
			"--constraint", fmt.Sprintf("node.labels.name==%s", nodeLabel),
			"--restart-condition", "on-failure",
			usedImage,
		)

	} else if distributedRole == common.DistributedParameterServer {
		// if DistributedParameterServer, 2 port needed, one communicate with master, one train worker
		logger.Log.Println("[JobManager]: DistributedParameterServer, map 2 ports to host")

		logger.Log.Printf("[JobManager]: DistributedParameterServer, WorkerPort is %d, PsExecutorPorts is %d\n",
			resourceSVC.WorkerPort, resourceSVC.PsExecutorPorts)

		dockerCmd = exec.Command(
			"docker", "service", "create",
			"--name", fmt.Sprintf("%s", serviceName),
			"--network", "host",
			"--replicas", "1",
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", dataPath)+
				fmt.Sprintf(",destination=%s", common.DataPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", dataOutput)+
				fmt.Sprintf(",destination=%s", common.DataOutputPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", modelPath)+
				fmt.Sprintf(",destination=%s", common.ModelPathContainer),
			"--mount", "type=bind,"+
				fmt.Sprintf("source=%s", string(absLogPath))+
				fmt.Sprintf(",destination=%s", string(absLogPath)),
			"--env", fmt.Sprintf("ENV=%s", common.LocalThread),
			"--env", fmt.Sprintf("SERVICE_NAME=%s", workerType),
			"--env", fmt.Sprintf("LOG_PATH=%s", common.LogPath),
			"--env", fmt.Sprintf("TASK_DATA_PATH=%s", common.DataPathContainer),
			"--env", fmt.Sprintf("TASK_MODEL_PATH=%s", common.ModelPathContainer),
			"--env", fmt.Sprintf("TASK_DATA_OUTPUT=%s", common.DataOutputPathContainer),
			"--env", fmt.Sprintf("RUN_TIME_LOGS=%s", localTaskRuntimeLogs),
			"--env", fmt.Sprintf("WORKER_ADDR=%s", workerAddr),
			"--env", fmt.Sprintf("MASTER_ADDR=%s", masterAddr),
			"--env", fmt.Sprintf("WORKER_ID=%s", fmt.Sprintf("%d", resourceSVC.WorkerId)),
			"--env", fmt.Sprintf("GROUP_ID=%s", fmt.Sprintf("%d", resourceSVC.GroupId)),
			"--env", fmt.Sprintf("PARTY_ID=%s", fmt.Sprintf("%d", common.PartyID)),
			"--env", fmt.Sprintf("DISTRIBUTED_ROLE=%s", fmt.Sprintf("%d", distributedRole)),
			"--env", fmt.Sprintf("EXECUTOR_TYPE=%s", workerType),
			"--env", fmt.Sprintf("MPC_EXE_PATH=%s", common.MpcExePath),
			"--env", fmt.Sprintf("FL_ENGINE_PATH=%s", common.FLEnginePath),
			"--constraint", fmt.Sprintf("node.labels.name==%s", nodeLabel),
			"--restart-condition", "on-failure",
			usedImage,
		)

	}

	rm := resourcemanager.InitResourceManager()

	go func() {
		defer logger.HandleErrors()
		if common.IsDebug == "debug-on" {
			logger.Log.Println("[JobManager]: run cmd", dockerCmd)
			return
		}
		rm.CreateResources(resourcemanager.InitDockerManager(), dockerCmd)
		rm.ReleaseResources()
	}()

}

//func DeployWorkerDockerContainer(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string,
//	resourceSVC *common.ResourceSVC, distributedRole uint, nodeLabel string) {
//
//	workerAddr := resourceSVC.ToAddr(resourceSVC.WorkerPort)
//
//	// 1. generate service name
//	var serviceName string
//	roleName := common.DistributedRoleToName(distributedRole)
//	if workerType == common.TrainWorker {
//		serviceName =
//			"party" + fmt.Sprintf("%d-", common.PartyID) +
//				roleName + fmt.Sprintf("%d", resourceSVC.WorkerId) +
//				"-job" + jobId +
//				"-train"
//		localTaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName
//
//		logger.Log.Println("[JobManager]: Current in docker, TrainWorker, svcName", serviceName)
//
//	} else if workerType == common.InferenceWorker {
//		serviceName =
//			"party" + fmt.Sprintf("%d-", common.PartyID) +
//				roleName + fmt.Sprintf("%d", resourceSVC.WorkerId) +
//				"-job" + jobId +
//				"-predict"
//		localTaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName
//
//		logger.Log.Println("[JobManager]: Current in docker, InferenceWorker, svcName", serviceName)
//	}
//
//	logger.Log.Println("[JobManager]: localTaskRuntimeLogs is ", localTaskRuntimeLogs)
//	//ee := os.MkdirAll(localTaskRuntimeLogs, os.ModePerm)
//	//if ee != nil {
//	//	logger.Log.Fatalln("[JobManager]: Creating TaskRuntimeLogs folder error", ee)
//	//}
//
//	var usedImage string
//	if common.IsDebug == common.DebugOn {
//		fmt.Println(common.TestImgName)
//		usedImage = common.TestImgName
//	} else {
//		usedImage = common.FalconDockerImgName
//	}
//
//	var absLogPath []rune
//	absLogPath = []rune(common.LogPath)
//	if absLogPath[0] == '.' {
//		absLogPath = absLogPath[1:]
//	}
//
//	var dockerCmd *exec.Cmd
//
//	// 2. generate cmd
//	// retrieve envs from the current partyServer envs, and assign them to worker container
//	// such that worker process can read them from container
//	// port mapping
//	//currentPath, _ := os.Getwd()
//	currentTime := time.Now().Unix()
//
//	if distributedRole == common.CentralizedWorker {
//		// if CentralizedWorker, 3 port needed, one communicate with master, one for falcon, one for mpc
//		logger.Log.Println("[JobManager]: CentralizedWorker, map 2 ports to host")
//
//		logger.Log.Printf("[JobManager]: CentralizedWorker, WorkerPort is %d, ExecutorExecutorPort is %d,\n",
//			resourceSVC.WorkerPort, resourceSVC.ExecutorExecutorPort)
//
//		dockerCmd = exec.Command(
//			"docker", "run",
//			"-d",
//			"--name", fmt.Sprintf("%s-time-%d", serviceName, currentTime),
//			"--net", "host",
//			"--publish", fmt.Sprintf("published=%d,target=%d",
//				resourceSVC.WorkerPort, resourceSVC.WorkerPort),
//			"-p", fmt.Sprintf("%d-%d:%d-%d",
//				utils.MinV(resourceSVC.ExecutorExecutorPort), utils.MaxV(resourceSVC.ExecutorExecutorPort),
//				utils.MinV(resourceSVC.ExecutorExecutorPort), utils.MaxV(resourceSVC.ExecutorExecutorPort)),
//			"--publish", fmt.Sprintf("published=%d,target=%d",
//				resourceSVC.MpcMpcPort, resourceSVC.MpcMpcPort),
//			"--publish", fmt.Sprintf("published=%d,target=%d",
//				resourceSVC.MpcExecutorPort+common.PortType(common.PartyID), resourceSVC.MpcExecutorPort+common.PortType(common.PartyID)),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", dataPath)+
//				fmt.Sprintf(",destination=%s", common.DataPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", dataOutput)+
//				fmt.Sprintf(",destination=%s", common.DataOutputPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", modelPath)+
//				fmt.Sprintf(",destination=%s", common.ModelPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", string(absLogPath))+
//				fmt.Sprintf(",destination=%s", string(absLogPath)),
//			"--env", fmt.Sprintf("ENV=%s", common.Docker),
//			"--env", fmt.Sprintf("SERVICE_NAME=%s", workerType),
//			"--env", fmt.Sprintf("LOG_PATH=%s", common.LogPath),
//			"--env", fmt.Sprintf("TASK_DATA_PATH=%s", common.DataPathContainer),
//			"--env", fmt.Sprintf("TASK_MODEL_PATH=%s", common.ModelPathContainer),
//			"--env", fmt.Sprintf("TASK_DATA_OUTPUT=%s", common.DataOutputPathContainer),
//			"--env", fmt.Sprintf("RUN_TIME_LOGS=%s", localTaskRuntimeLogs),
//			"--env", fmt.Sprintf("WORKER_ADDR=%s", workerAddr),
//			"--env", fmt.Sprintf("MASTER_ADDR=%s", masterAddr),
//			"--env", fmt.Sprintf("WORKER_ID=%s", fmt.Sprintf("%d", resourceSVC.WorkerId)),
//			"--env", fmt.Sprintf("PARTY_ID=%s", fmt.Sprintf("%d", common.PartyID)),
//			"--env", fmt.Sprintf("DISTRIBUTED_ROLE=%s", fmt.Sprintf("%d", distributedRole)),
//			"--env", fmt.Sprintf("EXECUTOR_TYPE=%s", workerType),
//			"--env", fmt.Sprintf("MPC_EXE_PATH=%s", common.MpcExePath),
//			"--env", fmt.Sprintf("FL_ENGINE_PATH=%s", common.FLEnginePath),
//			"-e", fmt.Sprintf("constraint.labels.name==%s", nodeLabel),
//			usedImage,
//		)
//
//	} else if distributedRole == common.DistributedWorker {
//		// if DistributedWorker, 4 port needed, one communicate with master, one for falcon, one for mpc, one for ps
//		logger.Log.Println("[JobManager]: DistributedWorker, map 3 ports to host")
//
//		logger.Log.Printf("[JobManager]: DistributedWorker, WorkerPort is %d, ExecutorExecutorPort is %d, ExecutorPSPort is %d\n",
//			resourceSVC.WorkerPort, resourceSVC.ExecutorExecutorPort, resourceSVC.ExecutorPSPort)
//
//		dockerCmd = exec.Command(
//			"docker", "run",
//			"-d",
//			"--name", fmt.Sprintf("%s-time-%d", serviceName, currentTime),
//			"--net", "host",
//			"--publish", fmt.Sprintf("published=%d,target=%d",
//				resourceSVC.WorkerPort, resourceSVC.WorkerPort),
//			"-p", fmt.Sprintf("%d-%d:%d-%d",
//				utils.MinV(resourceSVC.ExecutorExecutorPort), utils.MaxV(resourceSVC.ExecutorExecutorPort),
//				utils.MinV(resourceSVC.ExecutorExecutorPort), utils.MaxV(resourceSVC.ExecutorExecutorPort)),
//			"--publish", fmt.Sprintf("published=%d,target=%d",
//				resourceSVC.MpcMpcPort, resourceSVC.MpcMpcPort),
//			"--publish", fmt.Sprintf("published=%d,target=%d",
//				resourceSVC.MpcExecutorPort+common.PortType(common.PartyID), resourceSVC.MpcExecutorPort+common.PortType(common.PartyID)),
//			"--publish", fmt.Sprintf("published=%d,target=%d",
//				resourceSVC.ExecutorPSPort, resourceSVC.ExecutorPSPort),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", dataPath)+
//				fmt.Sprintf(",destination=%s", common.DataPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", dataOutput)+
//				fmt.Sprintf(",destination=%s", common.DataOutputPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", modelPath)+
//				fmt.Sprintf(",destination=%s", common.ModelPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", string(absLogPath))+
//				fmt.Sprintf(",destination=%s", string(absLogPath)),
//			"--env", fmt.Sprintf("ENV=%s", common.LocalThread),
//			"--env", fmt.Sprintf("SERVICE_NAME=%s", workerType),
//			"--env", fmt.Sprintf("LOG_PATH=%s", common.LogPath),
//			"--env", fmt.Sprintf("TASK_DATA_PATH=%s", common.DataPathContainer),
//			"--env", fmt.Sprintf("TASK_MODEL_PATH=%s", common.ModelPathContainer),
//			"--env", fmt.Sprintf("TASK_DATA_OUTPUT=%s", common.DataOutputPathContainer),
//			"--env", fmt.Sprintf("RUN_TIME_LOGS=%s", localTaskRuntimeLogs),
//			"--env", fmt.Sprintf("WORKER_ADDR=%s", workerAddr),
//			"--env", fmt.Sprintf("MASTER_ADDR=%s", masterAddr),
//			"--env", fmt.Sprintf("WORKER_ID=%s", fmt.Sprintf("%d", resourceSVC.WorkerId)),
//			"--env", fmt.Sprintf("PARTY_ID=%s", fmt.Sprintf("%d", common.PartyID)),
//			"--env", fmt.Sprintf("DISTRIBUTED_ROLE=%s", fmt.Sprintf("%d", distributedRole)),
//			"--env", fmt.Sprintf("EXECUTOR_TYPE=%s", workerType),
//			"--env", fmt.Sprintf("MPC_EXE_PATH=%s", common.MpcExePath),
//			"--env", fmt.Sprintf("FL_ENGINE_PATH=%s", common.FLEnginePath),
//			"-e", fmt.Sprintf("constraint.labels.name==%s", nodeLabel),
//			usedImage,
//		)
//
//	} else if distributedRole == common.DistributedParameterServer {
//		// if DistributedParameterServer, 2 port needed, one communicate with master, one train worker
//		logger.Log.Println("[JobManager]: DistributedParameterServer, map 2 ports to host")
//
//		logger.Log.Printf("[JobManager]: DistributedParameterServer, WorkerPort is %d, PsExecutorPorts is %v \n",
//			resourceSVC.WorkerPort, resourceSVC.PsExecutorPorts)
//
//		dockerCmd = exec.Command(
//			"docker", "run",
//			"-d",
//			"--name", fmt.Sprintf("%s-time-%d", serviceName, currentTime),
//			"--net", "host",
//			"-p", fmt.Sprintf("%d-%d:%d-%d",
//				utils.MinV(resourceSVC.PsExecutorPorts), utils.MaxV(resourceSVC.PsExecutorPorts),
//				utils.MinV(resourceSVC.PsExecutorPorts), utils.MaxV(resourceSVC.PsExecutorPorts)),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", dataPath)+
//				fmt.Sprintf(",destination=%s", common.DataPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", dataOutput)+
//				fmt.Sprintf(",destination=%s", common.DataOutputPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", modelPath)+
//				fmt.Sprintf(",destination=%s", common.ModelPathContainer),
//			"--mount", "type=bind,"+
//				fmt.Sprintf("source=%s", string(absLogPath))+
//				fmt.Sprintf(",destination=%s", string(absLogPath)),
//			"--env", fmt.Sprintf("ENV=%s", common.LocalThread),
//			"--env", fmt.Sprintf("SERVICE_NAME=%s", workerType),
//			"--env", fmt.Sprintf("LOG_PATH=%s", common.LogPath),
//			"--env", fmt.Sprintf("TASK_DATA_PATH=%s", common.DataPathContainer),
//			"--env", fmt.Sprintf("TASK_MODEL_PATH=%s", common.ModelPathContainer),
//			"--env", fmt.Sprintf("TASK_DATA_OUTPUT=%s", common.DataOutputPathContainer),
//			"--env", fmt.Sprintf("RUN_TIME_LOGS=%s", localTaskRuntimeLogs),
//			"--env", fmt.Sprintf("WORKER_ADDR=%s", workerAddr),
//			"--env", fmt.Sprintf("MASTER_ADDR=%s", masterAddr),
//			"--env", fmt.Sprintf("WORKER_ID=%s", fmt.Sprintf("%d", resourceSVC.WorkerId)),
//			"--env", fmt.Sprintf("PARTY_ID=%s", fmt.Sprintf("%d", common.PartyID)),
//			"--env", fmt.Sprintf("DISTRIBUTED_ROLE=%s", fmt.Sprintf("%d", distributedRole)),
//			"--env", fmt.Sprintf("EXECUTOR_TYPE=%s", workerType),
//			"--env", fmt.Sprintf("MPC_EXE_PATH=%s", common.MpcExePath),
//			"--env", fmt.Sprintf("FL_ENGINE_PATH=%s", common.FLEnginePath),
//			"-e", fmt.Sprintf("constraint.labels.name==%s", nodeLabel),
//			usedImage,
//		)
//
//	}
//
//	rm := resourcemanager.InitResourceManager()
//
//	go func() {
//		defer logger.HandleErrors()
//		rm.CreateResources(resourcemanager.InitDockerManager(), dockerCmd)
//		rm.ReleaseResources()
//	}()
//
//}
