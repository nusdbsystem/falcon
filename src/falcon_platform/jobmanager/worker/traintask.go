package worker

import (
	"falcon_platform/common"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"os"
	"os/exec"
	"strings"
	"time"
)

func (wk *TrainWorker) RunMpcTask(algorithmName string) {
	// no need to wait for mpc, once train task done, shutdown the mpc
	logger.Log.Println("[TrainWorker]: spawn thread for mpc")
	go func() {
		defer logger.HandleErrors()
		wk.mpc(algorithmName)
	}()
	// wait 4 seconds until starting ./semi-party.x return task-running or task-failed
	time.Sleep(time.Second * 5)
}

func (wk *TrainWorker) RunPreProcessingTask() {
	/**
		FL Engine requires:
			 ("party-id", po::value<int>(&party_id), "current party id")
	        ("party-num", po::value<int>(&party_num), "total party num")
	        ("party-type", po::value<int>(&party_type), "type of this party, active or passive")
	        ("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
	        ("network-file", po::value<std::string>(&network_file), "file name of network configurations")
	        ("log-file", po::value<std::string>(&log_file), "file name of log destination")
	        ("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
	        ("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
	        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
	        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
	        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
	        // algorithm-params is not needed in inference stage
	        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
	        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
	        // model-report-file is not needed in inference stage
	        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
	        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
	        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");
	        ("is-distributed", po::value<int>(&is_distributed), "is distributed");
	        ("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file");
	        ("worker-id", po::value<int>(&worker_id), "worker id");
	        ("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");
	 **/

	logger.Log.Println("[TrainWorker]: begin task pre-processing")

	// 1. convert party-type to int, partyType 1=Passive, 0=Active
	var partyType int
	partyTypeStr := wk.DslObj.PartyInfo.PartyType
	if partyTypeStr == "active" {
		partyType = 0
	} else if partyTypeStr == "passive" {
		partyType = 1
	}

	// 2. convert fl-type to int, fl-setting 1=vertical, 0=horizontal
	flSetting := 1 // default vertical
	flSettingStr := wk.DslObj.JobFlType
	if flSettingStr == "vertical" {
		flSetting = 1
	} else if flSettingStr == "horizontal" {
		flSetting = 0
	}

	// 3. generate many files store etc
	dataInputFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.PreProcessing.InputConfigs.DataInput.Data
	dataOutFile := common.TaskDataOutput + "/" + wk.DslObj.Tasks.PreProcessing.OutputConfigs.DataOutput
	logFile := common.TaskRuntimeLogs + "-" + wk.DslObj.Tasks.PreProcessing.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.PreProcessing.InputConfigs.DataInput.Key

	_ = os.MkdirAll(logFile, os.ModePerm)

	// 3. generate command line
	// todo, check with wyc how to call pre-process, is model-save-file and model-report needed?
	// this is not defined yet, since there is no such task right now
	cmd := exec.Command(
		common.FLEnginePath,
		"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
		"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
		"--party-type", fmt.Sprintf("%d", partyType),
		"--fl-setting", fmt.Sprintf("%d", flSetting),
		"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
		"--key-file", KeyFile,
		"--network-file", wk.DslObj.ExecutorPairNetworkCfg,

		"--algorithm-name", wk.DslObj.Tasks.PreProcessing.AlgorithmName,
		"--algorithm-params", wk.DslObj.Tasks.PreProcessing.InputConfigs.SerializedAlgorithmConfig,
		"--log-file", logFile,
		"--data-input-file", dataInputFile,
		"--data-output-file", dataOutFile,
		"--model-save-file", "",
		"--model-report-file", "",
	)

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task pre processing start")
	// by default, worker will launch executor resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	}

}

