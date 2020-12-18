package models


////////////////////////////////////
/////////// JobInfo  ////////////
////////////////////////////////////

func (jobDB *JobDB) JobInfoCreate(
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

	err := jobDB.Db.Create(u).Error
	return err, u

}


func (jobDB *JobDB) JobInfoGetByUserID(userId uint) (error, *JobInfoRecord) {

	u := &JobInfoRecord{}
	err := jobDB.Db.Where("user_id = ?", userId).First(u).Error
	return err, u
}


func (jobDB *JobDB) LatestJobInfoIdGetByUserIDAndJobName(UserId uint, jobName string) (error, *JobInfoRecord) {

	u := &JobInfoRecord{}

	err := jobDB.Db.Where("user_id = ? AND job_name = ?", UserId, jobName).Last(&u).Error

	return err, u
}

func (jobDB *JobDB) JobInfoGetById(Id uint) (error, *JobInfoRecord) {

	u := &JobInfoRecord{}
	err := jobDB.Db.Where("id = ?", Id).First(u).Error
	return err, u
}
