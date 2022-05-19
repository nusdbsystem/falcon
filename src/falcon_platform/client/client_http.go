// Those are communications between components of the platform, server and client must be correct
// each post and get has retry 5 time, if still fail, throw the error, catch it at main.
package client

import (
	"encoding/json"
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"io/ioutil"
	"net/url"
	"strings"
)

// add partyServer's address to
func PartyServerAdd(ServerAddr, partyserverAddr, partyserverPort string) {
	data := url.Values{
		common.PartyServerAddrKey: {partyserverAddr},
		common.PartyServerPortKey: {partyserverPort},
	}

	reqUrl := ServerAddr + common.PartyServerAdd

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'PartyServerAdd' Error, %s\n", err))
	}
}

func PartyServerDelete(ServerAddr, partyserverAddr string) {
	data := url.Values{common.PartyServerAddrKey: {partyserverAddr}}

	reqUrl := ServerAddr + common.PartyServerDelete

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'PartyServerDelete' Error, %s\n", err))
	}
}

func RunWorker(ServerAddr string, masterAddr string,
	workerType string,
	jobId string,
	dataPath, modelPath, dataOutput string,
	enableDistributedTrain int,
	workerPreGroup, partyNum, workerGroupNum int, stageName string, classID int,
) []byte {
	data := url.Values{
		common.MasterAddrKey:          {masterAddr},
		common.TaskTypeKey:            {workerType},
		common.JobId:                  {jobId},
		common.TrainDataPath:          {dataPath},
		common.ModelPath:              {modelPath},
		common.TrainDataOutput:        {dataOutput},
		common.EnableDistributedTrain: {fmt.Sprintf("%d", enableDistributedTrain)},
		common.WorkerPreGroup:         {fmt.Sprintf("%d", workerPreGroup)},
		common.TotalPartyNumber:       {fmt.Sprintf("%d", partyNum)},
		common.WorkerGroupNumber:      {fmt.Sprintf("%d", workerGroupNum)},
		common.StageNameKey:           {fmt.Sprintf("%s", stageName)},
		common.ClassIDKey:             {fmt.Sprintf("%d", classID)},
	}

	reqUrl := ServerAddr + common.RunWorker
	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'RunWorker' Error, %s\n", err))
	}
	resStr, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		panic(fmt.Sprintf("[Client]: Read 'RunWorker'reply' Error, %s\n", err))
	}
	return resStr
}

func JobUpdateMaster(ServerAddr string, masterAddr string, jobId uint) {

	data := url.Values{
		common.MasterAddrKey: {masterAddr},
		common.JobId:         {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + common.UpdateTrainJobMaster

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'JobUpdateMaster' Error, %s\n", err))
	}
}

func InferenceUpdateMaster(ServerAddr string, masterAddr string, jobId uint) {

	data := url.Values{
		common.MasterAddrKey: {masterAddr},
		common.JobId:         {fmt.Sprintf("%d", jobId)}}

	reqUrl := ServerAddr + common.UpdateInferenceJobMaster

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'InferenceUpdateMaster' Error, %s\n", err))
	}
}

func JobUpdateResInfo(ServerAddr string, errorMsg, jobResult, extInfo string, jobId uint) {
	data := url.Values{
		common.JobId:      {fmt.Sprintf("%d", jobId)},
		common.JobErrMsg:  {errorMsg},
		common.JobResult:  {jobResult},
		common.JobExtInfo: {extInfo},
	}

	reqUrl := ServerAddr + common.UpdateJobResInfo

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'JobUpdateResInfo' Error, %s\n", err))
	}
}

func JobUpdateStatus(ServerAddr string, status string, jobId uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.JobStatus: {status}}

	reqUrl := ServerAddr + common.UpdateJobStatus

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'JobUpdateStatus' Error, %s\n", err))
	}
}

func ModelUpdate(ServerAddr string, isTrained uint, jobId uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.IsTrained: {fmt.Sprintf("%d", isTrained)}}

	reqUrl := ServerAddr + common.ModelUpdate

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'ModelUpdate' Error, %s\n", err))
	}
}

func InferenceUpdateStatus(ServerAddr string, status string, jobId uint) {

	data := url.Values{
		common.JobId:     {fmt.Sprintf("%d", jobId)},
		common.JobStatus: {status}}

	reqUrl := ServerAddr + common.InferenceStatusUpdate

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			_ = resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'InferenceUpdateStatus' Error, %s\n", err))
	}
}

func AddPort(ServerAddr, port string) {

	data := url.Values{common.AddPort: {port}}

	reqUrl := ServerAddr + common.AddPort

	resp, err := PostForm(reqUrl, data)
	defer func() {
		if resp != nil {
			resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		//panic(fmt.Sprintf("[Client]: Requesting 'AddPort' Error, %s\n", err))
		logger.Log.Println("[Client]: Requesting 'AddPort' Error, %s, it probably already exist. \n", err)
	}
}

////////////////////////////////////
/////////// Get  ////////////
////////////////////////////////////
func GetFreePort(ServerAddr string, portNum int) []common.PortType {

	params := url.Values{}

	reqUrl := "http://" + strings.TrimSpace(ServerAddr+"/"+common.AssignPort)
	Url, err := url.Parse(reqUrl)

	if err != nil {
		panic(err)
	}
	params.Set("portNum", fmt.Sprintf("%d", portNum))

	Url.RawQuery = params.Encode()
	urlStr := Url.String()

	resp, err := Get(urlStr)

	defer func() {
		if resp != nil {
			resp.Body.Close()
		}
	}()
	if err != nil || resp.StatusCode != 200 {
		panic(fmt.Sprintf("[Client]: Requesting 'AssignPort' Error, %s\n", err))
	}
	resStr, err := ioutil.ReadAll(resp.Body)

	if err != nil {
		panic(fmt.Sprintf("[Client]: Read 'AssignPort'reply' Error, %s\n", err))
	}

	var sli = make([]common.PortType, portNum)

	err = json.Unmarshal(resStr, &sli)
	if err != nil {
		panic(fmt.Sprintf("[Client]: Read 'AssignPort'reply' Error, %s\n", err))
	}

	return sli
}
