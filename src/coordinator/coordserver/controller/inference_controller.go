package controller

import (
	"coordinator/cache"
	"coordinator/common"
	"coordinator/coordserver/entity"
	dist "coordinator/distributed"
	"coordinator/logger"
	"encoding/json"
)




func CreateInference(inferenceJob common.InferenceJob, ctx *entity.Context) (bool, uint) {

	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()

	e1, trainJob := ctx.JobDB.JobGetByJobID(inferenceJob.JobId)

	e2, JobInfo := ctx.JobDB.JobInfoGetById(trainJob.JobInfoID)
	e3, model := ctx.JobDB.ModelGetByID(inferenceJob.JobId)
	ctx.JobDB.Commit([]error{e1, e2, e3})

	// if train is not finished, return
	if model.IsTrained == 0{
		return false, 0
	}

	// if train is not finished, else create a inference job
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e4, inference := ctx.JobDB.CreateInference(model.ID, inferenceJob.JobId)
	ctx.JobDB.Commit(e4)

	var pInfo []common.PartyInfo
	var TaskInfo common.Tasks

	err := json.Unmarshal([]byte(JobInfo.PartyIds), &pInfo)
	err2 := json.Unmarshal([]byte(JobInfo.TaskInfo), &TaskInfo)
	if err != nil || err2!=nil {
		panic("json.Unmarshal(PartyIds or TaskInfo) error")
	}

	var inferencePartyInfo []common.PartyInfo

	DataInfo := inferenceJob.DataInfo

	if JobInfo.FlSetting == common.HorizontalFl {
		for _, info := range DataInfo{
			for _, v := range pInfo{

				// only deploy inference worker at party 0
				if info.ID == v.ID && info.ID == 0{
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
	}else if JobInfo.FlSetting == common.VerticalFl{
		for _, info := range DataInfo{
			for _, v := range pInfo{

				if info.ID == v.ID{
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

	logger.Do.Printf("CreateInference: JobType: %s, parsed partInfo : ",JobInfo.FlSetting)
	logger.Do.Println(inferencePartyInfo)

	addresses := common.ParseAddress(inferencePartyInfo)

	qItem := new(cache.QItem)
	qItem.AddrList = addresses
	qItem.JobId = inference.ID
	qItem.JobName = JobInfo.JobName
	qItem.JobFlType = JobInfo.FlSetting
	qItem.ExistingKey = JobInfo.ExistingKey
	qItem.PartyNums = JobInfo.PartyNum
	qItem.PartyInfo = inferencePartyInfo
	qItem.Tasks = TaskInfo

	go func(){
		defer logger.HandleErrors()
		dist.SetupDist(qItem, common.InferenceWorker)
	}()

	return true, inference.ID

}


func QueryRunningInferenceJobs(jobName string, ctx *entity.Context) []uint{

	InferenceIds, e := ctx.JobDB.InferenceGetCurrentRunningOneWithJobName(jobName, ctx.UsrId)

	if e !=nil{
		panic("QueryRunningJobs Error")
	}

	return InferenceIds
}


func InferenceUpdateStatus(jobId uint, status uint, ctx *entity.Context){
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, _ := ctx.JobDB.InferenceUpdateStatus(jobId, status)
	ctx.JobDB.Commit(e)

}

func InferenceUpdateMaster(jobId uint, masterAddr string, ctx *entity.Context) {
	ctx.JobDB.Tx = ctx.JobDB.Db.Begin()
	e, _ := ctx.JobDB.InferenceUpdateMaster(jobId, masterAddr)
	ctx.JobDB.Commit([]error{e})
}
