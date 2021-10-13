package common

import (
	b64 "encoding/base64"
	"encoding/json"
	"errors"
	"falcon_platform/common/proto/v0"
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
	JobName         string          `json:"job_name"`
	JobInfo         string          `json:"job_info"` // job's description
	JobFlType       string          `json:"job_fl_type"`
	ExistingKey     uint            `json:"existing_key"`
	PartyNums       uint            `json:"party_nums,uint"`
	TaskNum         uint            `json:"task_num,uint"`
	PartyInfoList   []PartyInfo     `json:"party_info"`
	DistributedTask DistributedTask `json:"distributed_task"`
	Tasks           Tasks           `json:"tasks"`
}

type PartyInfo struct {
	ID         PartyIdType `json:"id"`
	Addr       string      `json:"addr"`
	PartyType  string      `json:"party_type"`
	PartyPaths PartyPath   `json:"path"`
}

type PartyPath struct {
	DataInput  string `json:"data_input"`
	DataOutput string `json:"data_output"`
	ModelPath  string `json:"model_path"`
}

type DistributedTask struct {
	Enable       int `json:"enable"`
	WorkerNumber int `json:"worker_number"`
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

	//logger.Log.Println("Searching Algorithms...")

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
	} else if jobInfo.Tasks.ModelTraining.AlgorithmName == RandomForestAlgName {
		logger.Log.Println("ParseTrainJob: ModelTraining AlgorithmName match <-->", jobInfo.Tasks.ModelTraining.AlgorithmName)

		jobInfo.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig =
			GenerateRFParams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
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
	var Addresses []string

	for _, v := range pInfo {

		// list of Urlesses
		Addresses = append(Addresses, v.Addr)
	}
	return Addresses
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

	lrp := v0.LogisticRegressionParams{
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
		FitBias:                   res.FitBias,
	}

	out, err := proto.Marshal(&lrp)
	if err != nil {
		// if error, means parser object wrong,
		log.Fatalln("Failed to encode LogisticRegressionParams:", err)
	}
	return b64.StdEncoding.EncodeToString(out)
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

	dtp := v0.DecisionTreeParams{
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

	out, err := proto.Marshal(&dtp)
	if err != nil {
		log.Fatalln("Failed to encode DecisionTreeParams:", err)
	}

	return string(out)
}

func GenerateRFParams(cfg map[string]interface{}) string {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateRFParams error in doing Marshal")
	}

	res := RandomForest{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateRFParams error in doing Unmarshal")
	}

	dtp := v0.DecisionTreeParams{
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

	rfp := v0.RandomForestParams{
		NEstimator: res.NEstimator,
		SampleRate: res.SampleRate,
		DtParam:    &dtp,
	}

	out, err := proto.Marshal(&rfp)
	if err != nil {
		log.Fatalln("Failed to encode RandomForestParams:", err)
	}

	return b64.StdEncoding.EncodeToString(out)

}

func GeneratePreProcessparams(cfg map[string]interface{}) string {
	return ""
}

// executorPortArray : 2-dim array, r1 demote receiver 1's port,
//	   [ [na, r2, r3],
//		 [r1, na, r3 ],
//		 [r1, r2, na ] ]
// mpcPortArray: each party's mpc port
func GenerateNetworkConfig(mIps []string, executorPortArray [][]int32, mpcPortArray []int32) string {

	//logger.Log.Println("Scheduler: Assigned IP and ports are: ", Addresses, portArray)

	partyNums := len(mIps)
	var IPs []string
	for _, v := range mIps {
		IPs = append(IPs, strings.Split(v, ":")[0])
	}

	cfg := v0.NetworkConfig{
		Ips:                        IPs,
		ExecutorExecutorPortArrays: []*v0.PortArray{},
		ExecutorMpcPortArray:       nil,
	}

	// for each IP Url
	for i := 0; i < partyNums; i++ {
		p := &v0.PortArray{Ports: executorPortArray[i]}
		cfg.ExecutorExecutorPortArrays = append(cfg.ExecutorExecutorPortArrays, p)
	}

	// each executor knows other mpc's port
	cfg.ExecutorMpcPortArray = &v0.PortArray{Ports: mpcPortArray}

	out, err := proto.Marshal(&cfg)
	if err != nil {
		logger.Log.Println("Generate NetworkCfg failed ", err)
		panic(err)
	}
	return b64.StdEncoding.EncodeToString(out)

}

func GenerateDistributedNetworkConfig(psIp string, psPort []int32, workerIps []string, workerPorts []int32) string {

	// each port corresponding to one worker
	var psWorkers []*v0.PS
	for i := 0; i < len(psPort); i++ {
		ps := v0.PS{
			PsIp:   psIp,
			PsPort: psPort[i],
		}
		psWorkers = append(psWorkers, &ps)
	}

	//  worker address
	var workers []*v0.Worker
	for i := 0; i < len(workerIps); i++ {
		worker := v0.Worker{
			WorkerIp:   workerIps[i],
			WorkerPort: workerPorts[i]}
		workers = append(workers, &worker)
	}
	//  network config
	psCfg := v0.PSNetworkConfig{
		Ps:      psWorkers,
		Workers: workers,
	}

	out, err := proto.Marshal(&psCfg)
	if err != nil {
		logger.Log.Println("Generate Distributed NetworkCfg failed ", err)
		panic(err)
	}

	return b64.StdEncoding.EncodeToString(out)
}

func RetrieveDistributedNetworkConfig(a string) {

	res, _ := b64.StdEncoding.DecodeString(a)

	cfg := v0.PSNetworkConfig{}

	_ = proto.Unmarshal(res, &cfg)

	logger.Log.Println("[RetrieveDistributedNetworkConfig]: cfg.Ps", cfg.Ps)
	logger.Log.Println("[RetrieveDistributedNetworkConfig]: cfg.Workers", cfg.Workers)

}

func RetrieveNetworkConfig(a string) {
	res, _ := b64.StdEncoding.DecodeString(a)

	cfg := v0.NetworkConfig{}

	_ = proto.Unmarshal(res, &cfg)

	logger.Log.Println("[RetrieveNetworkConfig]: cfg.Ips", cfg.Ips)
	logger.Log.Println("[RetrieveNetworkConfig]: cfg.ExecutorExecutorPortArrays", cfg.ExecutorExecutorPortArrays)
	logger.Log.Println("[RetrieveNetworkConfig]: cfg.ExecutorMpcPortArray", cfg.ExecutorMpcPortArray)

}
