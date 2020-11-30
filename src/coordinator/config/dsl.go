package config

import (
	"encoding/json"
	"errors"
)

type DSL struct {
	JobName    string      `json:"job_name"`
	JobDecs    string      `json:"job_decs"`
	JobFlType  string      `json:"job_fl_type"`
	PartyNums  uint        `json:"party_nums,uint"`
	TaskNum    uint        `json:"task_num,uint"`
	PartyInfos []PartyInfo `json:"party_info"`
	Tasks      Tasks       `json:"tasks"`
}

type PartyInfo struct {
	ID         uint      `json:"id"`
	IP         string    `json:"ip"`
	PartyPaths PartyPath `json:"path"`
}

type PartyPath struct {
	DataInput      string `json:"data_input"`
	DataOutput     string `json:"data_output"`
	Model          string `json:"model"`
	Report         string `json:"report"`
	ExecutablePath string `json:"executable_path"`
}

type Tasks struct {
	PreProcessing PreProcessTask `json:"pre_processing"`
	ModelTraining ModelTrainTask `json:"model_training"`
}

type PreProcessTask struct {
	AlgorithmName string                 `json:"algorithm_name"`
	InputConfigs  map[string]interface{} `json:"input_configs"`
	OutputConfigs map[string]interface{} `json:"output_configs"`
}

type ModelTrainTask struct {
	AlgorithmName string                 `json:"algorithm_name"`
	InputConfigs  map[string]interface{} `json:"input_configs"`
	OutputConfigs map[string]interface{} `json:"output_configs"`
}

type ModelOutput struct {
	TrainedModel 		[]string    `json:"trained_model"`
	EvaluationReport  	string 		`json:"evaluation_report"`
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
			return errors.New("ip must be provide")
		}
	}

	return nil
}
