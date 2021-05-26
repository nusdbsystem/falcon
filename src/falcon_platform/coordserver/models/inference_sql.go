package models

import (
	"falcon_platform/common"
	"fmt"
	"time"

	"gorm.io/gorm"
)

func (jobDB *JobDB) CreateInference(
	tx *gorm.DB,
	ModelId uint,
	JobId uint,

) (error, *InferenceJobRecord) {

	u := &InferenceJobRecord{
		Status:     common.JobInit,
		JobId:      JobId,
		ModelId:    ModelId,
		CreateTime: time.Now(),
		UpdateTime: time.Now(),
		DeleteTime: time.Now(),
	}

	err := tx.Create(u).Error
	return err, u

}

func (jobDB *JobDB) InferenceUpdateStatus(
	tx *gorm.DB,
	jobId, status uint,

) (error, *InferenceJobRecord) {

	//todo Should we use job id to update moder_serve？add index to it if we use later

	u := &InferenceJobRecord{}
	err := tx.Model(u).
		Where("job_id = ?", jobId).
		Update("status", status).Error
	return err, u

}

func (jobDB *JobDB) InferenceUpdateMaster(tx *gorm.DB, Id uint, masterAddr string) (error, *InferenceJobRecord) {

	u := &InferenceJobRecord{}
	err := tx.Model(u).
		Where("id = ?", Id).
		Update("master_addr", masterAddr).Error
	return err, u

}

func (jobDB *JobDB) InferenceGetByID(jobId uint) (error, *InferenceJobRecord) {

	u := &InferenceJobRecord{}
	err := jobDB.DB.Where("id = ?", jobId).First(u).Error
	return err, u
}

func (jobDB *JobDB) InferenceGetCurrentRunningOneWithJobName(
	jobName string,
	userId uint,
) ([]uint, error) {

	type Result struct {
		inferenceId uint
	}
	var result []*Result

	sql := fmt.Sprintf("select c.id from %s.job_info_records as a "+
		"inner join %s.train_job_records as b on a.id = b.job_info_id "+
		"inner join %s.inference_job_records as c on b.job_id = c.job_id "+
		"where a.user_id = %d and a.job_name = '%s' and c.status = %d;",
		jobDB.database, jobDB.database, jobDB.database, userId, jobName, common.JobRunning)

	// Raw SQL
	e := jobDB.DB.Raw(sql).Scan(&result).Error

	var res []uint

	for _, v := range result {
		res = append(res, v.inferenceId)
	}

	return res, e

}
