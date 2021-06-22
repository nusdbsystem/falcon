package worker

import (
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"os"
	"os/exec"
	"strings"
	"time"
)

func (wk *TrainWorker) RunMpcTask(algorithmName string) {
	// no need to wait for mpc, once train task done, shutdown the mpc
	logger.Log.Println("[TrainWorker]: spawn thread for mpc")
	go wk.mpc(algorithmName)
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
			("data-input-file", po::value<std::string>(&data_file), "file name of dataset")
			("data-output-file", po::value<std::string>(&data_file), "file name of dataset")
			("model-save-file", po::value<std::string>(&data_file), "file name of dataset")
			("key-file", po::value<std::string>(&key_file), "file name of phe keys")

			("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
			("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
			("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm");
	 **/

	logger.Log.Println("[TrainWorker]: pre processing task called")

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
	logFile := common.TaskRuntimeLogs + "/" + wk.DslObj.Tasks.PreProcessing.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.PreProcessing.InputConfigs.DataInput.Key

	_ = os.MkdirAll(logFile, os.ModePerm)

	// 3. generate command line
	// todo, check with wyc how to call pre-process, is model-save-file and model-report needed?
	cmd := exec.Command(
		common.FLEnginePath,
		"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
		"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
		"--party-type", fmt.Sprintf("%d", partyType),
		"--fl-setting", fmt.Sprintf("%d", flSetting),
		"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
		"--key-file", KeyFile,
		"--network-file", wk.DslObj.NetWorkFile,

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
	wk.Tm.CreateResources(cmd)
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
			("data-input-file", po::value<std::string>(&data_file), "file name of dataset")
			("data-output-file", po::value<std::string>(&data_file), "file name of dataset")
			("model-save-file", po::value<std::string>(&data_file), "file name of dataset")
			("key-file", po::value<std::string>(&key_file), "file name of phe keys")

			("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
			("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
			("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm");
	 **/

	logger.Log.Println("[TrainWorker]: model training task called")

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
	logFile := common.TaskRuntimeLogs + "/" + wk.DslObj.Tasks.ModelTraining.AlgorithmName
	KeyFile := common.TaskDataPath + "/" + wk.DslObj.Tasks.ModelTraining.InputConfigs.DataInput.Key
	modelReportFile := common.TaskModelPath + "/" + wk.DslObj.Tasks.ModelTraining.OutputConfigs.EvaluationReport

	_ = os.MkdirAll(logFile, os.ModePerm)

	// 3. generate command line
	cmd := exec.Command(
		common.FLEnginePath,
		"--party-id", fmt.Sprintf("%d", wk.DslObj.PartyInfo.ID),
		"--party-num", fmt.Sprintf("%d", wk.DslObj.PartyNums),
		"--party-type", fmt.Sprintf("%d", partyType),
		"--fl-setting", fmt.Sprintf("%d", flSetting),
		"--existing-key", fmt.Sprintf("%d", wk.DslObj.ExistingKey),
		"--key-file", KeyFile,
		"--network-file", wk.DslObj.NetWorkFile,

		"--algorithm-name", wk.DslObj.Tasks.ModelTraining.AlgorithmName,
		"--algorithm-params", wk.DslObj.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig,
		"--log-file", logFile,
		"--data-input-file", modelInputFile,
		"--data-output-file", "",
		"--model-save-file", modelFile,
		"--model-report-file", modelReportFile,
	)

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	// 4. execute cmd
	logger.Log.Println("[TrainWorker]: task model training start")
	wk.Tm.CreateResources(cmd)
}

func (wk *TrainWorker) mpc(algName string) {
	/**
	 * @Author
	 * @Description
	 * @Date 2:52 下午 12/12/20
	 * @Param

		./semi-party.x -F -N 3 -p 0 -I -h 10.0.0.33 -pn 6000 algorithm_name
			-N party_num
			-p party_id
			-h IP
			-pn port
			algorithm_name
		-h is IP of part-0, all semi-party use the same port
		-h each mpc process only requires IP of party-0

		-h 是party_0的IP 端口目前只有一个 各个端口都相同就可以
		-h 每个mpc进程的启动输入都是party_0的IP
	 * @return
	 **/

	logger.Log.Println("[TrainWorker]: Task mpc called with algName ", algName)
	// TODO: make PartyID all the same type, currently it is sometimes uint sometimes string!
	partyId := wk.DslObj.PartyInfo.ID
	partyNum := wk.DslObj.PartyNums

	tmpThirdPathList := strings.Split(common.MpcExePath, "/")
	thirdMPSPDZPath := strings.Join(tmpThirdPathList[:len(tmpThirdPathList)-1], "/")
	logger.Log.Printf("[TrainWorker]: Third Party base path for executing semi-party.x is : \"%s\"\n", thirdMPSPDZPath)

	cmd := exec.Command(
		common.MpcExePath,
		"-F",
		"-N", fmt.Sprintf("%d", partyNum),
		"-p", fmt.Sprintf("%d", partyId),
		"-I",
		"-h", wk.DslObj.MpcIP,
		"-pn", fmt.Sprintf("%d", 5000), // fmt.Sprintf("%d", wk.DslObj.MpcPort),
		algName,
	)
	cmd.Dir = thirdMPSPDZPath

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	wk.MpcTm.CreateResources(cmd)
}

func (wk *TrainWorker) TestTaskCallee(TaskName string) {
	cmd := exec.Command("python3", "-u", "/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/examples/models/preprocessing.py", "-a="+TaskName, "-b=2", "-c=123")
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	wk.Tm.CreateResources(cmd)
}

func (wk *TrainWorker) TestMPCCallee(TaskName string) {
	cmd := exec.Command("python3", "-u", "/Users/nailixing/GOProj/src/github.com/falcon/src/falcon_platform/examples/models/preprocessing.py", "-a="+TaskName, "-b=2", "-c=123")
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	wk.MpcTm.CreateResources(cmd)
}
