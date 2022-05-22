package tasks

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	v0 "falcon_platform/common/proto/v0"
	"github.com/golang/protobuf/proto"

	"falcon_platform/common"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"os/exec"
)

// init register all existing tasks.
func init() {
	allTasks = GetAllTasks()
}

type TaskAbstract struct {
}

func (fl *TaskAbstract) GetRpcCallMethodName() string {
	return "DoTask"
}

func (fl *TaskAbstract) printParams(algName string, job *common.TrainJob) {
	logger.Log.Println("[TrainWorker]: ----------- WorkerAddr , printParams -----------", algName)
	//return

	if algName == "" {

	} else if algName == common.LogisticRegressAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.LogisticRegressionParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

	} else if algName == common.DecisionTreeAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.DecisionTreeParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

	} else if algName == common.RandomForestAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.RandomForestParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

		logger.Log.Printf("[TrainWorker]: original DtParam = %+v\n", lrp.DtParam)
		bs2, _ := json.Marshal(lrp.DtParam)
		var out2 bytes.Buffer
		_ = json.Indent(&out2, bs2, "", "\t")
		logger.Log.Printf("structure DtParam =%v\n", out2.String())

	} else if algName == common.LinearRegressionAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.LinearRegressionParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

	} else if algName == common.GBDTAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.GbdtParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

		logger.Log.Printf("[TrainWorker]: original DtParam = %+v\n", lrp.DtParam)
		bs2, _ := json.Marshal(lrp.DtParam)
		var out2 bytes.Buffer
		_ = json.Indent(&out2, bs2, "", "\t")
		logger.Log.Printf("structure DtParam =%v\n", out2.String())

	} else if algName == common.LimeSamplingAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.LimeInsSample.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.LimeSamplingParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

	} else if algName == common.LimeCompPredictionAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.LimePred.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.LimeCompPredictionParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

	} else if algName == common.LimeCompWeightsAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.LimeWeight.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.LimeCompWeightsParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

	} else if algName == common.LimeFeatSelAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.LimeFeature.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.LimeFeatSelParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

	} else if algName == common.LimeInterpretAlgName {

		res, _ := base64.StdEncoding.DecodeString(job.Tasks.LimeInterpret.InputConfigs.SerializedAlgorithmConfig)
		lrp := v0.LimeInterpretParams{}
		_ = proto.Unmarshal(res, &lrp)

		logger.Log.Printf("[TrainWorker]: original params = %+v\n", lrp)
		bs, _ := json.Marshal(lrp)
		var out bytes.Buffer
		_ = json.Indent(&out, bs, "", "\t")
		logger.Log.Printf("structure params =%v\n", out.String())

		res2, _ := base64.StdEncoding.DecodeString(lrp.InterpretModelParam)
		lrp2 := v0.LinearRegressionParams{}
		_ = proto.Unmarshal(res2, &lrp2)
		logger.Log.Printf("[TrainWorker]: original LinearRegressionParams = %+v\n", lrp2)
		bs2, _ := json.Marshal(lrp2)
		var out2 bytes.Buffer
		_ = json.Indent(&out2, bs2, "", "\t")
		logger.Log.Printf("structure LinearRegressionParams =%v\n", out2.String())

	} else {

	}

	logger.Log.Println("[TrainWorker]: ----------- WorkerAddr , printParams Done-----------")

}

// Task interface of tasks
type Task interface {

	// GetCommand init tasks envs (log dir etc,) and return cmd
	GetCommand(taskInfo *entity.TaskContext) *exec.Cmd

	GetRpcCallMethodName() string
}
