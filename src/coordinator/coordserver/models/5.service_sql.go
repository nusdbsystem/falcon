package models

////////////////////////////////////
/////////// ServiceInfo  ////////////
////////////////////////////////////

func (ms *MetaStore) SvcCreate(jobId uint) (error, *ServiceRecord) {

	u := &ServiceRecord{
		JobID: jobId,
	}
	err := ms.Db.Create(u).Error
	return err, u
}

func (ms *MetaStore) SvcUpdateMaster(jobId uint, masterAddr string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := ms.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("master_addr", masterAddr).Error

	return err, u
}

func (ms *MetaStore) SvcUpdateWorker(workerAddr string) (error, *ServiceRecord) {

	u := &ServiceRecord{}
	err := ms.Db.Model(u).
		Where("job_id = ?", 123).
		Update("worker_addr", workerAddr).Error

	return err, u
}
