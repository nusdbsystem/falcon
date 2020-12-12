package worker

import (
	"coordinator/client"
	"coordinator/common"
	_ "coordinator/common"
	"coordinator/distributed/base"
	"coordinator/distributed/entitiy"
	"coordinator/distributed/taskmanager"
	"coordinator/logger"
	"fmt"
	"time"
)

type Worker struct {

	base.RpcBase

	pm         *taskmanager.SubProcessManager
	taskFinish chan bool

	latestHeardTime int64
	SuicideTimeout  int

}

func (wk *Worker) DoTask(arg []byte, rep *entitiy.DoTaskReply) error {

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

	//var dataAlgorCfg map[string]interface{}
	//var modelAlgorCfg map[string]interface{}

	time.Sleep(time.Second*1)
	var KeyFile string

	rep.Errs = make(map[string]string)
	rep.OutLogs = make(map[string]string)

	logger.Do.Println("Worker:task 1 pre processing start")

	doTrain := taskmanager.DoMlTask(
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

	logFile = common.TaskRuntimeLogs + "/" + dta.TaskInfos.PreProcessing.AlgorithmName
	KeyFile = dta.TaskInfos.PreProcessing.InputConfigs.DataInput.Key
	isOk, killed, res = doTrain(
		dta.TaskInfos.PreProcessing.AlgorithmName,
		"algParams",
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
		return nil
	}

	rep.Killed = killed
	if killed {
		wk.taskFinish <- true
		return nil
	}
	logger.Do.Println("Worker:task 1 pre processing done", killed, isOk)

	// execute task 2: train
	logger.Do.Println("Worker:task model training start")
	logFile = common.TaskRuntimeLogs + "/" + dta.TaskInfos.ModelTraining.AlgorithmName
	KeyFile = dta.TaskInfos.ModelTraining.InputConfigs.DataInput.Key
	isOk, killed, res = doTrain(
		dta.TaskInfos.ModelTraining.AlgorithmName,
		"algParams",
		KeyFile,
		logFile,
		"",
		modelInputFile,
		modelFile,
		modelReportFile,
	)
	rep.ErrLogs = res
	if isOk != common.SubProcessNormal {
		return nil
	}

	rep.Killed = killed
	if killed {
		wk.taskFinish <- true
		return nil
	}

	logger.Do.Println("Worker:task model training", killed, isOk)

	// 2 thread will ready from isStop channel, only one is running at the any time

	for i := 10; i > 0; i-- {
		logger.Do.Println("Worker: Counting down before job done... ", i)

		time.Sleep(time.Second)
	}

	logger.Do.Printf("Worker: %s: task done\n", wk.Address)

	return nil
}

// call the master's register method,
func (wk *Worker) register(master string) {
	args := new(entitiy.RegisterArgs)
	args.WorkerAddr = wk.Address

	logger.Do.Printf("Worker: begin to call Master.Register to register address= %s \n", args.WorkerAddr)
	ok := client.Call(master, wk.Proxy, "Master.Register", args, new(struct{}))
	if ok == false {
		logger.Do.Printf("Worker: Register RPC %s, register error\n", master)
	}
}

// Shutdown is called by the master when all work has been completed.
// We should respond with the number of tasks we have processed.
func (wk *Worker) Shutdown(_ *struct{}, res *entitiy.ShutdownReply) error {
	logger.Do.Println("Worker: Shutdown", wk.Address)

	// there are running subprocess
	wk.pm.Lock()

	// shutdown other related thread
	wk.IsStop = true

	if wk.pm.NumProc > 0 {
		wk.pm.Unlock()

		wk.pm.IsStop <- true

		logger.Do.Println("Worker: Waiting to finish DoTask...", wk.pm.NumProc)
		<-wk.taskFinish
		logger.Do.Println("Worker: DoTask returned, Close the listener...")
	} else {
		wk.pm.Unlock()

	}

	err := wk.Listener.Close() // close the connection, cause error, and then ,break the worker
	if err != nil {
		logger.Do.Println("Worker: worker close error, ", err)
	}
	// this is used to define shut down the worker servers

	return nil
}

func (wk *Worker) eventLoop() {
	time.Sleep(time.Second * 5)
	for {

		wk.Lock()
		if wk.IsStop == true {
			logger.Do.Printf("Worker: isStop=true, server %s quite eventLoop \n", wk.Address)

			wk.Unlock()
			break
		}

		elapseTime := time.Now().UnixNano() - wk.latestHeardTime
		if int(elapseTime/int64(time.Millisecond)) >= wk.SuicideTimeout {

			logger.Do.Printf("Worker: Timeout, server %s begin to suicide \n", wk.Address)

			var reply entitiy.ShutdownReply
			ok := client.Call(wk.Address, wk.Proxy, "Worker.Shutdown", new(struct{}), &reply)
			if ok == false {
				logger.Do.Printf("Worker: RPC %s shutdown error\n", wk.Address)
			} else {
				logger.Do.Printf("Worker: worker timeout, RPC %s shutdown successfule\n", wk.Address)
			}
			// quite event loop no matter ok or not
			break
		}
		wk.Unlock()

		time.Sleep(time.Millisecond * common.WorkerTimeout / 5)
		fmt.Printf("Worker: CountDown:....... %d \n", int(elapseTime/int64(time.Millisecond)))
	}
}

func (wk *Worker) ResetTime(_ *struct{}, _ *struct{}) error {
	fmt.Println("Worker: reset the countdown")
	wk.reset()
	return nil
}

func (wk *Worker) reset() {
	wk.Lock()
	wk.latestHeardTime = time.Now().UnixNano()
	wk.Unlock()

}
