package jobmanager

import (
	"falcon_platform/common"
	fl_comms_pattern "falcon_platform/jobmanager/fl_comms_pattern"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"os/exec"
	"strings"
)

// DeployMasterDocker run master in a docker
func deployJobManagerDocker(job *common.TrainJob, workerType string) {
	// todo, only for testing, no need run in docker, still run master in thread.
	// in production, job manager will run in docker.
	manageJobLifeCycle(job, workerType)
}

//DeployWorkerDockerService
// 	masterAddr： IP of the masterAddr addr
//	workerType： train or inference worker
//	jobId： jobId
//	dataPath： data folder path of this party
//	modelPath： the path to save trained model
//	dataOutput： path to store processed data
//	workerID： id of the worker, mainly used to distinguish workers in distributed training
//	nodeLabel: deploy the worker container to the server whose label is "nodeLabel"
func DeployWorkerDockerService(masterAddr, workerType, jobId, dataPath, modelPath, dataOutput string,
	resourceSVC *fl_comms_pattern.ResourceSVC, nodeLabel string, stage string) {

	stageName := strings.Join(strings.Split(stage, "_")[1:], "-")

	workerAddr := resourceSVC.ToAddr(resourceSVC.WorkerPort)

	// 1. generate service name
	var serviceName string
	var localTaskRuntimeLogs string
	if workerType == common.TrainWorker {
		serviceName =
			"pty" + fmt.Sprintf("%d-", common.PartyID) +
				fmt.Sprintf("%d", resourceSVC.WorkerId) +
				"-job" + jobId +
				"-tr-" + stageName
		localTaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName

		logger.Log.Println("[JobManager]: Current in docker, TrainWorker, svcName", serviceName)

	} else if workerType == common.InferenceWorker {
		serviceName =
			"party" + fmt.Sprintf("%d-", common.PartyID) +
				fmt.Sprintf("%d", resourceSVC.WorkerId) +
				"-job" + jobId +
				"-predict-" + stageName
		localTaskRuntimeLogs = common.LogPath + "/" + common.RuntimeLogs + "/" + serviceName
		logger.Log.Println("[JobManager]: Current in docker, InferenceWorker, svcName", serviceName)
	}

	logger.Log.Println("[JobManager]: localTaskRuntimeLogs is ", localTaskRuntimeLogs)

	// get used image
	var usedImage string
	if common.IsDebug == common.DebugOn {
		usedImage = common.TestImgName
	} else {
		usedImage = common.FalconDockerImgName
	}

	// get abs path
	var absLogPath []rune
	absLogPath = []rune(common.LogPath)
	if absLogPath[0] == '.' {
		absLogPath = absLogPath[1:]
	}

	// 2. generate cmd
	dockerCmd := exec.Command(
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
		"--env", fmt.Sprintf("PARTY_ID=%s", fmt.Sprintf("%d", common.PartyID)),
		"--env", fmt.Sprintf("EXECUTOR_TYPE=%s", workerType),
		"--env", fmt.Sprintf("MPC_EXE_PATH=%s", common.MpcExePath),
		"--env", fmt.Sprintf("FL_ENGINE_PATH=%s", common.FLEnginePath),
		"--constraint", fmt.Sprintf("node.labels.name==%s", nodeLabel),
		"--restart-condition", "on-failure",
		usedImage,
	)

	// 3 init resource manager, and run it.
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
