package models

import (
	"coordinator/common"
	"time"
)

func (jobDB *JobDB) CreateInference(
	ModelId uint,
	JobId uint,

) (error, *InferenceJobRecord) {

	u := &InferenceJobRecord{
		Status:     common.JobInit,
		JobId:      JobId,
		ModelId:    ModelId,
		CreateTime: time.Now(),
		UpdateTime: time.Now(),
		DeleteTime: time.Now(),
	}

	err := jobDB.Db.Create(u).Error
	return err, u

}

func (jobDB *JobDB) PublishInference(
	jobId uint,
	IsTrained uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("is_trained", IsTrained).
		Update("update_time", time.Now()).Error

	return err, u

}

func (jobDB *JobDB) InferenceUpdateStatus(
	jobId, status uint,

) (error, *InferenceJobRecord) {

	//todo Should we use job id to update moder_serve？add index to it if we use later

	u := &InferenceJobRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("status", status).Error
	return err, u

}
