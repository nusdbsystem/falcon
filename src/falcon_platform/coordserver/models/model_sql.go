package models

import (
	"github.com/jinzhu/gorm"
	"time"
)


func (jobDB *JobDB) ModelCreate(
	tx *gorm.DB,
	JobId uint,
	ModelName string,
	ModelDecs string,

) (error, *ModelRecord) {

	u := &ModelRecord{
		JobId:     		 JobId,
		ModelName:    	ModelName,
		ModelDecs:    	ModelDecs,

		IsTrained:			 0,

		CreateTime: 		 time.Now(),
		UpdateTime: 		 time.Now(),
		DeleteTime:  		 time.Now(),
	}


	err := tx.Create(u).Error
	return err, u

}



func (jobDB *JobDB) ModelUpdate(
	tx *gorm.DB,
	jobId uint,
	IsTrained uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := tx.Model(u).
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