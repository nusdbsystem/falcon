package client

import (
	"coordinator/common"
	"fmt"
	"net/url"
)

func ListenerAdd(ServerAddress, listenerAddr string) {
	data := url.Values{common.ListenerAddr: {listenerAddr}}

	reqUrl := ServerAddress + "/" + common.ListenerAdd

	_ = PostForm(reqUrl, data)

}

func ListenerDelete(ServerAddress, listenerAddr string) {
	data := url.Values{common.ListenerAddr: {listenerAddr}}

	reqUrl := ServerAddress + "/" + common.ListenerDelete

	_ = PostForm(reqUrl, data)
}

func SetupWorker(ServerAddr string, masterAddress string, taskType string) {
	data := url.Values{
		common.MasterAddr: {masterAddress},
		common.TaskType:   {taskType}}

	reqUrl := ServerAddr + "/" + common.SetupWorker

	_ = PostForm(reqUrl, data)
}

func JobUpdateMaster(ServerAddr string, masterAddress string, jobId uint) {

	data := url.Values{
		common.MasterAddr: {masterAddress},
		common.JobId:      {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + "/" + common.UpdateJobMaster

	_ = PostForm(reqUrl, data)
}

func JobUpdateResInfo(ServerAddr string, errorMsg, jobResult, extInfo string, jobId uint) {
	data := url.Values{
		common.JobId:      {fmt.Sprintf("%d", jobId)},
		common.JobErrMsg:  {errorMsg},
		common.JobResult:  {jobResult},
		common.JobExtInfo: {extInfo},
	}

	reqUrl := ServerAddr + "/" + common.UpdateJobResInfo

	_ = PostForm(reqUrl, data)
}

func JobUpdateStatus(ServerAddr string, status uint, jobId uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.JobStatus: {fmt.Sprintf("%d", status)}}

	reqUrl := ServerAddr + "/" + common.UpdateJobStatus

	_ = PostForm(reqUrl, data)
}


func ModelUpdate(ServerAddr string, isTrained uint, jobId uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.IsTrained: {fmt.Sprintf("%d", isTrained)}}

	reqUrl := ServerAddr + "/" + common.ModelUpdate

	_ = PostForm(reqUrl, data)
}


func ModelServiceUpdateStatus(ServerAddr string, jobId, status uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.JobStatus: {fmt.Sprintf("%d", status)}}

	reqUrl := ServerAddr + "/" + common.UpdateModelServiceStatus

	_ = PostForm(reqUrl, data)
}

////////////////////////////////////
/////////// Get  ////////////
////////////////////////////////////
func JobGetStatus(ServerAddr string, jobId uint) uint {

	data := url.Values{
		common.JobId: {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + "/" + common.UpdateJobStatus

	_ = PostForm(reqUrl, data)
	return 1
}
