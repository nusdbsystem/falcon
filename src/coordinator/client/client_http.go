package client

import (
	"coordinator/common"
	"fmt"
	"net/url"
	"strings"
)

func PartyServerAdd(ServerUrl, partyserverUrl, partyserverPort string) error {
	data := url.Values{
		common.PartyServerUrlKey: {partyserverUrl},
		common.PartyServerPortKey: {partyserverPort},
		}

	reqUrl := ServerUrl + "/" + common.PartyServerAdd

	e := PostForm(reqUrl, data)
	return e

}

func PartyServerDelete(ServerUrl, partyserverUrl string) {
	data := url.Values{common.PartyServerUrlKey: {partyserverUrl}}

	reqUrl := ServerUrl + "/" + common.PartyServerDelete

	_ = PostForm(reqUrl, data)
}

func SetupWorker(ServerUrl string, masterUrl string, workerType string, jobId,dataPath,modelPath,dataOutput string) {
	data := url.Values{
		common.MasterUrlKey: {masterUrl},
		common.TaskTypeKey:   {workerType},
		common.JobId:   {jobId},
		common.TrainDataPath : {dataPath},
		common.ModelPath : {modelPath},
		common.TrainDataOutput : {dataOutput},
	}

	reqUrl := ServerUrl + "/" + common.SetupWorker

	_ = PostForm(reqUrl, data)
}

func JobUpdateMaster(ServerUrl string, masterUrl string, jobId uint) {

	data := url.Values{
		common.MasterUrlKey: {masterUrl},
		common.JobId:      {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerUrl + "/" + common.UpdateJobMaster

	_ = PostForm(reqUrl, data)
}

func JobUpdateResInfo(ServerUrl string, errorMsg, jobResult, extInfo string, jobId uint) {
	data := url.Values{
		common.JobId:      {fmt.Sprintf("%d", jobId)},
		common.JobErrMsg:  {errorMsg},
		common.JobResult:  {jobResult},
		common.JobExtInfo: {extInfo},
	}

	reqUrl := ServerUrl + "/" + common.UpdateJobResInfo

	_ = PostForm(reqUrl, data)
}

func JobUpdateStatus(ServerUrl string, status uint, jobId uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.JobStatus: {fmt.Sprintf("%d", status)}}

	reqUrl := ServerUrl + "/" + common.UpdateJobStatus

	_ = PostForm(reqUrl, data)
}


func ModelUpdate(ServerUrl string, isTrained uint, jobId uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.IsTrained: {fmt.Sprintf("%d", isTrained)}}

	reqUrl := ServerUrl + "/" + common.ModelUpdate

	_ = PostForm(reqUrl, data)
}


func ModelServiceUpdateStatus(ServerUrl string, jobId, status uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.JobStatus: {fmt.Sprintf("%d", status)}}

	reqUrl := ServerUrl + "/" + common.UpdateModelServiceStatus

	_ = PostForm(reqUrl, data)
}

////////////////////////////////////
/////////// Get  ////////////
////////////////////////////////////
func JobGetStatus(ServerUrl string, jobId uint) uint {

	data := url.Values{
		common.JobId: {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerUrl + "/" + common.UpdateJobStatus

	_ = PostForm(reqUrl, data)
	return 1
}


func GetFreePort(ServerUrl string) string{

	reqUrl := ServerUrl + "/" + common.AssignPort
	reqUrl = "http://" + strings.TrimSpace(reqUrl)
	port := Get(reqUrl)

	return port
}

func GetExistPort(ServerUrl, PartyServerIp string) string{
	params := url.Values{}
	params.Set(common.PartyServerUrlKey, PartyServerIp)

	rawUrl := "http://" + strings.TrimSpace(ServerUrl) + "/" + common.GetPartyServerPort

	reqURL, err := url.ParseRequestURI(rawUrl)
	if err != nil {
		fmt.Printf("url.ParseRequestURI()函数执行错误,错误为:%v\n", err)
		return err.Error()
	}

	//3.整合请求URL和参数
	//Encode方法将请求参数编码为url编码格式("bar=baz&foo=quux")，编码时会以键进行排序。
	reqURL.RawQuery = params.Encode()

	//4.发送HTTP请求
	//说明: reqURL.String() String将URL重构为一个合法URL字符串。

	port := Get(reqURL.String())

	return port
}


func AddPort(ServerUrl, port string) error{

	data := url.Values{common.AddPort: {port}}

	reqUrl := ServerUrl + "/" + common.AddPort

	e := PostForm(reqUrl, data)
	return e
}