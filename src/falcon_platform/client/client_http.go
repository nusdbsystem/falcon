package client

import (
	"falcon_platform/common"
	"fmt"
	"net/url"
	"strings"
)

func PartyServerAdd(ServerAddr, partyserverAddr, partyserverPort string) error {
	data := url.Values{
		common.PartyServerAddrKey: {partyserverAddr},
		common.PartyServerPortKey: {partyserverPort},
	}

	reqUrl := ServerAddr + "/" + common.PartyServerAdd

	e := PostForm(reqUrl, data)
	return e

}

func PartyServerDelete(ServerAddr, partyserverAddr string) {
	data := url.Values{common.PartyServerAddrKey: {partyserverAddr}}

	reqUrl := ServerAddr + "/" + common.PartyServerDelete

	_ = PostForm(reqUrl, data)
}

func SetupWorker(ServerAddr string, masterAddr string, workerType string, jobId, dataPath, modelPath, dataOutput string) {
	data := url.Values{
		common.MasterAddrKey:   {masterAddr},
		common.TaskTypeKey:     {workerType},
		common.JobId:           {jobId},
		common.TrainDataPath:   {dataPath},
		common.ModelPath:       {modelPath},
		common.TrainDataOutput: {dataOutput},
	}

	reqUrl := ServerAddr + "/" + common.SetupWorker

	_ = PostForm(reqUrl, data)
}

func JobUpdateMaster(ServerAddr string, masterAddr string, jobId uint) {

	data := url.Values{
		common.MasterAddrKey: {masterAddr},
		common.JobId:         {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + "/" + common.UpdateTrainJobMaster

	_ = PostForm(reqUrl, data)
}

func InferenceUpdateMaster(ServerAddr string, masterAddr string, jobId uint) {

	data := url.Values{
		common.MasterAddrKey: {masterAddr},
		common.JobId:         {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + "/" + common.UpdateInferenceJobMaster

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

func InferenceUpdateStatus(ServerAddr string, jobId, status uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.JobStatus: {fmt.Sprintf("%d", status)}}

	reqUrl := ServerAddr + "/" + common.InferenceStatusUpdate

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

func GetFreePort(ServerAddr string) string {

	reqUrl := "http://" + strings.TrimSpace(ServerAddr+"/"+common.AssignPort)
	port := Get(reqUrl)

	return port
}

func GetExistPort(ServerAddr, PartyServerIP string) string {
	params := url.Values{}
	params.Set(common.PartyServerAddrKey, PartyServerIP)

	rawUrl := "http://" + strings.TrimSpace(ServerAddr) + "/" + common.GetPartyServerPort

	reqURL, err := url.ParseRequestURI(rawUrl)
	if err != nil {
		fmt.Printf("url.ParseRequestURI() error: :%v\n", err)
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

func AddPort(ServerAddr, port string) error {

	data := url.Values{common.AddPort: {port}}

	reqUrl := ServerAddr + "/" + common.AddPort

	e := PostForm(reqUrl, data)
	return e
}
