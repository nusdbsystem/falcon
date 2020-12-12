package taskmanager

import (
	"coordinator/common"
	"os/exec"
)


func DoMPCTask(){

}


func DoMlTask(
	pm *SubProcessManager,

	partyId string,
	partyNum string,
	partyType string,
	flSetting string,
	existingKey string,

	netFile string,

) func(string, string, string,string, string,string, string,string) (string, bool, map[string]string ) {

	/**
	 * @Author
	 * @Description  record if the task is fail or not
	 * @Date 7:32 下午 12/12/20
	 * @Param key, algorithm name, value: error message
	 * @return
	 **/

	res := make(map[string]string)

	return func(
		algName string,
		algParams string,

		KeyFile string,
		logFile string,
		dataInputFile string,
		dataOutputFile string,
		modelSaveFile string,
		modelReport string,
		)(string, bool, map[string]string ){

		if algName==""{
			res[algName] = "skip"
			return common.SubProcessNormal, false, res
		}

		var envs []string

		cmd := exec.Command(
			common.FalconTrainExe,
			" --party-id "+partyId,
			" --party-num "+partyNum,
			" --party-type "+partyType,
			" --fl-setting "+flSetting,
			" --existing-key "+existingKey,
			" --key-file "+KeyFile,
			" --network-file "+netFile,

			" --algorithm-name "+algName,
			" --algorithm-params "+algParams,
			" --log-file "+logFile,
			" --data-input-file "+dataInputFile,
			" --data-output-file "+dataOutputFile,
			" --model-save-file "+modelSaveFile,
			" --model-report-file "+modelReport,
			)

		killed, err, el := pm.CreateResources(cmd, envs)

		res[algName] = el
		return err, killed, res
	}
}