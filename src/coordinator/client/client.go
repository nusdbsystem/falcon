package client

import (
	"coordinator/config"
	"fmt"
	"net/url"
)

func ListenerAdd(ServerAddress, listenerAddr string) {
	data := url.Values{config.ListenerAddr: {listenerAddr}}

	reqUrl := ServerAddress + "/" + config.ListenerAdd

	_ = PostForm(reqUrl, data)

}

func ListenerDelete(ServerAddress, listenerAddr string) {
	data := url.Values{config.ListenerAddr: {listenerAddr}}

	reqUrl := ServerAddress + "/" + config.ListenerDelete

	_ = PostForm(reqUrl, data)
}

func SetupWorker(ServerAddr string, masterAddress string) {
	data := url.Values{config.MasterAddr: {masterAddress}}

	reqUrl := ServerAddr + "/" + config.SetupWorker

	_ = PostForm(reqUrl, data)
}

func JobUpdateMaster(ServerAddr string, masterAddress string, jobId uint) {

	data := url.Values{
		config.MasterAddr: {masterAddress},
		config.JobId:      {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + "/" + config.UpdateJobMaster

	_ = PostForm(reqUrl, data)
}

func JobUpdateResInfo(ServerAddr string, errorMsg, jobResult, extInfo string, jobId uint) {
	data := url.Values{
		config.JobId:      {fmt.Sprintf("%d", jobId)},
		config.JobErrMsg:  {errorMsg},
		config.JobResult:  {jobResult},
		config.JobExtInfo: {extInfo},
	}

	reqUrl := ServerAddr + "/" + config.UpdateJobResInfo

	_ = PostForm(reqUrl, data)
}

func JobUpdateStatus(ServerAddr string, status uint, jobId uint) {

	data := url.Values{
		config.JobId:     {fmt.Sprintf("%d", jobId)},
		config.JobStatus: {fmt.Sprintf("%d", status)}}

	reqUrl := ServerAddr + "/" + config.UpdateJobStatus

	_ = PostForm(reqUrl, data)
}

////////////////////////////////////
/////////// Get  ////////////
////////////////////////////////////
func JobGetStatus(ServerAddr string, jobId uint) uint {

	data := url.Values{
		config.JobId: {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + "/" + config.UpdateJobStatus

	_ = PostForm(reqUrl, data)
	return 1
}
