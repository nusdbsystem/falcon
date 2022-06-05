package test

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/DAGscheduler"
	"falcon_platform/jobmanager/comms_pattern"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/jobmanager/tasks"
	"falcon_platform/logger"
	"falcon_platform/resourcemanager"
	"fmt"
	"gotest.tools/assert"
	"io/ioutil"
	"os"
	"reflect"
	"testing"
)

func mockConstructNtCfg(ExecutorExecutorPort []common.PortType, MpcMpcPort common.PortType, workerId uint, taskClassID string) []byte {
	newCfg := new(comms_pattern.FLNetworkCfgPerParty)
	newCfg.ExecutorExecutorPort = ExecutorExecutorPort
	newCfg.MpcMpcPort = MpcMpcPort
	newCfg.MpcExecutorPort = resourcemanager.GetMpcExecutorPort(int(workerId), taskClassID)
	newCfg.ExecutorPSPort = 0
	newCfg.PsExecutorPorts = []common.PortType{}
	newCfg.DistributedRole = common.CentralizedWorker
	return comms_pattern.EncodeFLNetworkCfgPerParty(newCfg)
}

func testParseTrainJob(dslFile string, jobID int) common.TrainJob {
	var job common.TrainJob
	jsonFile, err := os.Open(dslFile)
	if err != nil {
		panic(err)
	}
	byteValue, err := ioutil.ReadAll(jsonFile)
	if err != nil {
		panic(err)
	}
	err = common.ParseTrainJob(string(byteValue), &job)
	if err != nil {
		panic(err)
	}
	job.JobId = uint(jobID)
	job.PartyAddrList = common.ParseAddress(job.PartyInfoList)
	fmt.Println(job)
	return job
}

func testSchedule(job common.TrainJob) *DAGscheduler.DagScheduler {
	dagScheduler := DAGscheduler.NewDagScheduler(&job)
	if dagScheduler.ParallelismPolicy.IsValid == false {
		panic("Error")
	}
	dagScheduler.SplitTaskIntoStage(&job)
	return dagScheduler
}

func testStageInit(stage DAGscheduler.TaskStage, job common.TrainJob, tf *testCfg) comms_pattern.JobNetworkConfig {

	taskClassID := string(stage.Name) + "-" + fmt.Sprintf("%d", 0)
	assignedWorker := stage.AssignedWorker

	// update distributed information
	if assignedWorker == 1 {
		// if assignedWorker = 1 disable distributed
		job.DistributedTask.Enable = 0
	} else {
		// if assignedWorker > 1 disable distributed
		job.DistributedTask.Enable = 1
		job.DistributedTask.WorkerNumber = assignedWorker
	}

	var requiredResource = make([][]byte, len(job.PartyAddrList))

	// env for party server
	common.PartyServerClusterIPs = job.PartyAddrList

	for partyIndex, _ := range job.PartyAddrList {

		common.TaskDataPath = common.DataPathContainer
		common.TaskDataOutput = common.DataOutputPathContainer
		common.TaskModelPath = common.ModelPathContainer

		workerId := job.JobId
		resIns := new(comms_pattern.PartyRunWorkerReply)
		resIns.PartyID = common.PartyID
		resIns.ResourceNum = 1
		resIns.ResourceSVCs = make(map[common.WorkerIdType]*comms_pattern.ResourceSVC)
		nodeID := int(workerId) % len(job.PartyAddrList)
		nodeIP := common.PartyServerClusterIPs[nodeID]
		// init resourceSVC information
		resourceSVC := new(comms_pattern.ResourceSVC)
		resourceSVC.WorkerId = common.WorkerIdType(workerId)
		resourceSVC.ResourceIP = nodeIP
		resourceSVC.WorkerPort = tf.workerPortList[partyIndex]

		resourceSVC.JobNetCfg = mockConstructNtCfg(tf.ExecutorExecutorPortList[partyIndex], tf.MpcMpcPortList[partyIndex], workerId, taskClassID)

		resIns.ResourceSVCs[common.WorkerIdType(workerId)] = resourceSVC

		reply := comms_pattern.RunWorkerReply{
			EncodedStr: comms_pattern.EncodePartyRunWorkerReply(resIns),
		}
		requiredResource[partyIndex] = reply.EncodedStr
	}

	JobNetCfgIns := comms_pattern.GetJobNetCfg()[job.JobFlType]

	JobNetCfgIns.Constructor(requiredResource, uint(len(job.PartyAddrList)), logger.Log)

	return JobNetCfgIns
}

type testCfg struct {
	dsl                      string
	workerPortList           []common.PortType
	ExecutorExecutorPortList [][]common.PortType
	MpcMpcPortList           []common.PortType
}

