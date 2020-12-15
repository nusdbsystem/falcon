package common

import (
	"encoding/json"
	"errors"
	"google.golang.org/protobuf/proto"
	"log"
)

// protoc -I=/Users/nailixing/GOProj/src/github.com/falcon/src/executor/include/proto/v0/ --go_out=/Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/common /Users/nailixing/GOProj/src/github.com/falcon/src/executor/include/proto/v0/job.proto
type DSL struct {
	JobName    		string      				`json:"job_name"`
	JobDecs   	 	string      				`json:"job_decs"`
	JobFlType  		string      				`json:"job_fl_type"`
	ExistingKey  	uint      					`json:"existing_key"`
	PartyNums  		uint        				`json:"party_nums,uint"`
	TaskNum    		uint        				`json:"task_num,uint"`
	PartyInfos 		[]PartyInfo 				`json:"party_info"`
	Tasks      		Tasks      					`json:"tasks"`
}

type PartyInfo struct {
	ID         		uint      `json:"id"`
	IP         		string    `json:"ip"`
	PartyType       uint      `json:"party_type"`
	PartyPaths 		PartyPath `json:"path"`
}

type PartyPath struct {
	DataInput      string `json:"data_input"`
	DataOutput     string `json:"data_output"`
	ModelPath      string `json:"model_path"`
}

type Tasks struct {
	PreProcessing PreProcessTask `json:"pre_processing"`
	ModelTraining ModelTrainTask `json:"model_training"`
}

type PreProcessTask struct {
	AlgorithmName string            `json:"algorithm_name"`
	InputConfigs  InputConfig 		`json:"input_configs"`
	OutputConfigs PreProOutput 		`json:"output_configs"`
}

type ModelTrainTask struct {
	AlgorithmName string      `json:"algorithm_name"`
	InputConfigs  InputConfig `json:"input_configs"`
	OutputConfigs ModelOutput `json:"output_configs"`
}

type ModelOutput struct {
	TrainedModel 		string    `json:"trained_model"`
	EvaluationReport  	string 		`json:"evaluation_report"`
}

type PreProOutput struct {
	DataOutput 		string    	`json:"data_output"`
}

type InputConfig struct {
	DataInput 					DataInput    			`json:"data_input"`
	AlgorithmConfig  			map[string]interface{} 	`json:"algorithm_config"`
	SerializedAlgorithmConfig	string
}

type DataInput struct{

	Data  string  `json:"data"`
	Key   string    `json:"key"`
}


func ParseJob(contents string, jobInfo *DSL) error {
	// the error here can only check if field type is correct or not.
	// if the field is not filled, still pass, default to 0
	e := json.Unmarshal([]byte(contents), jobInfo)
	if e != nil {
		panic("Parse error")
	}

	// if there is PreProcessing, serialize it
	if jobInfo.Tasks.PreProcessing.AlgorithmName!=""{
		jobInfo.Tasks.PreProcessing.InputConfigs.SerializedAlgorithmConfig=
			GeneratePreProcessParams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	}
	// if there is ModelTraining, serialize it
	if jobInfo.Tasks.ModelTraining.AlgorithmName!=""{

		jobInfo.Tasks.ModelTraining.InputConfigs.SerializedAlgorithmConfig =
			GenerateLrParams(jobInfo.Tasks.ModelTraining.InputConfigs.AlgorithmConfig)
	}

	ep := jobVerify(jobInfo)
	if ep != nil {
		return errors.New("parse verify error")
	}
	return nil
}

func jobVerify(jobInfo *DSL) error {

	// verify task_num
	if jobInfo.TaskNum <= 0 {
		return errors.New("task_num must be integer and >0")
	}

	// verify party info
	for _, v := range jobInfo.PartyInfos {

		if len(v.IP) == 0 {
			return errors.New("ip must be provided")
		}
	}

	return nil
}

func ParseIps(pInfo []PartyInfo) ([]string){
	var iPs []string

	for _, v := range pInfo {

		// list of ip
		iPs = append(iPs, v.IP)
	}
	return iPs
}

func GenerateLrParams(cfg map[string]interface{}) string {

	jb, err := json.Marshal(cfg)
	if err!=nil{
		panic("GenerateLrParams error in doing Marshal")
	}

	res := LogisticRegression{}

	if err := json.Unmarshal(jb, &res); err != nil {
		// do error check
		panic("GenerateLrParams error in doing Unmarshal")
	}

	lrp := LogisticRegressionParams{
		BatchSize:res.BatchSize,
		MaxIteration:res.MaxIteration,
		ConvergeThreshold:res.ConvergeThreshold,
		WithRegularization:res.WithRegularization,
		Alpha:res.Alpha,
		LearningRate:res.LearningRate,
		Decay:res.Decay,
		Penalty:res.Penalty,
		Optimizer:res.Optimizer,
		MultiClass:res.MultiClass,
		Metric:res.Metric,
		DifferentialPrivacyBudget:res.DifferentialPrivacyBudget,
	}

	out, err := proto.Marshal(&lrp)
	if err != nil {

		// todo, should we exit from here directly ????? error handler management?
		log.Fatalln("Failed to encode GenerateLrParams:", err)
	}

	return string(out)
}


func GeneratePreProcessParams(cfg map[string]interface{}) string {
	return ""
}