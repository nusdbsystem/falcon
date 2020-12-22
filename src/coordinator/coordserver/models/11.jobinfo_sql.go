package models

import "github.com/jinzhu/gorm"

////////////////////////////////////
/////////// JobInfo  ////////////
////////////////////////////////////

func (jobDB *JobDB) JobInfoCreate(
	tx *gorm.DB,
	JobName string,
	UserID uint,
	PartyIds string,
	TaskInfo string,
	JobDecs string,
	PartyNum uint,
	FlSetting string,
	ExistingKey uint,
	TaskNum uint,
) (error, *JobInfoRecord) {

	u := &JobInfoRecord{
		UserID:     UserID,
		JobName:    JobName,
		JobDecs:    JobDecs,
		FlSetting:    FlSetting,
		ExistingKey: ExistingKey,
		PartyNum: 	PartyNum,
		PartyIds:   PartyIds,
		TaskNum:    TaskNum,
		TaskInfo:  TaskInfo,
	}

	err := tx.Create(u).Error
	return err, u

}


func (jobDB *JobDB) JobInfoGetByUserID(userId uint) (error, *JobInfoRecord) {

	u := &JobInfoRecord{}
	err := jobDB.Db.Where("user_id = ?", userId).First(u).Error
	return err, u
}


func (jobDB *JobDB) JobInfoIdGetByUserIDAndJobName(UserId uint, jobName string) (error, []uint) {

	var u []*JobInfoRecord
	var res []uint

	err := jobDB.Db.Where("user_id = ? AND job_name = ?", UserId, jobName).Find(&u).Error

	if err==nil{
		for _, item := range u{
			res = append(res, item.Id)
		}
	}

	return err, res
}

func (jobDB *JobDB) JobInfoGetById(Id uint) (error, *JobInfoRecord) {

	u := &JobInfoRecord{}
	err := jobDB.Db.Where("id = ?", Id).First(u).Error
	return err, u
}
