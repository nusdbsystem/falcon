package worker

import (
	"coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"fmt"
	"os/exec"
	"time"
)


func DoMPCTask(){

}


func DoMlTask(
	pm *taskmanager.SubProcessManager,

	partyId string,
	partyNum string,
	partyType string,
	flSetting string,
	existingKey string,

	netFile string,

) func(string, string, string,string, string,string, string,string) (string, bool, map[string]string ) {

	/**
	 * @Author
	 * @Description  record if the task is fail or not
	 * @Date 7:32 下午 12/12/20
	 * @Param key, algorithm name, value: error message
	 * @return
	 **/

	res := make(map[string]string)

	return func(
		algName string,
		algParams string,

		KeyFile string,
		logFile string,
		dataInputFile string,
		dataOutputFile string,
		modelSaveFile string,
		modelReport string,
	)(string, bool, map[string]string ){

		var envs []string

		cmd := exec.Command(
			common.FalconTrainExe,
			" --party-id "+partyId,
			" --party-num "+partyNum,
			" --party-type "+partyType,
			" --fl-setting "+flSetting,
			" --existing-key "+existingKey,
			" --key-file "+KeyFile,
			" --network-file "+netFile,

			" --algorithm-name "+algName,
			" --algorithm-params "+algParams,
			" --log-file "+logFile,
			" --data-input-file "+dataInputFile,
			" --data-output-file "+dataOutputFile,
			" --model-save-file "+modelSaveFile,
			" --model-report-file "+modelReport,
		)

		killed, err, el := pm.CreateResources(cmd, envs)

		res[algName] = el
		return err, killed, res
	}
}


func (wk *Worker) MlTaskProcess(arg []byte, rep *entitiy.DoTaskReply)  {
	/**
	 * @Author
	 * @Description
	 * @Date 2:52 下午 12/12/20
	 * @Param
		requires:

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

	 * @return
	 **/

	logger.Do.Printf("Worker: %s task started \n", wk.Address)

	var dta *entitiy.DoTaskArgs = entitiy.DecodeDoTaskArgs(arg)

	partyId := dta.AssignID
	partyNum := dta.PartyNums
	partyType := dta.PartyInfo.PartyType
	flSetting := dta.JobFlType
	existingKey := dta.ExistingKey

	dataInputFile := common.TaskDataPath +"/" + dta.TaskInfos.PreProcessing.InputConfigs.DataInput.Data
	dataOutFile := common.TaskDataOutput  +"/" + dta.TaskInfos.PreProcessing.OutputConfigs.DataOutput
	modelInputFile := common.TaskDataOutput +"/"+ dta.TaskInfos.ModelTraining.InputConfigs.DataInput.Data
	modelFile := common.TaskModelPath +"/"+ dta.TaskInfos.ModelTraining.OutputConfigs.TrainedModel

	modelReportFile := dta.TaskInfos.ModelTraining.OutputConfigs.EvaluationReport



	time.Sleep(time.Second*1)
	var algParams string
	var KeyFile string

	rep.Errs = make(map[string]string)
	rep.OutLogs = make(map[string]string)

	doTrain := DoMlTask(
		wk.pm,
		fmt.Sprintf("%d", partyId),
		fmt.Sprintf("%d", partyNum),
		fmt.Sprintf("%d", partyType),
		flSetting,
		fmt.Sprintf("%d", existingKey),
		dta.NetWorkFile,
	)

	var isOk string
	var killed bool
	var res map[string]string
	var logFile string


	if  dta.TaskInfos.PreProcessing.AlgorithmName!=""{
		logger.Do.Println("Worker:task 1 pre processing start")
		logFile = common.TaskRuntimeLogs + "/" + dta.TaskInfos.PreProcessing.AlgorithmName
		KeyFile = dta.TaskInfos.PreProcessing.InputConfigs.DataInput.Key

		algParams = dta.TaskInfos.PreProcessing.InputConfigs.SerializedAlgorithmConfig
		isOk, killed, res = doTrain(
			dta.TaskInfos.PreProcessing.AlgorithmName,
			algParams,
			KeyFile,
			logFile,
			dataInputFile,
			dataOutFile,
			"", "",
		)
		rep.ErrLogs = res
		if isOk != common.SubProcessNormal {
			// return res is used to control the rpc call status, always return nil, but
			// keep error at rep.Errs
			return
		}

		rep.Killed = killed
		if killed {
			wk.taskFinish <- true
			return
		}
		logger.Do.Println("Worker:task 1 pre processing done", killed, isOk)
	}

	if  dta.TaskInfos.ModelTraining.AlgorithmName!="" {
		// execute task 2: train
		logger.Do.Println("Worker:task model training start")
		logFile = common.TaskRuntimeLogs + "/" + dta.TaskInfos.ModelTraining.AlgorithmName
		KeyFile = dta.TaskInfos.ModelTraining.InputConfigs.DataInput.Key

		algParams = dta.TaskInfos.ModelTraining.InputConfigs.SerializedAlgorithmConfig
		isOk, killed, res = doTrain(
			dta.TaskInfos.ModelTraining.AlgorithmName,
			algParams,
			KeyFile,
			logFile,
			"",
			modelInputFile,
			modelFile,
			modelReportFile,
		)
		rep.ErrLogs = res
		if isOk != common.SubProcessNormal {
			return
		}

		rep.Killed = killed
		if killed {
			wk.taskFinish <- true
		}
		logger.Do.Println("Worker:task model training", killed, isOk)
	}

	// 2 thread will ready from isStop channel, only one is running at the any time

	for i := 10; i > 0; i-- {
		logger.Do.Println("Worker: Counting down before job done... ", i)

		time.Sleep(time.Second)
	}

	logger.Do.Printf("Worker: %s: task done\n", wk.Address)
}

func TestTaskProcess(){

}
