package DAGscheduler

import (
	"falcon_platform/common"
	"falcon_platform/logger"
)

// define a type

type TaskStage struct {
	Name           common.FalconTask
	AssignedWorker int
}

type DagScheduler struct {
	// stage, each stage has many tasks,
	Stages            map[common.FalconTask]TaskStage
	ParallelismPolicy *ParallelismSchedulePolicy
}

func NewDagScheduler(job *common.TrainJob) *DagScheduler {
	ds := new(DagScheduler)
	ds.Stages = make(map[common.FalconTask]TaskStage)
	ds.ParallelismPolicy = NewParallelismSchedulePolicy(job)
	return ds
}

func (ds *DagScheduler) SplitTaskIntoStage(job *common.TrainJob) {

	// stage 1
	if job.Tasks.PreProcessing.AlgorithmName != "" {
		taskStage := TaskStage{
			common.PreProcTaskKey,
			ds.ParallelismPolicy.PreProcessingParallelism}
		ds.Stages[common.PreProcTaskKey] = taskStage
	}

	// stage 2
	if job.Tasks.ModelTraining.AlgorithmName != "" {
		taskStage := TaskStage{
			common.ModelTrainTaskKey,
			ds.ParallelismPolicy.ModelTrainParallelism}
		ds.Stages[common.ModelTrainTaskKey] = taskStage
	}

	// stage 3
	if job.Tasks.LimeInsSample.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimeInstanceSampleTask,
			ds.ParallelismPolicy.LimeInstanceSampleParallelism}
		ds.Stages[common.LimeInstanceSampleTask] = taskStage
	}

	// stage 4
	if job.Tasks.LimePred.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimePredTaskKey,
			ds.ParallelismPolicy.LimeOriModelPredictionParallelism}
		ds.Stages[common.LimePredTaskKey] = taskStage
	}

	// stage 5
	if job.Tasks.LimeWeight.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimeWeightTaskKey,
			ds.ParallelismPolicy.LimeInstanceWeightParallelism}
		ds.Stages[common.LimeWeightTaskKey] = taskStage
	}

	// stage 6
	if job.Tasks.LimeFeature.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimeFeatureTaskKey,
			ds.ParallelismPolicy.LimeFeatureSelectionParallelism}
		ds.Stages[common.LimeFeatureTaskKey] = taskStage

	}

	if job.Tasks.LimeInterpret.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimeInterpretTaskKey,
			ds.ParallelismPolicy.LimeVFLModelTrainParallelism}

		ds.Stages[common.LimeInterpretTaskKey] = taskStage
	}

	logger.Log.Printf("[DagScheduler] sages = %v\n", ds.Stages)
}
