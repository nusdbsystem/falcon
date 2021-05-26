package models

import (
	"time"

	"gorm.io/gorm"
)

////////////////////////////////////
/////////// Jobs        ////////////
////////////////////////////////////

func (jobDB *JobDB) JobSubmit(
	tx *gorm.DB,
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
		DeleteTime: time.Now(), // deletetime is also now??
	}

	err := tx.Create(u).Error
	return err, u

}

func (jobDB *JobDB) JobGetByJobID(jobId uint) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := jobDB.DB.Where("job_id = ?", jobId).First(u).Error
	return err, u
}

func (jobDB *JobDB) JobGetByJobInfoID(jobInfoId uint) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := jobDB.DB.Where("job_info_id = ?", jobInfoId).First(u).Error
	return err, u
}

func (jobDB *JobDB) JobUpdateMaster(tx *gorm.DB, jobId uint, masterAddr string) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := tx.Model(u).
		Where("job_id = ?", jobId).
		Update("master_addr", masterAddr).Error
	return err, u

}

func (jobDB *JobDB) JobUpdateStatus(tx *gorm.DB, jobId uint, status uint) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := tx.Model(u).
		Where("job_id = ?", jobId).
		Update("status", status).Error
	return err, u

}

func (jobDB *JobDB) JobUpdateResInfo(tx *gorm.DB, jobId uint, jobErrMsg, jobResult, jobExtInfo string) (error, *TrainJobRecord) {

	u := &TrainJobRecord{}
	err := tx.Model(u).
		Where("job_id = ?", jobId).
		Update("error_msg", jobErrMsg).
		Update("job_result", jobResult).
		Update("ext_info", jobExtInfo).Error
	return err, u

}

////////////////////////////////////////////////////////////////
/////////// JobInfo,  call by other internal thread ////////////
////////////////////////////////////////////////////////////////

// TODO: is this needed as gorm is multi-thread safe?
func JobUpdateStatus(jobId uint, status uint) {
	jobDB := InitJobDB()

	jobDB.Connect()
	tx := jobDB.DB.Begin()

	e2, _ := jobDB.JobUpdateStatus(tx, jobId, status)

	jobDB.Commit(tx, e2)
}
