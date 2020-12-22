package common

import (
	"encoding/json"
	"errors"
)

type InferenceJob struct {
	JobName    		string      						`json:"job_name"`
	JobId      		uint      							`json:"job_id"`
	DataInfo 		[]InferenceDataInfo 		`json:"party_info"`
}

type InferenceDataInfo struct {
	ID         				uint      `json:"id"`
	InferenceDataPath 		string 	  `json:"production_data_path"`
}

func ParseInferenceJob(contents string, jobInfo *InferenceJob) error {
	// the error here can only check if field type is correct or not.
	// if the field is not filled, still pass, default to 0
	e := json.Unmarshal([]byte(contents), jobInfo)
	if e != nil {
		panic("InferenceDataInfo Parse error")
	}
	ep := InferenceJobVerify(jobInfo)
	if ep != nil {
		return errors.New("InferenceJobVerify verify error")
	}

	return nil
}

func InferenceJobVerify(jobInfo *InferenceJob) error {

	if len(jobInfo.DataInfo) > 0 {
		return errors.New("DataPath must be provided")
	}

	// verify party info
	for _, v := range jobInfo.DataInfo {

		if len(v.InferenceDataPath) == 0 {
			return errors.New("InferenceDataPath must be provided for party")
		}
	}

	return nil
}