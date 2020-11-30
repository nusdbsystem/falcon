package models

import "time"


func (ms *MetaStore) ModelCreate(
	JobId uint,
	ModelName string,
	ModelDecs string,
	PartyNumber uint,
	modelPaths string,
	executablePaths string,
	PartyIds string,
	ExtInfo string,

) (error, *ModelRecord) {

	u := &ModelRecord{
		JobId:     		 JobId,
		ModelName:    	ModelName,
		ModelDecs:    	ModelDecs,

		PartyNumber:    PartyNumber,
		PartyIds:    	PartyIds,

		ModelPaths:		 modelPaths,
		ExecutablePaths: executablePaths,

		IsTrained:			 0,
		IsPublished:		 0,
		IsDelete:    		 0,
		CreateTime: 		 time.Now(),
		UpdateTime: 		 time.Now(),
		DeleteTime:  		 time.Now(),
		ExtInfo:    		 ExtInfo,
	}


	err := ms.Db.Create(u).Error
	return err, u

}



func (ms *MetaStore) ModelUpdate(
	jobId uint,
	IsTrained uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := ms.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("is_trained", IsTrained).
		Update("update_time", time.Now()).Error

	return err, u

}




func (ms *MetaStore) ModelGetByID(
	jobId uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := ms.Db.Where("job_id = ?", jobId).First(u).Error

	return err, u

}