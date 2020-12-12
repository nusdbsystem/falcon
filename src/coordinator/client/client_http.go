package client

import (
	"coordinator/common"
	"fmt"
	"net/url"
	"strings"
)

func ListenerAdd(ServerAddress, listenerAddr, listenerPort string) error {
	data := url.Values{
		common.ListenerAddr: {listenerAddr},
		common.ListenerPortKey: {listenerPort},
		}

	reqUrl := ServerAddress + "/" + common.ListenerAdd

	e := PostForm(reqUrl, data)
	return e

}

func ListenerDelete(ServerAddress, listenerAddr string) {
	data := url.Values{common.ListenerAddr: {listenerAddr}}

	reqUrl := ServerAddress + "/" + common.ListenerDelete

	_ = PostForm(reqUrl, data)
}

func SetupWorker(ServerAddr string, masterAddress string, taskType string, jobId,dataPath,modelPath,dataOutput string) {
	data := url.Values{
		common.MasterAddr: {masterAddress},
		common.TaskType:   {taskType},
		common.JobId:   {jobId},
		common.TrainDataPath : {dataPath},
		common.ModelPath : {modelPath},
		common.TrainDataOutput : {dataOutput},
	}

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


func GetFreePort(ServerAddr string) string{

	reqUrl := ServerAddr + "/" + common.AssignPort
	reqUrl = "http://" + strings.TrimSpace(reqUrl)
	port := Get(reqUrl)

	return port
}

func GetExistPort(ServerAddr, ListenerIp string) string{
	params := url.Values{}
	params.Set(common.ListenerAddr, ListenerIp)

	rawUrl := "http://" + strings.TrimSpace(ServerAddr) + "/" + common.GetListenerPort

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


func AddPort(ServerAddress, port string) error{

	data := url.Values{common.AddPort: {port}}

	reqUrl := ServerAddress + "/" + common.AddPort

	e := PostForm(reqUrl, data)
	return e
}