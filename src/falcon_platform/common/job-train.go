package common

import (
	b64 "encoding/base64"
	"encoding/json"
	"errors"
	"falcon_platform/common/proto/v0"
	"falcon_platform/logger"
	"fmt"
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
	PreProcessing PreProcessTask    `json:"pre_processing"`
	ModelTraining ModelTrainTask    `json:"model_training"`
	LimeInsSample LimeInsSampleTask `json:"lime_ins_sample"`
	LimePred      LimePredTask      `json:"lime_pred"`
	LimeWeight    LimeWeightTask    `json:"lime_weight"`
	LimeFeature   LimeFeatureTask   `json:"lime_feature"`
	LimeInterpret LimeInterpretTask `json:"lime_interpret"`
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

type LimeInsSampleTask struct {
	MpcAlgorithmName string      `json:"mpc_algorithm_name"`
	AlgorithmName    string      `json:"algorithm_name"`
	InputConfigs     InputConfig `json:"input_configs"`
	OutputConfigs    ModelOutput `json:"output_configs"`
}

type LimePredTask struct {
	MpcAlgorithmName string      `json:"mpc_algorithm_name"`
	AlgorithmName    string      `json:"algorithm_name"`
	InputConfigs     InputConfig `json:"input_configs"`
	OutputConfigs    ModelOutput `json:"output_configs"`
}

type LimeWeightTask struct {
	MpcAlgorithmName string      `json:"mpc_algorithm_name"`
	AlgorithmName    string      `json:"algorithm_name"`
	InputConfigs     InputConfig `json:"input_configs"`
	OutputConfigs    ModelOutput `json:"output_configs"`
}

type LimeFeatureTask struct {
	ClassNum         int32
	MpcAlgorithmName string      `json:"mpc_algorithm_name"`
	AlgorithmName    string      `json:"algorithm_name"`
	InputConfigs     InputConfig `json:"input_configs"`
	OutputConfigs    ModelOutput `json:"output_configs"`
}

type LimeInterpretTask struct {
	ClassNum         int32
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
	if jobInfo.Tasks.ModelTraining.AlgorithmName == "" {
		logger.Log.Println("ParseTrainJob: ModelTraining skip")

	} else if jobInfo.Tasks.ModelTraining.AlgorithmName == LogisticRegressAlgName {
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
	} else if jobInfo.Tasks.ModelTraining.AlgorithmName == LinearRegressionAlgName {
		logger.Log.Println("ParseTrainJob: ModelTraining AlgorithmName match <-->", jobInfo.Tasks.ModelTraining.AlgorithmName)

		jobInfo.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig =
			GenerateLinearRegressionParams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	} else {
		return errors.New("algorithm name can not be detected")
	}

	// if there is interpretability related task, serialize it

	// LimePred

	if jobInfo.Tasks.LimePred.AlgorithmName == "" {
		logger.Log.Println("ParseTrainJob: LimePred skip")

	} else if jobInfo.Tasks.LimePred.AlgorithmName == LimeCompPredictionAlgName {
		logger.Log.Println("ParseTrainJob: LimePred AlgorithmName match <-->", jobInfo.Tasks.LimePred.AlgorithmName)

		jobInfo.Tasks.LimePred.InputConfigs.SerializedAlgorithmConfig =
			GenerateLimeCompPredictionParams(jobInfo.Tasks.LimePred.InputConfigs.AlgorithmConfig)
	} else {
		return errors.New("algorithm name can not be detected")
	}

	// LimeWeight
	if jobInfo.Tasks.LimeWeight.AlgorithmName == "" {
		logger.Log.Println("ParseTrainJob: LimeWeight skip")

	} else if jobInfo.Tasks.LimeWeight.AlgorithmName == LimeCompWeightsAlgName {
		logger.Log.Println("ParseTrainJob: LimeWeight AlgorithmName match <-->", jobInfo.Tasks.LimeWeight.AlgorithmName)

		jobInfo.Tasks.LimeWeight.InputConfigs.SerializedAlgorithmConfig =
			GenerateLimeCompWeightsParams(jobInfo.Tasks.LimeWeight.InputConfigs.AlgorithmConfig)
	} else {
		return errors.New("algorithm name can not be detected")
	}

	// LimeFeature
	if jobInfo.Tasks.LimeFeature.AlgorithmName == "" {
		logger.Log.Println("ParseTrainJob: LimeFeature skip")

	} else if jobInfo.Tasks.LimeFeature.AlgorithmName == LimeFeatSelAlgName {
		logger.Log.Println("ParseTrainJob: LimeFeature AlgorithmName match <-->", jobInfo.Tasks.LimeFeature.AlgorithmName)

		_, jobInfo.Tasks.LimeFeature.ClassNum, _ =
			GenerateLimeFeatSelParams(jobInfo.Tasks.LimeFeature.InputConfigs.AlgorithmConfig, 0)
	} else {
		return errors.New("algorithm name can not be detected")
	}

	// LimeInterpret
	if jobInfo.Tasks.LimeInterpret.AlgorithmName == "" {
		logger.Log.Println("ParseTrainJob: LimeInterpret skip")

	} else if jobInfo.Tasks.LimeInterpret.AlgorithmName == LimeInterpretAlgName {
		logger.Log.Println("ParseTrainJob: LimeInterpret AlgorithmName match <-->", jobInfo.Tasks.LimeInterpret.AlgorithmName)

		_, jobInfo.Tasks.LimeInterpret.ClassNum =
			GenerateLimeInterpretParams(jobInfo.Tasks.LimeInterpret.InputConfigs.AlgorithmConfig, 0, "", jobInfo.Tasks.LimeInterpret.MpcAlgorithmName)
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

	return b64.StdEncoding.EncodeToString(out)
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

func GenerateLinearRegressionParams(cfg map[string]interface{}) string {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateLinearRegressionParams error in doing Marshal")
	}

	res := LinearRegression{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateLinearRegressionParams error in doing Unmarshal")
	}

	dtp := v0.LinearRegressionParams{
		BatchSize:                 res.BatchSize,
		MaxIteration:              res.MaxIteration,
		ConvergeThreshold:         res.ConvergeThreshold,
		WithRegularization:        res.WithRegularization,
		Alpha:                     res.Alpha,
		LearningRate:              res.LearningRate,
		Decay:                     res.Decay,
		Penalty:                   res.Penalty,
		Optimizer:                 res.Optimizer,
		Metric:                    res.Metric,
		DifferentialPrivacyBudget: res.DifferentialPrivacyBudget,
		FitBias:                   res.FitBias,
	}

	out, err := proto.Marshal(&dtp)
	if err != nil {
		log.Fatalln("Failed to encode GenerateLinearRegressionParams:", err)
	}

	return b64.StdEncoding.EncodeToString(out)

}

func GenerateLimeCompPredictionParams(cfg map[string]interface{}) string {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateLimeCompPredictionParams error in doing Marshal")
	}

	res := LimeCompPrediction{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateLimeCompPredictionParams error in doing Unmarshal")
	}

	dtp := v0.LimeCompPredictionParams{
		OriginalModelName:      res.OriginalModelName,
		OriginalModelSavedFile: res.OriginalModelSavedFile,
		ModelType:              res.ModelType,
		ClassNum:               res.ClassNum,
		GeneratedSampleFile:    res.GeneratedSampleFile,
		ComputedPredictionFile: res.ComputedPredictionFile,
	}

	out, err := proto.Marshal(&dtp)
	if err != nil {
		log.Fatalln("Failed to encode GenerateLimeCompPredictionParams:", err)
	}

	return b64.StdEncoding.EncodeToString(out)

}

func GenerateLimeCompWeightsParams(cfg map[string]interface{}) string {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateLimeCompWeightsParams error in doing Marshal")
	}

	res := LimeCompWeights{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateLimeCompWeightsParams error in doing Unmarshal")
	}

	dtp := v0.LimeCompWeightsParams{
		ExplainInstanceIdx:      res.ExplainInstanceIdx,
		GeneratedSampleFile:     res.GeneratedSampleFile,
		ComputedPredictionFile:  res.ComputedPredictionFile,
		IsPrecompute:            res.IsPrecompute,
		NumSamples:              res.NumSamples,
		ClassNum:                res.ClassNum,
		DistanceMetric:          res.DistanceMetric,
		Kernel:                  res.Kernel,
		KernelWidth:             res.KernelWidth,
		SampleWeightsFile:       res.SampleWeightsFile,
		SelectedSamplesFile:     res.SelectedSamplesFile,
		SelectedPredictionsFile: res.SelectedPredictionsFile,
	}

	out, err := proto.Marshal(&dtp)
	if err != nil {
		log.Fatalln("Failed to encode GenerateLimeCompWeightsParams:", err)
	}

	return b64.StdEncoding.EncodeToString(out)

}

