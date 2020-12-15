package models

import (
	"coordinator/common"
	"time"
)



func (jobDB *JobDB) CreateService(
	AppName string,
	ModelId uint,
	JobId uint,
	ExtInfo string,

) (error, *ModelServiceInfo) {

	u := &ModelServiceInfo{
		ModelServiceName: AppName,
		Status:           common.JobInit,
		JobId:            JobId,
		ModelId:          ModelId,
		IsPublished:      1,
		IsDelete:         0,
		CreateTime:       time.Now(),
		UpdateTime:       time.Now(),
		DeleteTime:       time.Now(),
		ExtInfo:          ExtInfo,
	}

	err := jobDB.Db.Create(u).Error
	return err, u

}


func (jobDB *JobDB) PublishService(
	jobId uint,
	IsPublished uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("is_published", IsPublished).
		Update("update_time", time.Now()).Error

	return err, u

}


func (jobDB *JobDB) ModelServiceUpdateStatus(
	jobId, status uint,

) (error, *ModelServiceInfo) {

	//todo Should we use job id to update moder_serve？add index to it if we use later

	u := &ModelServiceInfo{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("status", status).Error
	return err, u

}