func (wk *TrainWorker) RunModelTrainingTask() {
	/**
		FL Engine requires:
	        ("party-id", po::value<int>(&party_id), "current party id")
	        ("party-num", po::value<int>(&party_num), "total party num")
	        ("party-type", po::value<int>(&party_type), "type of this party, active or passive")
	        ("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
	        ("network-file", po::value<std::string>(&network_file), "file name of network configurations")
	        ("log-file", po::value<std::string>(&log_file), "file name of log destination")
	        ("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
	        ("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
	        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
	        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
	        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
	        // algorithm-params is not needed in inference stage
	        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
	        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
	        // model-report-file is not needed in inference stage
	        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
	        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
	        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");
	        ("is-distributed", po::value<int>(&is_distributed), "is distributed");
	        ("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file");
	        ("worker-id", po::value<int>(&worker_id), "worker id");
	        ("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");
	 **/

	logger.Log.Println("[TrainWorker]: begin task model training")

	// 1. convert party-type to int, partyType 1=Passive, 0=Active
	var partyType int
	partyTypeStr := wk.DslObj.PartyInfo.PartyType
	if partyTypeStr == "active" {
		partyType = 0
	} else if partyTypeStr == "passive" {
		partyType = 1
	}

	// 2. convert fl-type to int, fl-setting 1=vertical, 0=horizontal
	flSetting := 1 // default vertical
	flSettingStr := wk.DslObj.JobFlType
	if flSettingStr == "vertical" {
		flSetting = 1
	} else if flSettingStr == "horizontal" {
		flSetting = 0
	}

	// 3. generate many files store etc
	modelInputFile := common.TaskDataOutput + "/" + wk.DslObj.Tasks.ModelTraining.InputConfigs.DataInput.Data
	modelFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.ModelTraining.OutputConfigs.TrainedModel
	logFile := common.TaskRuntimeLogs + "-" + wk.DslObj.Tasks.ModelTraining.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.ModelTraining.InputConfigs.DataInput.Key
	modelReportFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.ModelTraining.OutputConfigs.EvaluationReport

	// 3. generate command line
	var cmd *exec.Cmd
	// in distributed training situation and this worker is parameter server
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedParameterServer {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedParameterServer)

		psLogFile := logFile + "/parameter_server"
		ee := os.MkdirAll(psLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating parameter server folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", psLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.ModelTraining.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in distributed training situation and this worker is train worker
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedWorker {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedWorker)

		wkLogFile := logFile + "/distributed-worker-" + fmt.Sprintf("%d", wk.WorkerID)
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.ModelTraining.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in centralized training
	if wk.DslObj.DistributedTask.Enable == 0 {

		logger.Log.Println("[PartyServer]: training method: centralized")

		wkLogFile := logFile + "/worker"
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.ModelTraining.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", common.EmptyParams,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task model training start")
	// by default, worker will launch executor resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	}

}

func (wk *TrainWorker) RunLimePredTask() {
	/**
		FL Engine requires:
	        ("party-id", po::value<int>(&party_id), "current party id")
	        ("party-num", po::value<int>(&party_num), "total party num")
	        ("party-type", po::value<int>(&party_type), "type of this party, active or passive")
	        ("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
	        ("network-file", po::value<std::string>(&network_file), "file name of network configurations")
	        ("log-file", po::value<std::string>(&log_file), "file name of log destination")
	        ("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
	        ("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
	        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
	        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
	        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
	        // algorithm-params is not needed in inference stage
	        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
	        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
	        // model-report-file is not needed in inference stage
	        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
	        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
	        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");
	        ("is-distributed", po::value<int>(&is_distributed), "is distributed");
	        ("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file");
	        ("worker-id", po::value<int>(&worker_id), "worker id");
	        ("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");
	 **/

	logger.Log.Println("[TrainWorker]: begin task lime prediction")

	// 1. convert party-type to int, partyType 1=Passive, 0=Active
	var partyType int
	partyTypeStr := wk.DslObj.PartyInfo.PartyType
	if partyTypeStr == "active" {
		partyType = 0
	} else if partyTypeStr == "passive" {
		partyType = 1
	}

	// 2. convert fl-type to int, fl-setting 1=vertical, 0=horizontal
	flSetting := 1 // default vertical
	flSettingStr := wk.DslObj.JobFlType
	if flSettingStr == "vertical" {
		flSetting = 1
	} else if flSettingStr == "horizontal" {
		flSetting = 0
	}

	// 3. generate many files store etc
	modelInputFile := common.TaskDataOutput + "/" + wk.DslObj.Tasks.LimePred.InputConfigs.DataInput.Data
	modelFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimePred.OutputConfigs.TrainedModel
	logFile := common.TaskRuntimeLogs + "-" + wk.DslObj.Tasks.LimePred.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.LimePred.InputConfigs.DataInput.Key
	modelReportFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimePred.OutputConfigs.EvaluationReport

	// 3. generate command line
	var cmd *exec.Cmd
	// in distributed training situation and this worker is parameter server
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedParameterServer {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedParameterServer)

		psLogFile := logFile + "/parameter_server"
		ee := os.MkdirAll(psLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating parameter server folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", psLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimePred.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimePred.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in distributed training situation and this worker is train worker
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedWorker {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedWorker)

		wkLogFile := logFile + "/distributed-worker-" + fmt.Sprintf("%d", wk.WorkerID)
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimePred.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimePred.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in centralized training
	if wk.DslObj.DistributedTask.Enable == 0 {

		logger.Log.Println("[PartyServer]: training method: centralized")

		wkLogFile := logFile + "/worker"
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimePred.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimePred.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", common.EmptyParams,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task lime prediction start")
	// by default, worker will launch executor resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	}

}

func (wk *TrainWorker) RunLimeWeightTask() {
	/**
		FL Engine requires:
	        ("party-id", po::value<int>(&party_id), "current party id")
	        ("party-num", po::value<int>(&party_num), "total party num")
	        ("party-type", po::value<int>(&party_type), "type of this party, active or passive")
	        ("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
	        ("network-file", po::value<std::string>(&network_file), "file name of network configurations")
	        ("log-file", po::value<std::string>(&log_file), "file name of log destination")
	        ("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
	        ("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
	        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
	        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
	        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
	        // algorithm-params is not needed in inference stage
	        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
	        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
	        // model-report-file is not needed in inference stage
	        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
	        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
	        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");
	        ("is-distributed", po::value<int>(&is_distributed), "is distributed");
	        ("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file");
	        ("worker-id", po::value<int>(&worker_id), "worker id");
	        ("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");
	 **/

	logger.Log.Println("[TrainWorker]: begin task RunLimeWeight")

	// 1. convert party-type to int, partyType 1=Passive, 0=Active
	var partyType int
	partyTypeStr := wk.DslObj.PartyInfo.PartyType
	if partyTypeStr == "active" {
		partyType = 0
	} else if partyTypeStr == "passive" {
		partyType = 1
	}

	// 2. convert fl-type to int, fl-setting 1=vertical, 0=horizontal
	flSetting := 1 // default vertical
	flSettingStr := wk.DslObj.JobFlType
	if flSettingStr == "vertical" {
		flSetting = 1
	} else if flSettingStr == "horizontal" {
		flSetting = 0
	}

	// 3. generate many files store etc
	modelInputFile := common.TaskDataOutput + "/" + wk.DslObj.Tasks.LimeWeight.InputConfigs.DataInput.Data
	modelFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimeWeight.OutputConfigs.TrainedModel
	logFile := common.TaskRuntimeLogs + "-" + wk.DslObj.Tasks.LimeWeight.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.LimeWeight.InputConfigs.DataInput.Key
	modelReportFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimeWeight.OutputConfigs.EvaluationReport

	// 3. generate command line
	var cmd *exec.Cmd
	// in distributed training situation and this worker is parameter server
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedParameterServer {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedParameterServer)

		psLogFile := logFile + "/parameter_server"
		ee := os.MkdirAll(psLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating parameter server folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", psLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeWeight.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeWeight.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in distributed training situation and this worker is train worker
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedWorker {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedWorker)

		wkLogFile := logFile + "/distributed-worker-" + fmt.Sprintf("%d", wk.WorkerID)
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeWeight.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeWeight.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in centralized training
	if wk.DslObj.DistributedTask.Enable == 0 {

		logger.Log.Println("[PartyServer]: training method: centralized")

		wkLogFile := logFile + "/worker"
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeWeight.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeWeight.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", common.EmptyParams,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task RunLimeWeight start")
	// by default, worker will launch executor resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	}

}

func (wk *TrainWorker) RunLimeFeatureTask() {
	/**
		FL Engine requires:
	        ("party-id", po::value<int>(&party_id), "current party id")
	        ("party-num", po::value<int>(&party_num), "total party num")
	        ("party-type", po::value<int>(&party_type), "type of this party, active or passive")
	        ("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
	        ("network-file", po::value<std::string>(&network_file), "file name of network configurations")
	        ("log-file", po::value<std::string>(&log_file), "file name of log destination")
	        ("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
	        ("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
	        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
	        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
	        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
	        // algorithm-params is not needed in inference stage
	        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
	        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
	        // model-report-file is not needed in inference stage
	        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
	        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
	        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");
	        ("is-distributed", po::value<int>(&is_distributed), "is distributed");
	        ("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file");
	        ("worker-id", po::value<int>(&worker_id), "worker id");
	        ("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");
	 **/

	logger.Log.Println("[TrainWorker]: begin task RunLimeFeature")

	// 1. convert party-type to int, partyType 1=Passive, 0=Active
	var partyType int
	partyTypeStr := wk.DslObj.PartyInfo.PartyType
	if partyTypeStr == "active" {
		partyType = 0
	} else if partyTypeStr == "passive" {
		partyType = 1
	}

	// 2. convert fl-type to int, fl-setting 1=vertical, 0=horizontal
	flSetting := 1 // default vertical
	flSettingStr := wk.DslObj.JobFlType
	if flSettingStr == "vertical" {
		flSetting = 1
	} else if flSettingStr == "horizontal" {
		flSetting = 0
	}

	// 3. generate many files store etc
	modelInputFile := common.TaskDataOutput + "/" + wk.DslObj.Tasks.LimeFeature.InputConfigs.DataInput.Data
	modelFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimeFeature.OutputConfigs.TrainedModel
	logFile := common.TaskRuntimeLogs + "-" + wk.DslObj.Tasks.LimeFeature.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.LimeFeature.InputConfigs.DataInput.Key
	modelReportFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimeFeature.OutputConfigs.EvaluationReport

	// 3. generate command line
	var cmd *exec.Cmd
	// in distributed training situation and this worker is parameter server
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedParameterServer {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedParameterServer)

		psLogFile := logFile + "/parameter_server"
		ee := os.MkdirAll(psLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating parameter server folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", psLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeFeature.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in distributed training situation and this worker is train worker
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedWorker {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedWorker)

		wkLogFile := logFile + "/distributed-worker-" + fmt.Sprintf("%d", wk.WorkerID)
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeFeature.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in centralized training
	if wk.DslObj.DistributedTask.Enable == 0 {

		logger.Log.Println("[PartyServer]: training method: centralized")

		wkLogFile := logFile + "/worker"
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeFeature.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", common.EmptyParams,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task RunLimeFeature start")
	// by default, worker will launch executor resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	}

}

func (wk *TrainWorker) RunLimeInterpretTask() {
	/**
		FL Engine requires:
	        ("party-id", po::value<int>(&party_id), "current party id")
	        ("party-num", po::value<int>(&party_num), "total party num")
	        ("party-type", po::value<int>(&party_type), "type of this party, active or passive")
	        ("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
	        ("network-file", po::value<std::string>(&network_file), "file name of network configurations")
	        ("log-file", po::value<std::string>(&log_file), "file name of log destination")
	        ("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
	        ("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
	        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
	        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
	        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
	        // algorithm-params is not needed in inference stage
	        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
	        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
	        // model-report-file is not needed in inference stage
	        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
	        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
	        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");
	        ("is-distributed", po::value<int>(&is_distributed), "is distributed");
	        ("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file");
	        ("worker-id", po::value<int>(&worker_id), "worker id");
	        ("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");
	 **/

	logger.Log.Println("[TrainWorker]: begin task Interpret training")

	// 1. convert party-type to int, partyType 1=Passive, 0=Active
	var partyType int
	partyTypeStr := wk.DslObj.PartyInfo.PartyType
	if partyTypeStr == "active" {
		partyType = 0
	} else if partyTypeStr == "passive" {
		partyType = 1
	}

	// 2. convert fl-type to int, fl-setting 1=vertical, 0=horizontal
	flSetting := 1 // default vertical
	flSettingStr := wk.DslObj.JobFlType
	if flSettingStr == "vertical" {
		flSetting = 1
	} else if flSettingStr == "horizontal" {
		flSetting = 0
	}

	// 3. generate many files store etc
	modelInputFile := common.TaskDataOutput + "/" + wk.DslObj.Tasks.LimeInterpret.InputConfigs.DataInput.Data
	modelFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimeInterpret.OutputConfigs.TrainedModel
	logFile := common.TaskRuntimeLogs + "-" + wk.DslObj.Tasks.LimeInterpret.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.LimeInterpret.InputConfigs.DataInput.Key
	modelReportFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.LimeInterpret.OutputConfigs.EvaluationReport

	// 3. generate command line
	var cmd *exec.Cmd
	// in distributed training situation and this worker is parameter server
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedParameterServer {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedParameterServer)

		psLogFile := logFile + "/parameter_server"
		ee := os.MkdirAll(psLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating parameter server folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", psLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeInterpret.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in distributed training situation and this worker is train worker
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedWorker {

		logger.Log.Println("[PartyServer]: training method: distributed, ",
			"current distributed role: ", common.DistributedWorker)

		wkLogFile := logFile + "/distributed-worker-" + fmt.Sprintf("%d", wk.WorkerID)
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeInterpret.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", wk.DslObj.DistributedExecutorPairNetworkCfg,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	// in centralized training
	if wk.DslObj.DistributedTask.Enable == 0 {

		logger.Log.Println("[PartyServer]: training method: centralized")

		wkLogFile := logFile + "/worker"
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command(
			common.FLEnginePath,
			"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
			"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
			"--party-type", fmt.Sprintf("%d", partyType),
			"--fl-setting", fmt.Sprintf("%d", flSetting),
			"--network-file", wk.DslObj.ExecutorPairNetworkCfg,
			"--log-file", wkLogFile,
			"--data-input-file", modelInputFile,
			"--data-output-file", common.EmptyParams,
			"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
			"--key-file", KeyFile,
			"--algorithm-name", wk.DslObj.Tasks.LimeInterpret.AlgorithmName,
			"--algorithm-params", wk.DslObj.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig,
			"--model-save-file", modelFile,
			"--model-report-file", modelReportFile,
			"--is-inference", fmt.Sprintf("%d", 0),
			"--inference-endpoint", common.EmptyParams,
			"--is-distributed", fmt.Sprintf("%d", wk.DslObj.DistributedTask.Enable),
			"--distributed-train-network-file", common.EmptyParams,
			"--worker-id", fmt.Sprintf("%d", wk.WorkerID),
			"--distributed-role", fmt.Sprintf("%d", wk.DistributedRole),
		)
	}

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task model LimeInterpret start")
	// by default, worker will launch executor resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	}

}

func (wk *TrainWorker) mpc(algName string) {
	/**
	 * @Description
	 * @Date 2:52 下午 12/12/20
	 * @Param

		./semi-party.x -F -N 3 -p 0 -I -h 10.0.0.33 -pn 6000 algorithm_name
			-N party_num
			-p party_id
			-h IP
			-pn port
			-ip: network file path
			algorithm_name

		-h is IP of part-0, all semi-party use the same port
		-h each mpc process only requires IP of party-0

		-h 是party_0的IP 端口目前只有一个 各个端口都相同就可以
		-h 每个mpc进程的启动输入都是party_0的IP

		-14000 端口用于和所有executor通信，默认是14000，

		if there is -ip, no need host ip?
	 * @return
	 **/

	logger.Log.Println("[TrainWorker]: begin task mpc")
	partyId := wk.DslObj.PartyInfo.ID
	partyNum := wk.DslObj.PartyNums

	tmpThirdPathList := strings.Split(common.MpcExePath, "/")
	thirdMPSPDZPath := strings.Join(tmpThirdPathList[:len(tmpThirdPathList)-1], "/")
	logger.Log.Printf("[TrainWorker]: Third Party base path for executing semi-party.x is : \"%s\"\n", thirdMPSPDZPath)

	var cmd *exec.Cmd

	// write file to local disk, input path to mpc process
	mpcExecutorPairNetworkCfgPath := fmt.Sprintf("/opt/falcon/third_party/MP-SPDZ/mpc-network-%d", wk.DslObj.PartyInfo.ID)

	tmpFile, err := os.Create(mpcExecutorPairNetworkCfgPath)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: create file error, %s \n", mpcExecutorPairNetworkCfgPath)
		//panic(err)
	}
	_, err = tmpFile.WriteString(wk.DslObj.MpcPairNetworkCfg)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: write file error, %s \n", mpcExecutorPairNetworkCfgPath)
		//panic(err)
	}
	_ = tmpFile.Close()

	logger.Log.Printf("[TrainWorker]: Config mpcExecutorPairNetworkCfgPath file, "+
		"write to file [%s] \n", mpcExecutorPairNetworkCfgPath)

	// write file to local disk, input path to mpc process
	mpcExecutorPortFile := common.MpcExecutorPortFileBasePath + algName
	tmpFile, err = os.Create(mpcExecutorPortFile)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: create file error, %s \n", mpcExecutorPortFile)
		//panic(err)
	}
	_, err = tmpFile.WriteString(wk.DslObj.MpcExecutorNetworkCfg)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: write file error, %s \n", mpcExecutorPortFile)
		//panic(err)
	}
	_ = tmpFile.Close()

	logger.Log.Printf("[TrainWorker]: Config network-mpc, network file, "+
		"write port [%s] to file [%s] \n", wk.DslObj.MpcExecutorNetworkCfg, mpcExecutorPortFile)

	// in distributed training situation and this worker is train worker
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedWorker {
		cmd = exec.Command(
			common.MpcExePath,
			"-F",
			"-N", fmt.Sprintf("%d", partyNum),
			"-p", fmt.Sprintf("%d", partyId),
			"-I",
			"-ip", mpcExecutorPairNetworkCfgPath, //  which is network file path
			algName,
		)
		cmd.Dir = thirdMPSPDZPath
	}

	if wk.DslObj.DistributedTask.Enable == 0 {
		cmd = exec.Command(
			common.MpcExePath,
			"-F",
			"-N", fmt.Sprintf("%d", partyNum),
			"-p", fmt.Sprintf("%d", partyId),
			"-I",
			"-ip", mpcExecutorPairNetworkCfgPath, //  which is network file path
			algName,
		)
		cmd.Dir = thirdMPSPDZPath
	}

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	// by default, worker will launch mpc resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.MpcTm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	}
}

func (wk *TrainWorker) TestTaskCallee() {

	logFile := common.TaskRuntimeLogs + "-" + wk.DslObj.Tasks.ModelTraining.AlgorithmName
	// 3. generate command line
	var cmd *exec.Cmd
	// in distributed training situation and this worker is parameter server
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedParameterServer {

		logger.Log.Println("[PartyServer]: training method: distributed",
			"current distributed role: ", common.DistributedParameterServer)

		psLogFile := logFile + "/parameter_server"
		ee := os.MkdirAll(psLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating parameter server folder error", ee)
		}
		cmd = exec.Command("python3",
			"-u",
			"/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/examples/models/preprocessing.py",
			"-a="+psLogFile,
			"-b=2",
			"-c=123")
	}

	// in distributed training situation and this worker is train worker
	if wk.DslObj.DistributedTask.Enable == 1 && wk.DistributedRole == common.DistributedWorker {

		logger.Log.Println("[PartyServer]: training method: distributed",
			"current distributed role: ", common.DistributedWorker)

		wkLogFile := logFile + "/distributed-worker-" + fmt.Sprintf("%d", wk.WorkerID)
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command("python3",
			"-u",
			"/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/examples/models/preprocessing.py",
			"-a="+wkLogFile,
			"-b=2",
			"-c=123")
	}

	// in centralized training
	if wk.DslObj.DistributedTask.Enable == 0 {

		logger.Log.Println("[PartyServer]: training method: centralized")

		wkLogFile := logFile + "/worker"
		ee := os.MkdirAll(wkLogFile, os.ModePerm)
		if ee != nil {
			logger.Log.Fatalln("[PartyServer]: Creating distributed worker folder error", ee)
		}

		cmd = exec.Command("python3",
			"-u",
			"/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/examples/models/preprocessing.py",
			"-a="+wkLogFile,
			"-b=2",
			"-c=123")
	}

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task model training start")
	wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)

}

func (wk *TrainWorker) TestMPCCallee(TaskName string) {
	cmd := exec.Command("python3", "-u", "/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/examples/models/preprocessing.py", "-a="+TaskName, "-b=2", "-c=123")
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	wk.MpcTm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
}
