package models

import (
	"time"
)

////////////////////////////////////
/////////// Jobs        ////////////
////////////////////////////////////

func (jobDB *JobDB) JobSubmit(
	UserID uint,
	Status uint,
	JobInfoID uint,
) (error, *TrainJobRecord) {

	u := &TrainJobRecord{
		UserID:     UserID,
		Status:     Status,
		JobInfoID:  JobInfoID,
		CreateTime: time.Now(),
		UpdateTime: time.Now(),
		DeleteTime: time.Now(),
	}

	err := jobDB.Db.Create(u).Error
	return err, u

}

func (jobDB *JobDB) JobGetByJobID(jobId uint) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := jobDB.Db.Where("job_id = ?", jobId).First(u).Error
	return err, u
}

func (jobDB *JobDB) JobGetByJobInfoID(jobInfoId uint) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := jobDB.Db.Where("job_info_id = ?", jobInfoId).First(u).Error
	return err, u
}

func (jobDB *JobDB) JobUpdateMaster(jobId uint, masterAddr string) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("master_addr", masterAddr).Error
	return err, u

}

func (jobDB *JobDB) JobUpdateStatus(jobId uint, status uint) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("status", status).Error
	return err, u

}

func (jobDB *JobDB) JobUpdateResInfo(jobId uint, jobErrMsg, jobResult, jobExtInfo string) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
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