func TestTaskPredict(t *testing.T) {

	logger.Log, logger.LogFile = logger.GetLogger("./TestSubProc")

	dsl := "/Users/kevin/project_golang/src/github.com/falcon/src/falcon_platform/examples/full_template/three_parties_lime_job_breastcancer_lr_predict.json"
	workerPortList := []common.PortType{22114, 22119, 22124}
	ExecutorExecutorPortList := [][]common.PortType{{22115, 22116, 22117}, {22120, 22121, 22122}, {22125, 22126, 22127}}
	MpcMpcPortList := []common.PortType{22118, 22123, 22128}

	tf := new(testCfg)
	tf.dsl = dsl
	tf.workerPortList = workerPortList
	tf.ExecutorExecutorPortList = ExecutorExecutorPortList
	tf.MpcMpcPortList = MpcMpcPortList

	// begin to test
	common.MpcExePath = "/opt/falcon/third_party/MP-SPDZ/semi-party.x"
	common.FLEnginePath = "/opt/falcon/build/src/executor/falcon"

	// 1. parse job into object.
	job := testParseTrainJob(dsl, 8)

	// 2. schedule job into multiple parties.
	dagScheduler := testSchedule(job)
	stage, _ := dagScheduler.DagTasks[common.LimePredTaskKey]
	mpcAlgorithmName := reflect.ValueOf(job.ExecutedTasks[stage.Name]).Elem().Field(0).String()

	// 3. init stage
	JobNetCfgIns := testStageInit(stage, job, tf)

	mpc := new(tasks.MpcTask)
	lpt := new(tasks.LimePredictTask)

	// 4. test worker 0
	serviceName := "pty0-cent-worker8-job8-tr-pred-task-stage"
	common.TaskRuntimeLogs = "/opt/falcon/src/falcon_platform/falcon_logs/Party-0_20220603_205609/" + common.RuntimeLogs + "/" + serviceName

	wk := &entity.WorkerInfo{"127.0.0.1", 0, 8, 0}
	mpcTaskInfo := &entity.TaskContext{TaskName: common.MpcTaskKey,
		Wk:           wk,
		FLNetworkCfg: JobNetCfgIns.SerializeNetworkCfg(),
		Job:          &job,
		MpcAlgName:   mpcAlgorithmName,
	}

	taskInfo := &entity.TaskContext{TaskName: stage.Name,
		Wk:           wk,
		FLNetworkCfg: JobNetCfgIns.SerializeNetworkCfg(),
		Job:          &job}

	// test serialize and DeserializeTask
	args := string(entity.SerializeTask(taskInfo))
	fmt.Println(args)
	argTask, err := entity.DeserializeTask([]byte(args))
	if err != nil {
		panic(err)
	}
	fmt.Println(argTask)

	mpcCmdStr := mpc.GetCommand(mpcTaskInfo).String()
	expectedStr1 := "/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 3 -p 0 -I -ip /opt/falcon/third_party/MP-SPDZ/mpc-network-0 logistic_regression"
	assert.Equal(t, mpcCmdStr, expectedStr1, "Prediction MPC command is not correct ")

	lptCmdStr := lpt.GetCommand(taskInfo).String()
	expectedStr2 := "/opt/falcon/build/src/executor/falcon --party-id 0 --party-num 3 --party-type 0 --fl-setting 1 --network-file CgkxMjcuMC4wLjEKCTEyNy4wLjAuMQoJMTI3LjAuMC4xEgsKCeOsAeSsAeWsARILCgnorAHprAHqrAESCwoJ7awB7qwB76wBGgsKCYiFA4iFA4iFAw== --log-file /opt/falcon/src/falcon_platform/falcon_logs/Party-0_20220603_205609/runtime_logs/pty0-cent-worker8-job8-tr-pred-task-stage-lime_compute_prediction/centralized_worker --data-input-file /dataPath/client.txt --data-output-file /dataOutputPath --existing-key 1 --key-file /dataPath/phe_keys --algorithm-name lime_compute_prediction --algorithm-params ChNsb2dpc3RpY19yZWdyZXNzaW9uEhcvbG9nX3JlZy9zYXZlZF9tb2RlbC5wYhoZL2xvZ19yZWcvc2FtcGxlZF9kYXRhLnR4dCIOY2xhc3NpZmljYXRpb24oAjIYL2xvZ19yZWcvcHJlZGljdGlvbnMudHh0 --model-save-file /modelPath/saved_model.pb --model-report-file /modelPath/report.txt --is-inference 0 --inference-endpoint 0 --is-distributed 0 --distributed-train-network-file 0 --worker-id 0 --distributed-role 2"
	fmt.Println(lptCmdStr)
	fmt.Println(expectedStr2)
	assert.Equal(t, lptCmdStr, expectedStr2, "Prediction command is not correct ")

	// 4. test worker 1...

}
