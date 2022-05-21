package worker

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/jobmanager/tasks"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"net/rpc"
	"time"
)

type TrainWorker struct {
	WorkerBase
}

//InitTrainWorker
// * @Description
// * @Date 下午4:38 22/08/21
// * @Param
//	masterAddr: master ip+port, worker send message to master
//	workerAddr: current worker address, worker register it to master, receive master's call，
//	PartyID: id of the party which launch this worker
//	workerID： id of the worker, mainly used to distinguish workers in distributed training
//	distributedRole： 0: ps, 1: worker
// * @return

func InitTrainWorker(masterAddr, workerAddr string,
	PartyID common.PartyIdType, WorkerID common.WorkerIdType) *TrainWorker {

	wk := TrainWorker{}
	wk.InitWorkerBase(workerAddr, common.TrainWorker)
	wk.MasterAddr = masterAddr
	wk.PartyID = PartyID
	wk.WorkerID = WorkerID

	logger.Log.Printf("[TrainWorker] Worker initialized, WorkerName=%s, WorkerID=%d, PartyID=%d, \n",
		wk.Name, wk.WorkerID, wk.PartyID)

	return &wk
}

// Run worker
func (wk *TrainWorker) Run() {

	// 0 thread: start event Loop
	go func() {
		defer logger.HandleErrors()
		wk.HeartBeatLoop()
	}()

	rpcSvc := rpc.NewServer()

	err := rpcSvc.Register(wk)
	if err != nil {
		logger.Log.Fatalf("%s: start Error \n", wk.Name)
	}

	logger.Log.Printf("[%s] from PartyID %d to register to masterAddr(%s)\n", wk.Name, wk.PartyID, wk.MasterAddr)
	wk.RegisterToMaster(wk.MasterAddr)

	// start rpc server blocking...
	wk.StartRPCServer(rpcSvc, true)
}

//DoTask receive master call
func (wk *TrainWorker) DoTask(args string, rep *entity.DoTaskReply) error {
	// 1. decode args

	taskArg, err := entity.DeserializeTask([]byte(args))
	if err != nil {
		panic("Parser doTask args fails")
	}
	var taskName common.FalconTask = taskArg.TaskName

	logger.Log.Println("[TrainWorker] TrainWorker.DoTask called, taskName:", taskName)
	defer func() {
		logger.Log.Printf("[TrainWorker]: WorkerAddr %s, TrainWorker.DoTask Done for tasks %s \n", wk.Addr, taskName)
	}()

	// 2. init tasks manager for this tasks
	wk.Tm = resourcemanager.InitResourceManager()
	defer func() {
		// once all tasks finish, DeleteResources
		wk.Tm.DeleteResources()
		// clear wk.Tm
		wk.Tm = resourcemanager.InitResourceManager()
	}()

	update := func() {
		// update tasks error msg
		rep.TaskMsg.RuntimeMsg = wk.Tm.RunTimeErrorLog
		// update tasks status
		wk.Tm.Mux.Lock()
		if wk.Tm.TaskStatus == common.TaskFailed {
			rep.RuntimeError = true
		} else {
			rep.RuntimeError = false
		}
		wk.Tm.Mux.Unlock()
	}

	// if the task is registered,
	if task, ok := tasks.GetAllTasks()[taskName]; ok {
		wk.runTask(task, taskArg)
		update()
		return nil
	} else {
		rep.RuntimeError = true
		rep.TaskMsg.RuntimeMsg = "TaskName wrong! only support <pre_processing, model_training," +
			" lime_pred_task, lime_weight_task, lime_feature_task, lime_interpret_task>"
		panic("tasks name not found error, taskName=" + taskName)
		return nil
	}
}

// runTask execute a given tasks .
func (wk *TrainWorker) runTask(task tasks.Task, taskArg *entity.TaskContext) {

	cmd := task.GetCommand(taskArg)
	// by default, worker will launch executor resource by sub process
	if common.IsDebug != common.DebugOn {
		wk.Tm.CreateResources(resourcemanager.InitSubProcessManager(), cmd)
	} else {
		time.Sleep(10 * time.Minute)
	}
}

func (wk *TrainWorker) RunMpc(args string, rep *entity.DoTaskReply) error {

	taskArg, err := entity.DeserializeTask([]byte(args))
	if err != nil {
		panic("Parser doTask args fails")
	}
	var taskName common.FalconTask = taskArg.TaskName
	if taskName != common.MpcTaskKey {
		panic("MPC task, but taskName not equal MpcTaskKey")
	}
	// 1. decode args
	logger.Log.Println("[TrainWorker] TrainWorker.RunMpc called, mpcAlgorithmName:", taskArg.MpcAlgName)
	defer func() {
		logger.Log.Printf("[TrainWorker]: WorkerAddr %s, TrainWorker.RunMpc for tasks %s done\n", wk.Addr, taskArg.MpcAlgName)
	}()

	update := func() {
		// update tasks error msg
		rep.TaskMsg.RuntimeMsg = wk.MpcTm.RunTimeErrorLog
		// update tasks status
		wk.MpcTm.Mux.Lock()
		if wk.MpcTm.TaskStatus == common.TaskFailed {
			rep.RuntimeError = true
		} else {
			rep.RuntimeError = false
		}
		wk.MpcTm.Mux.Unlock()
	}

	task := tasks.GetAllTasks()[taskName]

	// clear the existing mpc resources if exist
	wk.MpcTm.DeleteResources()
	// init new resource manager
	wk.MpcTm = resourcemanager.InitResourceManager()
	// as for mpc, don't record it;s final status, as longs as it running, return as successful

	// no need to wait for mpc, once train tasks done, shutdown the mpc
	logger.Log.Println("[TrainWorker]: spawn thread for mpc")
	go func() {
		defer logger.HandleErrors()
		wk.runTask(task, taskArg)
	}()
	// wait 4 seconds until starting ./semi-party.x return tasks-running or tasks-failed
	time.Sleep(time.Second * 5)

	update()
	return nil
}

func (w *TrainWorker) RetrieveModelReport(_ string, rep *entity.RetrieveModelReportReply) error {
	rep.RuntimeError = false
	rep.ContainsModelReport = false
	return nil
}