func GenerateLimeFeatSelParams(cfg map[string]interface{}, classId int32) (string, int32, string) {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateLimeFeatSelParams error in doing Marshal")
	}

	res := LimeFeatSel{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateLimeFeatSelParams error in doing Unmarshal")
	}

	var selectFeatureFile = ""
	fileVec := []rune(res.SelectedFeaturesFile)
	selectFeatureFile = string(fileVec[:len(fileVec)-4]) + fmt.Sprintf("%d", classId) + ".txt"

	log.Println("save features to ", selectFeatureFile)

	dtp := v0.LimeFeatSelParams{
		SelectedSamplesFile:     res.SelectedSamplesFile,
		SelectedPredictionsFile: res.SelectedPredictionsFile,
		SampleWeightsFile:       res.SampleWeightsFile,
		NumSamples:              res.NumSamples,
		ClassNum:                res.ClassNum,
		ClassId:                 classId,
		FeatureSelection:        res.FeatureSelection,
		NumExplainedFeatures:    res.NumExplainedFeatures,
		SelectedFeaturesFile:    selectFeatureFile,
	}

	out, err := proto.Marshal(&dtp)
	if err != nil {
		log.Fatalln("Failed to encode GenerateLimeFeatSelParams:", err)
	}

	return b64.StdEncoding.EncodeToString(out), res.ClassNum, selectFeatureFile

}

