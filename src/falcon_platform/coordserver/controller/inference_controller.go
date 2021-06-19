package controller

import (
	"encoding/json"
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/coordserver/entity"
	dist "falcon_platform/distributed"
	"falcon_platform/logger"
	"time"
)

func CreateInference(inferenceJob common.InferenceJob, ctx *entity.Context) (bool, uint) {

	e, trainJob := ctx.JobDB.JobGetByJobID(inferenceJob.JobId)

	if e != nil {
		panic(e)
	}
	e, JobInfo := ctx.JobDB.JobInfoGetById(trainJob.JobInfoID)
	if e != nil {
		panic(e)
	}
	e, model := ctx.JobDB.ModelGetByID(inferenceJob.JobId)
	if e != nil {
		panic(e)
	}

	// if train is not finished, return
	if model.IsTrained == 0 {
		return false, 0
	}

	// if train is not finished, else create a inference job
	tx := ctx.JobDB.DB.Begin()
	e4, inference := ctx.JobDB.CreateInference(tx, model.ID, inferenceJob.JobId)
	ctx.JobDB.Commit(tx, e4)

	var pInfo []common.PartyInfo
	var TaskInfo common.Tasks

	err := json.Unmarshal([]byte(JobInfo.PartyIds), &pInfo)
	err2 := json.Unmarshal([]byte(JobInfo.TaskInfo), &TaskInfo)
	if err != nil || err2 != nil {
		panic("json.Unmarshal(PartyIds or TaskInfo) error")
	}

	var inferencePartyInfo []common.PartyInfo

	DataInfo := inferenceJob.DataInfo

	if JobInfo.FlSetting == common.HorizontalFl {
		for _, info := range DataInfo {
			for _, v := range pInfo {

				// TODO: should be on Active party, ie party type, not id==0
				if info.ID == v.ID && info.ID == 0 {
					tmp := common.PartyInfo{}
					tmp.ID = info.ID
					tmp.Addr = v.Addr
					tmp.PartyType = v.PartyType
					tmp.PartyPaths = common.PartyPath{}
					tmp.PartyPaths.DataInput = info.InferenceDataPath
					inferencePartyInfo = append(inferencePartyInfo, tmp)
					break
				}
			}
		}
	} else if JobInfo.FlSetting == common.VerticalFl {
		for _, info := range DataInfo {
			for _, v := range pInfo {

				if info.ID == v.ID {
					tmp := common.PartyInfo{}
					tmp.ID = info.ID
					tmp.Addr = v.Addr
					tmp.PartyType = v.PartyType
					tmp.PartyPaths = common.PartyPath{}
					tmp.PartyPaths.DataInput = info.InferenceDataPath
					inferencePartyInfo = append(inferencePartyInfo, tmp)
				}
			}
		}
	}

	logger.Log.Printf("CreateInference: JobType: %s, parsed partInfo : ", JobInfo.FlSetting)
	logger.Log.Println(inferencePartyInfo)

	addresses := common.ParseAddress(inferencePartyInfo)

	dslOjb := new(cache.DslObj)
	dslOjb.PartyAddrList = addresses
	dslOjb.JobId = inference.ID
	dslOjb.JobName = JobInfo.JobName
	dslOjb.JobFlType = JobInfo.FlSetting
	dslOjb.ExistingKey = JobInfo.ExistingKey
	dslOjb.PartyNums = JobInfo.PartyNum
	dslOjb.PartyInfoList = inferencePartyInfo
	dslOjb.Tasks = TaskInfo

	go func() {
		defer logger.HandleErrors()
		dist.SetupDist(dslOjb, common.InferenceWorker)
	}()

	return true, inference.ID

}

func QueryRunningInferenceJobs(jobName string, ctx *entity.Context) []uint {

	InferenceIds, e := ctx.JobDB.InferenceGetCurrentRunningOneWithJobName(jobName, ctx.UsrId)

	if e != nil {
		panic("QueryRunningJobs Error")
	}

	return InferenceIds
}

func InferenceUpdateStatus(jobId uint, status string, ctx *entity.Context) {
	tx := ctx.JobDB.DB.Begin()
	e, _ := ctx.JobDB.InferenceUpdateStatus(tx, jobId, status)
	ctx.JobDB.Commit(tx, e)

}

func InferenceUpdateMaster(jobId uint, masterAddr string, ctx *entity.Context) {
	tx := ctx.JobDB.DB.Begin()
	e, _ := ctx.JobDB.InferenceUpdateMaster(tx, jobId, masterAddr)
	ctx.JobDB.Commit(tx, []error{e})
}

func UpdateInference(newInfId uint, InferenceIds []uint, ctx *entity.Context) {

	MaxCheck := 10
loop:
	for {
		if MaxCheck < 0 {
			logger.Log.Println("[UpdateInference]: Update Failed, latest job is not running")
			break
		}

		e, u := ctx.JobDB.InferenceGetByID(newInfId)
		if e != nil {
			panic(e)
		}

		// if the latest inference is running, stop the old one
		if u.Status == common.JobRunning {
			for _, infId := range InferenceIds {

				e, u := ctx.JobDB.InferenceGetByID(infId)
				if e != nil {
					panic(e)
				}

				dist.KillJob(u.MasterAddr, common.Network)

				tx := ctx.JobDB.DB.Begin()
				e, _ = ctx.JobDB.InferenceUpdateStatus(tx, infId, common.JobKilled)
				ctx.JobDB.Commit(tx, e)
			}
			logger.Log.Println("[UpdateInference]: Update successfully")
			break loop
		} else {
			MaxCheck--
		}
		// check db every 3 second
		time.Sleep(time.Second * 3)
	}

}
