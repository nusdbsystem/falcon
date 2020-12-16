package worker

import (
	"coordinator/common"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"encoding/json"
	"fmt"
	"os/exec"
	"sync"
	"time"
)


func(wk *TrainWorker) TrainTask(dta *entitiy.DoTaskArgs, rep *entitiy.DoTaskReply){

	wg := sync.WaitGroup{}

	wg.Add(2)

	go wk.MpcTaskProcess(dta, "algName")
	go wk.MlTaskProcess(dta, rep, &wg)

	// wait until all the task done
	wg.Wait()

	// kill all the monitors
	wk.Pm.Cancel()

	wk.Pm.Lock()
	rep.Killed = wk.Pm.IsKilled
	if wk.Pm.IsKilled == true{
		wk.Pm.Unlock()
		wk.TaskFinish <- true
	}else{
		wk.Pm.Unlock()
	}
}

func (wk *TrainWorker) MlTaskProcess(dta *entitiy.DoTaskArgs, rep *entitiy.DoTaskReply, wg *sync.WaitGroup)  {
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
	defer wg.Done()

	logger.Do.Printf("Worker: %s task started \n", wk.Url)


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

	var algParams string
	var KeyFile string

	doTrain := doMlTask(
		wk.Pm,
		fmt.Sprintf("%d", partyId),
		fmt.Sprintf("%d", partyNum),
		fmt.Sprintf("%d", partyType),
		flSetting,
		fmt.Sprintf("%d", existingKey),
		dta.NetWorkFile,
	)

	var exitStr string
	var res map[string]string
	var logFile string
	fmt.Println(res)

	if  dta.TaskInfos.PreProcessing.AlgorithmName!=""{
		logger.Do.Println("Worker:task 1 pre processing start")
		logFile = common.TaskRuntimeLogs + "/" + dta.TaskInfos.PreProcessing.AlgorithmName
		KeyFile = dta.TaskInfos.PreProcessing.InputConfigs.DataInput.Key

		algParams = dta.TaskInfos.PreProcessing.InputConfigs.SerializedAlgorithmConfig
		exitStr, res = doTrain(
			dta.TaskInfos.PreProcessing.AlgorithmName,
			algParams,
			KeyFile,
			logFile,
			dataInputFile,
			dataOutFile,
			"", "",
		)
		if exit := wk.execResHandler(exitStr, res, rep); exit==true{
			return
		}
		logger.Do.Println("Worker:task 1 pre processing done", rep)
	}

	if  dta.TaskInfos.ModelTraining.AlgorithmName!="" {
		// execute task 2: train
		logger.Do.Println("Worker:task model training start")
		logFile = common.TaskRuntimeLogs + "/" + dta.TaskInfos.ModelTraining.AlgorithmName
		KeyFile = dta.TaskInfos.ModelTraining.InputConfigs.DataInput.Key

		algParams = dta.TaskInfos.ModelTraining.InputConfigs.SerializedAlgorithmConfig
		exitStr, res = doTrain(
			dta.TaskInfos.ModelTraining.AlgorithmName,
			algParams,
			KeyFile,
			logFile,
			"",
			modelInputFile,
			modelFile,
			modelReportFile,
		)

		if exit := wk.execResHandler(exitStr, res, rep); exit==true{
			return
		}
		logger.Do.Println("Worker:task model training", rep)
	}

	// 2 thread will ready from isStop channel, only one is running at the any time

	for i := 10; i > 0; i-- {
		logger.Do.Println("Worker: Counting down before job done... ", i)

		time.Sleep(time.Second)
	}

	logger.Do.Printf("Worker: %s: task done\n", wk.Url)
}

func (wk *TrainWorker) MpcTaskProcess(dta *entitiy.DoTaskArgs, algName string){
	/**
	 * @Author
	 * @Description
	 * @Date 2:52 下午 12/12/20
	 * @Param

		./semi-party.x -F -N 3 -I -p 0 -h 10.0.0.33 -pn 6000 algorithm_name
			-N party_num
			-p party_id
			-h ip
			-pn port
			algorithm_name

	 * @return
	 **/

	partyId := dta.AssignID
	partyNum := dta.PartyNums

	var envs []string

	cmd := exec.Command(
		common.MpcExe,
		" --F ",
		" -N  " + fmt.Sprintf("%d",partyNum),
		" --I ",
		" --p " + fmt.Sprintf("%d",partyId),
		" --h " + dta.IP,
		" --pn " + wk.Port,
		" "+algName,
		)

	wk.Pm.CreateResources(cmd, envs)

	return

}

func (wk *TrainWorker) execResHandler(
	exitStr string,
	RuntimeErrorMsg map[string]string,
	rep *entitiy.DoTaskReply) bool{
	/**
	 * @Author
	 * @Description
	 * @Date 3:55 下午 14/12/20
	 * @Param
			true: has error, exit
			false: no error, keep running
	 * @return
	 **/
	js, err := json.Marshal(RuntimeErrorMsg)
	if err !=nil{
		logger.Do.Println("Worker: Serialize job status error", err)
		return true
	}
	rep.TaskMsg.RuntimeMsg = string(js)

	if exitStr != common.SubProcessNormal {
		rep.RuntimeError = true
		// return is used to control the rpc call status, always return nil, but
		// keep error at rep.ErrorMsg
		return true
	}

	rep.RuntimeError = false
	return false
}

func TestTaskProcess(arg *entitiy.DoTaskArgs){

	logger.Do.Println(arg)
	time.Sleep(time.Minute)
}

func doMlTask(
	pm *taskmanager.SubProcessManager,

	partyId string,
	partyNum string,
	partyType string,
	flSetting string,
	existingKey string,

	netFile string,

) func(string, string, string,string, string,string, string,string) (string, map[string]string ) {

	/**
	 * @Author
	 * @Description  record if the task is fail or not
	 * @Date 7:32 下午 12/12/20
	 * @Param key, algorithm name, value: error message
	 * @return
	 **/

	// Closure
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
	)(string, map[string]string ){

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

		exitStr, runTimeErrorLog := pm.CreateResources(cmd, envs)

		res[algName] = runTimeErrorLog
		return exitStr, res
	}
}
