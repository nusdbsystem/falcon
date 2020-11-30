package models

import "time"



func (ms *MetaStore) PublishService(
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
