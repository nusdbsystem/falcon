package worker

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/base"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"falcon_platform/utils"
	"net/rpc"
)

type TrainWorker struct {
	base.WorkerBase
}

func InitTrainWorker(masterAddr, workerAddr string, PartyID string) *TrainWorker {

	wk := TrainWorker{}
	wk.InitWorkerBase(workerAddr, common.TrainWorker)
	wk.MasterAddr = masterAddr
	wk.WorkerID = PartyID

	return &wk
}

func (wk *TrainWorker) Run() {

	// 0 thread: start event Loop
	go wk.HeartBeatLoop()

	rpcSvc := rpc.NewServer()

	err := rpcSvc.Register(wk)
	if err != nil {
		logger.Log.Fatalf("%s: start Error \n", wk.Name)
	}

	logger.Log.Printf("%s from PartyID %s to register with masterAddr(%s)\n", wk.Name, wk.WorkerID, wk.MasterAddr)
	wk.Register(wk.MasterAddr)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)

}

func (wk *TrainWorker) DoTask(taskName string, rep *entity.DoTaskReply) error {
	// 1. decode args
	logger.Log.Println("[TrainWorker] TrainWorker.DoTask called, taskName: ", taskName)
	defer func() {
		logger.Log.Printf("[TrainWorker]: %s: task %s done\n", wk.Addr, taskName)
	}()

	// 2. init task manager for this task
	wk.Tm = resourcemanager.InitResourceManager()
	defer func() {
		// once all tasks finish, clear resources monitors,
		wk.Tm.ResourceClear()
		// clear wk.Tm
		wk.Tm = resourcemanager.InitResourceManager()
	}()

	update := func() {
		// update task error msg
		rep.TaskMsg.RuntimeMsg = wk.Tm.RunTimeErrorLog
		// update task status
		wk.Tm.Mux.Lock()
		if wk.Tm.TaskStatus == common.TaskFailed {
			rep.RuntimeError = true
		} else {
			rep.RuntimeError = false
		}
		wk.Tm.Mux.Unlock()
	}

	if taskName == common.PreProcSubTask {
		//wk.TestTaskCallee(taskName)// for testing/debug, dont remove,
		wk.RunPreProcessingTask()
		// update task error msg
		update()
		return nil
	}
	if taskName == common.ModelTrainSubTask {
		//wk.TestTaskCallee(taskName)// for testing/debug, dont remove,
		wk.RunModelTrainingTask()
		// update task error msg
		update()
		return nil
	}

	rep.RuntimeError = true
	rep.TaskMsg.RuntimeMsg = "TaskName wrong! must be in <pre_processing, model_training>"

	return nil
}

func (wk *TrainWorker) RunMpc(mpcAlgorithmName string, rep *entity.DoTaskReply) error {

	// 1. decode args
	logger.Log.Println("[TrainWorker] TrainWorker.RunMpc called, mpcAlgorithmName: ", mpcAlgorithmName)
	defer func() {
		logger.Log.Printf("[TrainWorker]: %s: task %s done\n", wk.Addr, mpcAlgorithmName)
	}()

	update := func() {
		// update task error msg
		rep.TaskMsg.RuntimeMsg = wk.MpcTm.RunTimeErrorLog
		// update task status
		wk.MpcTm.Mux.Lock()
		if wk.MpcTm.TaskStatus == common.TaskFailed {
			rep.RuntimeError = true
		} else {
			rep.RuntimeError = false
		}
		wk.MpcTm.Mux.Unlock()
	}

	// clear the existing mpc resources if exist
	wk.MpcTm.ResourceClear()
	// init new resource manager
	wk.MpcTm = resourcemanager.InitResourceManager()
	// as for mpc, don't record it;s final status, as longs as it running, return as successful
	wk.RunMpcTask(mpcAlgorithmName)
	//wk.TestMPCCallee(mpcAlgorithmName)// for testing/debug, dont remove,

	update()
	return nil
}

func (w *TrainWorker) RetrieveModelReport(_ string, rep *entity.RetrieveModelReportReply) error {

	workerActiveType := w.DslObj.PartyInfo.PartyType

	if workerActiveType == common.ActiveParty {
		rep.RuntimeError = false
		rep.ContainsModelReport = true

		// read local file and send back
		//modelReportFile := common.TaskModelPath + "/" + w.DslObj.Tasks.ModelTraining.OutputConfigs.EvaluationReport
		modelReportFile := "/Users/nailixing/GOProj/src/github.com/scripts/report"
		logger.Log.Println("[TrainWorker] TrainWorker.RetrieveModelReport called, read file: ", modelReportFile)
		reportStr, err := utils.ReadFile(modelReportFile)
		if err != nil {
			rep.RuntimeError = true
			rep.TaskMsg.RuntimeMsg = err.Error()
		}
		rep.ModelReport = reportStr
	} else if workerActiveType == common.PassiveParty {
		rep.RuntimeError = false
		rep.ContainsModelReport = false
	} else {
		rep.RuntimeError = true
		rep.TaskMsg.RuntimeMsg = "PartyType not recognized"
	}
	logger.Log.Println("[TrainWorker] :RetrieveModelReport finished")
	return nil
}
