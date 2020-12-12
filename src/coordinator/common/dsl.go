package common

import (
	"encoding/json"
	"errors"
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
	Tasks      		Tasks      `json:"tasks"`
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
	AlgorithmName string             `json:"algorithm_name"`
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
	DataInput 			DataInput    			`json:"data_input"`
	AlgorithmConfig  	map[string]interface{} 	`json:"algorithm_config"`
}

type DataInput struct{

	Data  string  `json:"data"`
	Key   string    `json:"key"`
}


func ParseDsl(contents string, jobInfo *DSL) error {
	// the error here can only check if field type is correct or not.
	// if the field is not filled, still pass, default to 0
	e := json.Unmarshal([]byte(contents), jobInfo)
	if e != nil {
		panic("Parse error")
	}
	ep := dslVerify(jobInfo)
	if ep != nil {
		return errors.New("parse verify error")
	}
	return nil
}

func dslVerify(jobInfo *DSL) error {

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
