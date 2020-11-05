package models

import (
	"time"
)

////////////////////////////////////
/////////// JobInfo  ////////////
////////////////////////////////////

func (ms *MetaStore) JobSubmit(
	JobName string,
	UserID uint,
	PartyIds string,
	JobDecs string,
	TaskNum uint,
	Status uint,
) (error, *JobRecord) {

	u := &JobRecord{
		JobName:    JobName,
		UserID:     UserID,
		PartyIds:   PartyIds,
		TaskNum:    TaskNum,
		Status:     Status,
		JobDecs:    JobDecs,
		CreateTime: time.Now(),
		UpdateTime: time.Now(),
		DeleteTime: time.Now(),
	}

	err := ms.Db.Create(u).Error
	return err, u

}

func (ms *MetaStore) JobGetByJobID(jobId uint) (error, *JobRecord) {

	u := &JobRecord{}
	err := ms.Db.Where("job_id = ?", jobId).First(u).Error
	return err, u
}

func (ms *MetaStore) JobUpdateMaster(jobId uint, masterAddr string) (error, *JobRecord) {

	u := &JobRecord{}
	err := ms.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("master_address", masterAddr).Error
	return err, u

}

func (ms *MetaStore) JobUpdateStatus(jobId uint, status uint) (error, *JobRecord) {

	u := &JobRecord{}
	err := ms.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("status", status).Error
	return err, u

}

func (ms *MetaStore) JobUpdateResInfo(jobId uint, jobErrMsg, jobResult, jobExtInfo string) (error, *JobRecord) {

	u := &JobRecord{}
	err := ms.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("error_msg", jobErrMsg).
		Update("job_result", jobResult).
		Update("ext_info", jobExtInfo).Error
	return err, u

}

////////////////////////////////////////////////////////////////
/////////// JobInfo,  call by other internal thread ////////////
////////////////////////////////////////////////////////////////

func JobUpdateStatus(jobId uint, status uint) {
	ms := InitDefaultMetaStore()

	ms.Connect()
	ms.Tx = ms.Db.Begin()

	e2, _ := ms.JobUpdateStatus(jobId, status)

	ms.Commit(e2)
	ms.DisConnect()
}
