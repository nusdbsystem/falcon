package test

import (
	"falcon_platform/common"
	"falcon_platform/jobmanager/comms_pattern"
	"falcon_platform/jobmanager/entity"
	"fmt"
	"testing"
)

func TestSerialize(t *testing.T) {

	job := common.TrainJob{}
	job.JobFlType = "vertical"

	JobNetCfgIns := comms_pattern.GetJobNetCfg()[job.JobFlType]

	wk := &entity.WorkerInfo{"worker0Addr", 0, 8, 0}

	generalTask := &entity.TaskContext{TaskName: common.MpcTaskKey,
		Wk:           wk,
		FLNetworkCfg: JobNetCfgIns.SerializeNetworkCfg(),
		Job:          &job,
		MpcAlgName:   "mpcAlgorithmName",
	}

	args := string(entity.SerializeTask(generalTask))

	taskArg, err := entity.DeserializeTask([]byte(args))
	if err != nil {
		panic("Parser doTask args fails")
	}
	fmt.Println(taskArg)
	res, err := comms_pattern.DeserializeFLNetworkCfg([]byte(taskArg.FLNetworkCfg))
	if err != nil {
		panic(err)
	}
	fmt.Println(res)
}
