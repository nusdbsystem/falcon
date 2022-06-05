package tasks

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/comms_pattern"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"fmt"
	"os"
	"os/exec"
)

// init register all existing tasks.
func init() {
	allTasks = GetAllTasks()
	allTasks[common.LimeFeatureTaskKey] = new(LimeFeatureTask)
}

type LimeFeatureTask struct {
	TaskAbstract
}

// GetCommand FL Engine requires:
//	("party-id", po::value<int>(&party_id), "current party id")
//	("party-num", po::value<int>(&party_num), "total party num")
//	("party-type", po::value<int>(&party_type), "type of this party, active or passive")
//	("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
//	("network-file", po::value<std::string>(&network_file), "file name of network configurations")
//	("log-file", po::value<std::string>(&log_file), "file name of log destination")
//	("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
//	("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
//	("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
//	("key-file", po::value<std::string>(&key_file), "file name of phe keys")
//	("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
//	// algorithm-params is not needed in inference stage
//	("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
//	("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
//	// model-report-file is not needed in inference stage
//	("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
//	("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
//	("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");
//	("is-distributed", po::value<int>(&is_distributed), "is distributed");
//	("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file");
//	("worker-id", po::value<int>(&worker_id), "worker id");
//	("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");
func (this *LimeFeatureTask) GetCommand(taskInfo *entity.TaskContext) *exec.Cmd {

	wk := taskInfo.Wk
	fLConfig := (*taskInfo.FLNetworkCfg).(*comms_pattern.FLNetworkCfg)
	job := taskInfo.Job

	this.printParams(job.Tasks.LimeFeature.AlgorithmName, job)

	logger.Log.Println("[TrainWorker]: begin tasks RunLimeFeature")

	distRole := fLConfig.WorkerRole[wk.PartyID][wk.WorkerID]

	partyType := common.ConvertPartyType2Int(job.PartyInfoList[wk.PartyIndex].PartyType)
	flSetting := common.ConvertJobFlType2Int(job.JobFlType)

	// 3. generate many files store etc
	modelInputFile := common.TaskDataPath + "/" + job.Tasks.LimeFeature.InputConfigs.DataInput.Data
	modelFile := common.TaskModelPath + "/" + job.Tasks.LimeFeature.OutputConfigs.TrainedModel
	logFile := common.TaskRuntimeLogs + "-" + job.Tasks.LimeFeature.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + job.Tasks.LimeFeature.InputConfigs.DataInput.Key
	modelReportFile := common.TaskModelPath + "/" + job.Tasks.LimeFeature.OutputConfigs.EvaluationReport

	// 3. generate command line
	var usedLogFile string
	var distNetworkCfg string
	// in distributed training situation and this worker is parameter server
	if job.DistributedTask.Enable == 1 && distRole == common.DistributedParameterServer {
		logger.Log.Println("[PartyServer]: distributed training method with current distributed role: ", common.DistributedParameterServer)
		usedLogFile = logFile + "/parameter_server"
		distNetworkCfg = fLConfig.DistributedNetworkCfg[wk.PartyID]
	}

	// in distributed training situation and this worker is train worker
	if job.DistributedTask.Enable == 1 && distRole == common.DistributedWorker {
		logger.Log.Println("[PartyServer]: distributed training method with current distributed role: ", common.DistributedWorker)
		usedLogFile = logFile + "/distributed_worker_" + fmt.Sprintf("%d", wk.WorkerID)
		distNetworkCfg = fLConfig.DistributedNetworkCfg[wk.PartyID]
	}

	// in centralized training
	if job.DistributedTask.Enable == 0 {
		logger.Log.Println("[PartyServer]: training method: centralized")
		usedLogFile = logFile + "/centralized_worker"
		distNetworkCfg = common.EmptyParams
	}

	// create log folder for this tasks
	ee := os.MkdirAll(usedLogFile, os.ModePerm)
	if ee != nil {
		logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
	}

	cmd := exec.Command(
		common.FLEnginePath,
		"--party-id", fmt.Sprintf("%d", job.PartyInfoList[wk.PartyIndex].ID),
		"--party-num", fmt.Sprintf("%d", job.PartyNums),
		"--party-type", fmt.Sprintf("%d", partyType),
		"--fl-setting", fmt.Sprintf("%d", flSetting),
		"--network-file", fLConfig.ExecutorPairNetworkCfg[wk.WorkerID],
		"--log-file", usedLogFile,
		"--data-input-file", modelInputFile,
		"--data-output-file", common.TaskDataOutput,
		"--existing-key", fmt.Sprintf("%d", job.ExistingKey),
		"--key-file", KeyFile,
		"--algorithm-name", job.Tasks.LimeFeature.AlgorithmName,
		"--algorithm-params", job.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig,
		"--model-save-file", modelFile,
		"--model-report-file", modelReportFile,
		"--is-inference", fmt.Sprintf("%d", 0),
		"--inference-endpoint", common.EmptyParams,
		"--is-distributed", fmt.Sprintf("%d", job.DistributedTask.Enable),
		"--distributed-train-network-file", distNetworkCfg,
		"--worker-id", fmt.Sprintf("%d", int(wk.WorkerID)%job.DistributedTask.WorkerNumber),
		"--distributed-role", fmt.Sprintf("%d", distRole),
	)

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	return cmd
}
