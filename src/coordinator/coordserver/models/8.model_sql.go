package models

import "time"


func (jobDB *JobDB) ModelCreate(
	JobId uint,
	ModelName string,
	ModelDecs string,
	PartyNumber uint,
	PartyIds string,
	ExtInfo string,

) (error, *ModelRecord) {

	u := &ModelRecord{
		JobId:     		 JobId,
		ModelName:    	ModelName,
		ModelDecs:    	ModelDecs,

		PartyNumber:    PartyNumber,
		PartyIds:    	PartyIds,

		IsTrained:			 0,
		IsPublished:		 0,
		IsDelete:    		 0,
		CreateTime: 		 time.Now(),
		UpdateTime: 		 time.Now(),
		DeleteTime:  		 time.Now(),
		ExtInfo:    		 ExtInfo,
	}


	err := jobDB.Db.Create(u).Error
	return err, u

}



func (jobDB *JobDB) ModelUpdate(
	jobId uint,
	IsTrained uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := jobDB.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("is_trained", IsTrained).
		Update("update_time", time.Now()).Error

	return err, u

}




func (jobDB *JobDB) ModelGetByID(
	jobId uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := jobDB.Db.Where("job_id = ?", jobId).First(u).Error

	return err, u

}