func GenerateLimeInterpretParams(cfg map[string]interface{}, classId int32, selectFeatureFile string, mpcAlgName string) (string, int32) {

	jb, err := json.Marshal(cfg)
	if err != nil {
		panic("GenerateLimeInterpretParams error in doing Marshal")
	}

	var out []byte
	var classNum int32

	if mpcAlgName == LimeDecisionTreeAlgName {

		res := LimeInterpretDT{}

		if err := json.Unmarshal(jb, &res); err != nil {
			// do error check
			panic("GenerateLimeInterpretParams error in doing Unmarshal")
		}

		// if selectFeatureFile is empty , assign it to default value.
		// 以及如果lime_feature有执行，那么lime_interpret中的selected_data_file: 也需要加上lime_feature中带class_id的输出；
		// 如果lime_feature没有执行，那么lime_interpret可以是相同的
		if selectFeatureFile == "" {
			selectFeatureFile = res.SelectedDataFile
		}

		lr := v0.DecisionTreeParams{
			TreeType:            res.InterpretModelParam.TreeType,
			Criterion:           res.InterpretModelParam.Criterion,
			SplitStrategy:       res.InterpretModelParam.SplitStrategy,
			ClassNum:            res.InterpretModelParam.ClassNum,
			MaxDepth:            res.InterpretModelParam.MaxDepth,
			MaxBins:             res.InterpretModelParam.MaxBins,
			MinSamplesSplit:     res.InterpretModelParam.MinSamplesSplit,
			MinSamplesLeaf:      res.InterpretModelParam.MinSamplesLeaf,
			MaxLeafNodes:        res.InterpretModelParam.MaxLeafNodes,
			MinImpurityDecrease: res.InterpretModelParam.MinImpurityDecrease,
			MinImpuritySplit:    res.InterpretModelParam.MinImpuritySplit,
			DpBudget:            res.InterpretModelParam.DpBudget,
		}

		out1, err := proto.Marshal(&lr)
		if err != nil {
			log.Fatalln("Failed to encode GenerateLimeInterpretParams:", err)
		}

		dtp := v0.LimeInterpretParams{
			SelectedDataFile:        selectFeatureFile,
			SelectedPredictionsFile: res.SelectedPredictionsFile,
			SampleWeightsFile:       res.SampleWeightsFile,
			NumSamples:              res.NumSamples,
			ClassNum:                res.ClassNum,
			ClassId:                 classId,
			InterpretModelName:      res.InterpretModelName,
			InterpretModelParam:     b64.StdEncoding.EncodeToString(out1),
			ExplanationReport:       res.ExplanationReport[:len(res.ExplanationReport)-4] + fmt.Sprintf("%d", classId) + ".txt",
		}

		out, err = proto.Marshal(&dtp)
		if err != nil {
			log.Fatalln("Failed to encode GenerateLimeInterpretParams:", err)
		}

		classNum = res.ClassNum

	} else if mpcAlgName == LimeLinearRegressionAlgName {

		res := LimeInterpretLR{}

		if err := json.Unmarshal(jb, &res); err != nil {
			// do error check
			panic("GenerateLimeInterpretParams error in doing Unmarshal")
		}

		// if selectFeatureFile is empty , assign it to default value.
		// 以及如果lime_feature有执行，那么lime_interpret中的selected_data_file: 也需要加上lime_feature中带class_id的输出；
		// 如果lime_feature没有执行，那么lime_interpret可以是相同的
		if selectFeatureFile == "" {
			selectFeatureFile = res.SelectedDataFile
		}

		lr := v0.LinearRegressionParams{
			BatchSize:                 res.InterpretModelParam.BatchSize,
			MaxIteration:              res.InterpretModelParam.MaxIteration,
			ConvergeThreshold:         res.InterpretModelParam.ConvergeThreshold,
			WithRegularization:        res.InterpretModelParam.WithRegularization,
			Alpha:                     res.InterpretModelParam.Alpha,
			LearningRate:              res.InterpretModelParam.LearningRate,
			Decay:                     res.InterpretModelParam.Decay,
			Penalty:                   res.InterpretModelParam.Penalty,
			Optimizer:                 res.InterpretModelParam.Optimizer,
			Metric:                    res.InterpretModelParam.Metric,
			DifferentialPrivacyBudget: res.InterpretModelParam.DifferentialPrivacyBudget,
			FitBias:                   res.InterpretModelParam.FitBias,
		}

		out1, err := proto.Marshal(&lr)
		if err != nil {
			log.Fatalln("Failed to encode GenerateLimeInterpretParams:", err)
		}

		dtp := v0.LimeInterpretParams{
			SelectedDataFile:        selectFeatureFile,
			SelectedPredictionsFile: res.SelectedPredictionsFile,
			SampleWeightsFile:       res.SampleWeightsFile,
			NumSamples:              res.NumSamples,
			ClassNum:                res.ClassNum,
			ClassId:                 classId,
			InterpretModelName:      res.InterpretModelName,
			InterpretModelParam:     b64.StdEncoding.EncodeToString(out1),
			ExplanationReport:       res.ExplanationReport[:len(res.ExplanationReport)-4] + fmt.Sprintf("%d", classId) + ".txt",
		}

		out, err = proto.Marshal(&dtp)
		if err != nil {
			log.Fatalln("Failed to encode GenerateLimeInterpretParams:", err)
		}

		classNum = res.ClassNum

	} else {
		log.Fatalln("GenerateLimeInterpretParams, mpc alg name is not supported:", err)
	}

	return b64.StdEncoding.EncodeToString(out), classNum

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
