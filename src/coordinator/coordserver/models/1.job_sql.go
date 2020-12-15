package models

import (
	"time"
)

////////////////////////////////////
/////////// JobInfo  ////////////
////////////////////////////////////

func (jobDB *JobDB) JobSubmit(
	JobName string,
	UserID uint,
	PartyIds string,
	TaskInfos string,
	JobDecs string,
	TaskNum uint,
	Status uint,
) (error, *JobRecord) {

	u := &JobRecord{
		JobName:    JobName,
		UserID:     UserID,
		PartyIds:   PartyIds,
		TaskInfos:  TaskInfos,
		TaskNum:    TaskNum,
		Status:     Status,
		JobDecs:    JobDecs,
		CreateTime: time.Now(),
		UpdateTime: time.Now(),
		DeleteTime: time.Now(),
	}

	err := jobDB.Db.Create(u).Error
	return err, u

}

func (jobDB *JobDB) JobGetByJobID(jobId uint) (error, *JobRecord) {

	u := &JobRecord{}
	err := jobDB.Db.Where("job_id = ?", jobId).First(u).Error
	return err, u
}

func (jobDB *JobDB) JobUpdateMaster(jobId uint, masterAddr string) (error, *JobRecord) {

	u := &JobRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("master_address", masterAddr).Error
	return err, u

}

func (jobDB *JobDB) JobUpdateStatus(jobId uint, status uint) (error, *JobRecord) {

	u := &JobRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("status", status).Error
	return err, u

}

func (jobDB *JobDB) JobUpdateResInfo(jobId uint, jobErrMsg, jobResult, jobExtInfo string) (error, *JobRecord) {

	u := &JobRecord{}
	err := jobDB.Db.Model(u).
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
	jobDB := InitJobDB()

	jobDB.Connect()
	jobDB.Tx = jobDB.Db.Begin()

	e2, _ := jobDB.JobUpdateStatus(jobId, status)

	jobDB.Commit(e2)
	jobDB.Disconnect()
}
