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

func (jobDB *JobDB) SvcUpdateMaster(jobId uint, masterUrl string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("master_url", masterUrl).Error

	return err, u
}

func (jobDB *JobDB) SvcUpdateWorker(workerUrl string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := jobDB.Db.Model(u).
		Where("job_id = ?", 123).
		Update("worker_url", workerUrl).Error

	return err, u
}
