package tasks

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/comms_pattern"
	"falcon_platform/jobmanager/entity"
	"falcon_platform/logger"
	"fmt"
	"os"
	"os/exec"
	"strconv"
	"strings"
)

// init register all existing tasks.
func init() {
	allTasks = GetAllTasks()
	allTasks[common.MpcTaskKey] = new(MpcTask)
}

// MpcTask used to run the Mpc tasks
type MpcTask struct {
	TaskAbstract
}

// GetCommand
// * @Description
// * @Date 2:52 下午 12/12/20
// * @Param
//	./semi-party.x -F -N 3 -p 0 -I -h 10.0.0.33 -pn 6000 algorithm_name
//		-N party_num
//		-p party_id
//		-h IP
//		-pn port
//		-ip: network file path
//		algorithm_name
//	-h is IP of part-0, all semi-party use the same port
//	-h each mpc process only requires IP of party-0
//	-h 是party_0的IP 端口目前只有一个 各个端口都相同就可以
//	-h 每个mpc进程的启动输入都是party_0的IP
//	-14000 端口用于和所有executor通信，默认是14000，
//	if there is -ip, no need host ip?
// * @return
func (this *MpcTask) GetCommand(taskInfo *entity.TaskContext) *exec.Cmd {

	wk := taskInfo.Wk
	fLConfig, err := comms_pattern.DeserializeFLNetworkCfg([]byte(taskInfo.FLNetworkCfg))
	if err != nil {
		panic("Decode task error")
	}
	//job := taskInfo.Job

	partyId := strconv.Itoa(int(wk.PartyID))
	partyNum := strconv.Itoa(int(taskInfo.Job.PartyNums))
	mpcPairNetworkCfg := fLConfig.MpcPairNetworkCfg[wk.WorkerID]
	mpcExecutorNetworkCfg := fLConfig.MpcExecutorNetworkCfg[wk.WorkerID][wk.PartyIndex]
	algName := taskInfo.MpcAlgName

	logger.Log.Println("[TrainWorker]: begin tasks mpc")

	tmpThirdPathList := strings.Split(common.MpcExePath, "/")
	thirdMPSPDZPath := strings.Join(tmpThirdPathList[:len(tmpThirdPathList)-1], "/")
	logger.Log.Printf("[TrainWorker]: Third Party base path for executing semi-party.x is : \"%s\"\n", thirdMPSPDZPath)

	var cmd *exec.Cmd

	// write file to local disk, input path to mpc process
	mpcExecutorPairNetworkCfgPath := fmt.Sprintf("/opt/falcon/third_party/MP-SPDZ/mpc-network-%s", partyId)

	logger.Log.Printf("[TrainWorker]: Config mpc-network file, "+
		"write [%s] to file [%s] \n", mpcPairNetworkCfg, mpcExecutorPairNetworkCfgPath)

	tmpFile, err := os.Create(mpcExecutorPairNetworkCfgPath)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: create file error, %s \n", mpcExecutorPairNetworkCfgPath)
		//panic(err)
	}
	_, err = tmpFile.WriteString(mpcPairNetworkCfg)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: write file error, %s \n", mpcExecutorPairNetworkCfgPath)
		//panic(err)
	}
	_ = tmpFile.Close()

	// write file to local disk, input path to mpc process
	mpcExecutorPortFile := common.MpcExecutorPortFileBasePath + algName
	logger.Log.Printf("[TrainWorker]: Config mpcExecutorPortFile file, "+
		"write [%s] to file [%s] \n", mpcExecutorNetworkCfg, mpcExecutorPortFile)

	tmpFile, err = os.Create(mpcExecutorPortFile)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: create file error, %s \n", mpcExecutorPortFile)
		//panic(err)
	}
	_, err = tmpFile.WriteString(mpcExecutorNetworkCfg)
	if err != nil {
		logger.Log.Printf("[TrainWorker]: write file error, %s \n", mpcExecutorPortFile)
		//panic(err)
	}
	_ = tmpFile.Close()

	logger.Log.Printf("[TrainWorker]: Config mpcExecutorPairNetworkCfgPath file, "+
		"write port [%s] to file [%s] \n", mpcExecutorNetworkCfg, mpcExecutorPortFile)

	cmd = exec.Command(
		common.MpcExePath,
		"-F",
		"-N", partyNum,
		"-p", partyId,
		"-I",
		"-ip", mpcExecutorPairNetworkCfgPath, //  which is network file path
		algName,
	)
	cmd.Dir = thirdMPSPDZPath

	logger.Log.Printf("---------------------------------------------------------------------------------\n")
	logger.Log.Printf("[TrainWorker]: cmd is \"%s\"\n", cmd.String())
	logger.Log.Printf("---------------------------------------------------------------------------------\n")

	return cmd
}

func (this *MpcTask) GetRpcCallMethodName() string {
	return "RunMpc"
}
