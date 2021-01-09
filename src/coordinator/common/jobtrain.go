package common

import (
	"coordinator/logger"
	"encoding/json"
	"errors"
	"log"
	"strings"

	"google.golang.org/protobuf/proto"
)

// protoc -I=/Users/nailixing/GOProj/src/github.com/falcon/src/executor/include/proto/v0/ --go_out=/Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/common /Users/nailixing/GOProj/src/github.com/falcon/src/executor/include/proto/v0/job.proto
// TODO: make the train job field consistent with wyc executor flags!
// TODO: jobFLtype wyc side expects int
// TODO: partyType wyc side expects int
type TrainJob struct {
	JobName     string      `json:"job_name"`
	JobDecs     string      `json:"job_decs"`
	JobFlType   string      `json:"job_fl_type"`
	ExistingKey uint        `json:"existing_key"`
	PartyNums   uint        `json:"party_nums,uint"`
	TaskNum     uint        `json:"task_num,uint"`
	PartyInfo   []PartyInfo `json:"party_info"`
	Tasks       Tasks       `json:"tasks"`
}

type PartyInfo struct {
	ID         uint      `json:"id"`
	Addr       string    `json:"addr"`
	PartyType  string    `json:"party_type"`
	PartyPaths PartyPath `json:"path"`
}

type PartyPath struct {
	DataInput  string `json:"data_input"`
	DataOutput string `json:"data_output"`
	ModelPath  string `json:"model_path"`
}

type Tasks struct {
	PreProcessing PreProcessTask `json:"pre_processing"`
	ModelTraining ModelTrainTask `json:"model_training"`
}

type PreProcessTask struct {
	AlgorithmName string       `json:"algorithm_name"`
	InputConfigs  InputConfig  `json:"input_configs"`
	OutputConfigs PreProOutput `json:"output_configs"`
}

type ModelTrainTask struct {
	AlgorithmName string      `json:"algorithm_name"`
	InputConfigs  InputConfig `json:"input_configs"`
	OutputConfigs ModelOutput `json:"output_configs"`
}

type ModelOutput struct {
	TrainedModel     string `json:"trained_model"`
	EvaluationReport string `json:"evaluation_report"`
}

type PreProOutput struct {
	DataOutput string `json:"data_output"`
}

type InputConfig struct {
	DataInput                 DataInput              `json:"data_input"`
	AlgorithmConfig           map[string]interface{} `json:"algorithm_config"`
	SerializedAlgorithmConfig string
}

type DataInput struct {
	Data string `json:"data"`
	Key  string `json:"key"`
}

func ParseTrainJob(contents string, jobInfo *TrainJob) error {
	// the error here can only check if field type is correct or not.
	// if the field is not filled, still pass, default to 0
	marshalErr := json.Unmarshal([]byte(contents), jobInfo)
	if marshalErr != nil {
		logger.Do.Println("Json unmarshal error", marshalErr)
		panic("Json unmarshal error")
	}

	logger.Do.Println("Searching Algorithms...")

	// if there is PreProcessing, serialize it
	if jobInfo.Tasks.PreProcessing.AlgorithmName != "" {
		logger.Do.Println("ParseTrainJob: PreProcessing AlgorithmName match <-->", jobInfo.Tasks.PreProcessing.AlgorithmName)
		jobInfo.Tasks.PreProcessing.InputConfigs.SerializedAlgorithmConfig =
			GeneratePreProcessparams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	}
	// if there is ModelTraining, serialize it
	if jobInfo.Tasks.ModelTraining.AlgorithmName != "" {
		logger.Do.Println("ParseTrainJob: ModelTraining AlgorithmName match <-->", jobInfo.Tasks.ModelTraining.AlgorithmName)

		jobInfo.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig =
			GenerateLrParams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	}

	verifyErr := trainJobVerify(jobInfo)
	if verifyErr != nil {
		logger.Do.Println("train job verify error", verifyErr)
		return errors.New("train job verify error")
	}

	logger.Do.Println("Parsed jobInfo: ", jobInfo)

	return nil
}

func trainJobVerify(jobInfo *TrainJob) error {

	// verify task_num
	// TODO: task_num should be derived from json list
	if jobInfo.TaskNum <= 0 {
		return errors.New("task_num must be integer and >0")
	}

	// verify party info
	for _, v := range jobInfo.PartyInfo {

		if len(v.Addr) == 0 {
			return errors.New("IP must be provided")
		}
	}

	// verify Job federated learning Type | fl-setting
	// fmt.Println(jobInfo.JobFlType, HorizontalFl, VerticalFl)
	if jobInfo.JobFlType != HorizontalFl && jobInfo.JobFlType != VerticalFl {
		return errors.New("job_fl_type must be either 'horizontal' or 'vertical' ")
	}

	return nil
}

func ParseAddress(pInfo []PartyInfo) []string {
	var Addrs []string

	for _, v := range pInfo {

		// list of addresses
		Addrs = append(Addrs, v.Addr)
	}
	return Addrs
}

func GenerateLrParams(cfg map[string]interface{}) string {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateLrParams error in doing Marshal")
	}

	res := LogisticRegression{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateLrParams error in doing Unmarshal")
	}

	lrp := LogisticRegressionParams{
		BatchSize:                 res.BatchSize,
		MaxIteration:              res.MaxIteration,
		ConvergeThreshold:         res.ConvergeThreshold,
		WithRegularization:        res.WithRegularization,
		Alpha:                     res.Alpha,
		LearningRate:              res.LearningRate,
		Decay:                     res.Decay,
		Penalty:                   res.Penalty,
		Optimizer:                 res.Optimizer,
		MultiClass:                res.MultiClass,
		Metric:                    res.Metric,
		DifferentialPrivacyBudget: res.DifferentialPrivacyBudget,
	}

	out, err := proto.Marshal(&lrp)
	if err != nil {

		// todo, should we exit from here directly ????? error handler management?
		log.Fatalln("Failed to encode GenerateLrparams:", err)
	}

	return string(out)
}

func GeneratePreProcessparams(cfg map[string]interface{}) string {
	return ""
}

func GenerateNetworkConfig(addrs []string, portArray [][]int32) string {
	logger.Do.Println("Scheduler: Generating NetworkCfg ...")
	logger.Do.Println("Scheduler: Assigned IP and ports are: ", addrs, portArray)

	partyNums := len(addrs)
	var IPs []string
	for _, v := range addrs {
		IPs = append(IPs, strings.Split(v, ":")[0])
	}

	cfg := NetworkConfig{
		IPs:        IPs,
		PortArrays: []*PortArray{},
	}

	// for each IP addr
	for i := 0; i < partyNums; i++ {
		p := &PortArray{Ports: portArray[i]}
		cfg.PortArrays = append(cfg.PortArrays, p)
	}

	out, err := proto.Marshal(&cfg)
	if err != nil {
		logger.Do.Println("Generate NetworkCfg failed ", err)
		panic(err)
	}

	return string(out)

}
