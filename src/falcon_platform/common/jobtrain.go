package common

import (
	"encoding/json"
	"errors"
	"falcon_platform/logger"
	"google.golang.org/protobuf/proto"
	"log"
	"strings"
)

// TODO: make the train job field consistent with wyc executor flags!
// TODO: jobFLtype wyc side expects int
// TODO: partyType wyc side expects int

// TranJob object parsed from dsl file
type TrainJob struct {
	JobName       string      `json:"job_name"`
	JobInfo       string      `json:"job_info"` // job's description
	JobFlType     string      `json:"job_fl_type"`
	ExistingKey   uint        `json:"existing_key"`
	PartyNums     uint        `json:"party_nums,uint"`
	TaskNum       uint        `json:"task_num,uint"`
	PartyInfoList []PartyInfo `json:"party_info"`
	Tasks         Tasks       `json:"tasks"`
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
	MpcAlgorithmName string       `json:"mpc_algorithm_name"`
	AlgorithmName    string       `json:"algorithm_name"`
	InputConfigs     InputConfig  `json:"input_configs"`
	OutputConfigs    PreProOutput `json:"output_configs"`
}

type ModelTrainTask struct {
	MpcAlgorithmName string      `json:"mpc_algorithm_name"`
	AlgorithmName    string      `json:"algorithm_name"`
	InputConfigs     InputConfig `json:"input_configs"`
	OutputConfigs    ModelOutput `json:"output_configs"`
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
		logger.Log.Println("Json unmarshal error", marshalErr)
		return marshalErr
	}

	logger.Log.Println("Searching Algorithms...")

	// if there is PreProcessing, serialize it
	if jobInfo.Tasks.PreProcessing.AlgorithmName != "" {
		logger.Log.Println("ParseTrainJob: PreProcessing AlgorithmName match <-->", jobInfo.Tasks.PreProcessing.AlgorithmName)
		jobInfo.Tasks.PreProcessing.InputConfigs.SerializedAlgorithmConfig =
			GeneratePreProcessparams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	}
	// if there is ModelTraining, serialize it
	if jobInfo.Tasks.ModelTraining.AlgorithmName == LogisticRegressAlgName {
		logger.Log.Println("ParseTrainJob: ModelTraining AlgorithmName match <-->", jobInfo.Tasks.ModelTraining.AlgorithmName)

		jobInfo.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig =
			GenerateLrParams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	} else if jobInfo.Tasks.ModelTraining.AlgorithmName == DecisionTreeAlgName {
		logger.Log.Println("ParseTrainJob: ModelTraining AlgorithmName match <-->", jobInfo.Tasks.ModelTraining.AlgorithmName)

		jobInfo.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig =
			GenerateTreeParams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	} else {
		return errors.New("algorithm name can not be detected")
	}

	verifyErr := trainJobVerify(jobInfo)
	if verifyErr != nil {
		logger.Log.Println("train job verify error", verifyErr)
		return errors.New("train job verify error")
	}

	//logger.Log.Println("Parsed jobInfo: ", jobInfo)

	return nil
}

func trainJobVerify(jobInfo *TrainJob) error {

	// verify task_num
	// TODO: task_num should be derived from json list
	if jobInfo.TaskNum <= 0 {
		return errors.New("task_num must be integer and >0")
	}

	// verify party info
	for _, v := range jobInfo.PartyInfoList {

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

		// list of Urlesses
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
		// if error, means parser object wrong,
		log.Fatalln("Failed to encode GenerateLrparams:", err)
	}

	return string(out)
}

func GenerateTreeParams(cfg map[string]interface{}) string {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateTreeParams error in doing Marshal")
	}

	res := DecisionTree{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateTreeParams error in doing Unmarshal")
	}

	lrp := DecisionTreeParams{
		TreeType:            res.TreeType,
		Criterion:           res.Criterion,
		SplitStrategy:       res.SplitStrategy,
		ClassNum:            res.ClassNum,
		MaxDepth:            res.MaxDepth,
		MaxBins:             res.MaxBins,
		MinSamplesSplit:     res.MinSamplesSplit,
		MinSamplesLeaf:      res.MinSamplesLeaf,
		MaxLeafNodes:        res.MaxLeafNodes,
		MinImpurityDecrease: res.MinImpurityDecrease,
		MinImpuritySplit:    res.MinImpuritySplit,
		DpBudget:            res.DpBudget,
	}

	out, err := proto.Marshal(&lrp)
	if err != nil {
		log.Fatalln("Failed to encode GenerateLrparams:", err)
	}

	return string(out)
}

func GeneratePreProcessparams(cfg map[string]interface{}) string {
	return ""
}

func GenerateNetworkConfig(Urls []string, portArray [][]int32) string {

	//logger.Log.Println("Scheduler: Assigned IP and ports are: ", Urls, portArray)

	partyNums := len(Urls)
	var IPs []string
	for _, v := range Urls {
		IPs = append(IPs, strings.Split(v, ":")[0])
	}

	cfg := NetworkConfig{
		IPs:        IPs,
		PortArrays: []*PortArray{},
	}

	// for each IP Url
	for i := 0; i < partyNums; i++ {
		p := &PortArray{Ports: portArray[i]}
		cfg.PortArrays = append(cfg.PortArrays, p)
	}

	out, err := proto.Marshal(&cfg)
	if err != nil {
		logger.Log.Println("Generate NetworkCfg failed ", err)
		panic(err)
	}

	return string(out)

}
