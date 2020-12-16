package models

////////////////////////////////////
/////////// ServiceInfo  ////////////
////////////////////////////////////

func (jobDB *JobDB) SvcCreate(jobId uint) (error, *ServiceRecord) {

	u := &ServiceRecord{
		JobID: jobId,
	}
	err := jobDB.Db.Create(u).Error
	return err, u
}

func (jobDB *JobDB) SvcUpdateMaster(jobId uint, masterAddr string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("master_addr", masterAddr).Error

	return err, u
}

func (jobDB *JobDB) SvcUpdateWorker(workerAddr string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", 123).
		Update("worker_addr", workerAddr).Error

	return err, u
}
