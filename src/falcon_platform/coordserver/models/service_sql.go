package models

import "gorm.io/gorm"

////////////////////////////////////
/////////// ServiceInfo  ////////////
////////////////////////////////////

func (jobDB *JobDB) SvcCreate(tx *gorm.DB, jobId uint) (error, *ServiceRecord) {

	u := &ServiceRecord{
		JobID: jobId,
	}
	err := tx.Create(u).Error
	return err, u
}

func (jobDB *JobDB) SvcUpdateMaster(tx *gorm.DB, jobId uint, masterAddr string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := tx.Model(u).
		Where("job_id = ?", jobId).
		Update("master_addr", masterAddr).Error

	return err, u
}

func (jobDB *JobDB) SvcUpdateWorker(tx *gorm.DB, workerAddr string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := tx.Model(u).
		Where("job_id = ?", 123).
		Update("worker_addr", workerAddr).Error

	return err, u
}
