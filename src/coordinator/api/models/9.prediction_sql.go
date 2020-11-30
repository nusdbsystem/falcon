package models

import "time"



func (ms *MetaStore) CreateService(
	AppName string,
	ModelId uint,
	ExtInfo string,

) (error, *ModelServiceInfo) {

	u := &ModelServiceInfo{
		ModelServiceName:    AppName,
		ModelId:     		 ModelId,
		IsPublished:		 1,
		IsDelete:    		 0,
		CreateTime: 		 time.Now(),
		UpdateTime: 		 time.Now(),
		DeleteTime:  		 time.Now(),
		ExtInfo:    		 ExtInfo,
	}


	err := ms.Db.Create(u).Error
	return err, u

}


func (ms *MetaStore) PublishService(
	jobId uint,
	IsPublished uint,

) (error, *ModelRecord) {

	u := &ModelRecord{}

	err := ms.Db.Model(u).
		Where("job_id = ?", jobId).
		Update("is_published", IsPublished).
		Update("update_time", time.Now()).Error

	return err, u

}