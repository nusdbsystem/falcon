package DAGscheduler

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/logger"
	"fmt"
	"os/exec"
	"strconv"
	"strings"
)

type ParallelismSchedulePolicy struct {

	// prediction
	PreProcessingParallelism int

	ModelTrainParallelism int

	LimeInstanceSampleParallelism int

	// prediction
	LimeOriModelPredictionParallelism int
	LimeInstanceWeightParallelism     int
	// how many class running at the same time
	LimeClassParallelism            int
	LimeFeatureSelectionParallelism int
	// how many worker each class use
	LimeVFLModelTrainParallelism int
}

func NewParallelismSchedulePolicy(dslOjb *cache.DslObj) *ParallelismSchedulePolicy {
	sp := new(ParallelismSchedulePolicy)
	sp.generateNewPolicy(dslOjb)
	return sp
}

func (sp *ParallelismSchedulePolicy) generateNewPolicy(dslOjb *cache.DslObj) {

	// fetch user defined total worker number
	workerNum := dslOjb.DistributedTask.WorkerNumber

	sp.PreProcessingParallelism = 1
	sp.ModelTrainParallelism = 1
	sp.LimeInstanceSampleParallelism = 1

	// if only one stage
	if len(dslOjb.Stages) == 1 {
		sp.updateSingleStageParallelism(dslOjb.Stages[0], workerNum)
	} else {
		// if many stage. run scheduler
		cmd := exec.Command("python3", "autoscale/KttScheduler.py", "--worker", fmt.Sprintf("%d", workerNum))
		out, err := cmd.Output()
		if err != nil {
			panic(err.Error())
		}
		result := strings.Split(strings.TrimSpace(string(out)), "\n")
		var resultInt []int
		for _, ele := range result {
			re, _ := strconv.Atoi(ele)
			resultInt = append(resultInt, re)
		}

		sp.LimeOriModelPredictionParallelism = resultInt[0]
		sp.LimeInstanceWeightParallelism = resultInt[1]
		sp.LimeFeatureSelectionParallelism = resultInt[2]
		sp.LimeVFLModelTrainParallelism = resultInt[3]
		sp.LimeClassParallelism = resultInt[4]
	}
	logger.Log.Println("[JobManager]: schedule result = ", sp.toString())
}

func (sp *ParallelismSchedulePolicy) updateSingleStageParallelism(stageName common.FalconStage, workerNum int) {
	if stageName == common.LimePredStage {
		sp.LimeOriModelPredictionParallelism = workerNum
	}
	if stageName == common.LimeWeightStage {
		sp.LimeInstanceWeightParallelism = workerNum
	}
	if stageName == common.LimeFeatureSelectionStage {
		sp.LimeFeatureSelectionParallelism = workerNum
	}
	if stageName == common.LimeVFLModelTrainStage {
		sp.LimeVFLModelTrainParallelism = workerNum
	}
}

func (sp *ParallelismSchedulePolicy) toString() string {
	return fmt.Sprintf("\nPreProcessingParallelism=%d\n"+
		"ModelTrainParallelism=%d\n"+
		"LimeInstanceSampleParallelism=%d\n"+
		"LimeOriModelPredictionParallelism=%d\n"+
		"LimeInstanceWeightParallelism=%d\n"+
		"LimeFeatureSelectionParallelism=%d\n"+
		"LimeVFLModelTrainParallelism=%d\n"+
		"LimeClassParallelism=%d\n",
		sp.PreProcessingParallelism,
		sp.ModelTrainParallelism,
		sp.LimeInstanceSampleParallelism, sp.LimeOriModelPredictionParallelism, sp.LimeInstanceWeightParallelism,
		sp.LimeFeatureSelectionParallelism, sp.LimeVFLModelTrainParallelism, sp.LimeClassParallelism,
	)
}
