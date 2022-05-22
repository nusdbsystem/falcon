package DAGscheduler

import (
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

	// Training
	ModelTrainParallelism int

	// Sampling
	LimeInstanceSampleParallelism int

	// prediction
	LimeOriModelPredictionParallelism int
	LimeInstanceWeightParallelism     int
	// how many class running at the same time
	LimeClassParallelism            int
	LimeFeatureSelectionParallelism int
	// how many worker each class use
	LimeVFLModelTrainParallelism int

	IsValid bool
}

func NewParallelismSchedulePolicy(job *common.TrainJob) *ParallelismSchedulePolicy {
	sp := new(ParallelismSchedulePolicy)
	sp.IsValid = sp.generateNewPolicy(job)
	return sp
}

func (sp *ParallelismSchedulePolicy) generateNewPolicy(job *common.TrainJob) bool {

	// fetch user defined total worker number
	workerNum := job.DistributedTask.WorkerNumber

	sp.PreProcessingParallelism = 1
	sp.ModelTrainParallelism = 1
	sp.LimeInstanceSampleParallelism = 1

	// if only one stage
	if len(job.ExecutedTasks) == 1 {
		for k := range job.ExecutedTasks {
			sp.updateSingleStageParallelism(k, workerNum)
			logger.Log.Println("[JobManager]: schedule result = ", sp.toString())
		}
		return true
	} else {

		if job.DistributedTask.Average == 1 {
			averageWorker := (job.DistributedTask.WorkerNumber - 1) / int(1+1+job.ClassNum*(1+1))

			if averageWorker == 2 {
				averageWorker = 1
			}

			sp.LimeOriModelPredictionParallelism = averageWorker
			sp.LimeInstanceWeightParallelism = averageWorker
			sp.LimeFeatureSelectionParallelism = averageWorker
			sp.LimeVFLModelTrainParallelism = averageWorker

			sp.LimeClassParallelism = int(job.ClassNum)
			logger.Log.Println("[JobManager]: schedule result = ", sp.toString())
			return true

		} else {

			// if many stage. run scheduler
			cmd := exec.Command(
				"python3",
				"autoscale/KttScheduler.py",
				"-w", fmt.Sprintf("%d", workerNum),
				"-d", "100000",
				"-c", fmt.Sprintf("%d", job.ClassNum))

			out, err := cmd.Output()
			if err != nil {
				panic(err.Error())
			}
			result := strings.Split(strings.TrimSpace(string(out)), "\n")
			logger.Log.Println("[JobManager]: KttScheduler return with:", result)
			label := result[0]
			if strings.Contains(label, "OK") {
				var resultInt []int
				for _, ele := range result[1:] {
					re, _ := strconv.Atoi(ele)
					resultInt = append(resultInt, re)
				}
				sp.LimeOriModelPredictionParallelism = resultInt[0]
				sp.LimeInstanceWeightParallelism = resultInt[1]
				sp.LimeFeatureSelectionParallelism = resultInt[2]
				sp.LimeVFLModelTrainParallelism = resultInt[3]
				sp.LimeClassParallelism = int(job.ClassNum)
				logger.Log.Println("[JobManager]: schedule result = ", sp.toString())
				return true
			} else if strings.Contains(label, "ERROR") {
				logger.Log.Println("[JobManager]: schedule result = ", sp.toString())
				return false
			} else {
				panic("KttScheduler.py return un-recognized result")
			}
		}
	}
}

func (sp *ParallelismSchedulePolicy) updateSingleStageParallelism(stageName common.FalconTask, workerNum int) {
	if stageName == common.LimePredTaskKey {
		sp.LimeOriModelPredictionParallelism = workerNum
	}
	if stageName == common.LimeWeightTaskKey {
		sp.LimeInstanceWeightParallelism = workerNum
	}
	if stageName == common.LimeFeatureTaskKey {
		sp.LimeFeatureSelectionParallelism = workerNum
	}
	if stageName == common.LimeInterpretTaskKey {
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
