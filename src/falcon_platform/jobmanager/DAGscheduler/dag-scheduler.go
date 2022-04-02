package DAGscheduler

import (
	"falcon_platform/cache"
	"falcon_platform/common"
	"falcon_platform/logger"
)

// define a type

type TaskStage struct {
	Name             common.FalconStage
	TasksParallelism map[common.FalconTask]int
	AssignedWorker   int
}

type DagScheduler struct {
	// stage, each stage has many tasks,
	Stages            map[common.FalconStage]TaskStage
	ParallelismPolicy *ParallelismSchedulePolicy
}

func NewDagScheduler(dslOjb *cache.DslObj) *DagScheduler {
	ds := new(DagScheduler)
	ds.Stages = make(map[common.FalconStage]TaskStage)
	ds.ParallelismPolicy = NewParallelismSchedulePolicy()
	ds.splitTaskIntoStage(dslOjb)
	return ds
}

func (ds *DagScheduler) splitTaskIntoStage(dslOjb *cache.DslObj) {

	// stage 1

	if dslOjb.Tasks.PreProcessing.AlgorithmName != "" {
		taskStage := TaskStage{
			common.PreProcStage,
			map[common.FalconTask]int{
				common.PreProcSubTask: ds.ParallelismPolicy.PreProcessingParallelism},
			ds.ParallelismPolicy.PreProcessingParallelism}
		ds.Stages[common.PreProcStage] = taskStage
	}

	// stage 2
	if dslOjb.Tasks.ModelTraining.AlgorithmName != "" {
		taskStage := TaskStage{
			common.ModelTrainStage,
			map[common.FalconTask]int{
				common.ModelTrainSubTask: ds.ParallelismPolicy.ModelTrainParallelism},
			ds.ParallelismPolicy.ModelTrainParallelism}
		ds.Stages[common.ModelTrainStage] = taskStage
	}

	// stage 3
	if dslOjb.Tasks.LimeInsSample.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimeInstanceSampleStage,
			map[common.FalconTask]int{
				common.LimeInstanceSampleTask: ds.ParallelismPolicy.LimeInstanceSampleParallelism},
			ds.ParallelismPolicy.LimeInstanceSampleParallelism}
		ds.Stages[common.LimeInstanceSampleStage] = taskStage
	}

	// stage 4
	if dslOjb.Tasks.LimePred.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimePredStage,
			map[common.FalconTask]int{
				common.LimePredSubTask: ds.ParallelismPolicy.LimeOriModelPredictionParallelism},
			ds.ParallelismPolicy.LimeOriModelPredictionParallelism}
		ds.Stages[common.LimePredStage] = taskStage
	}

	// stage 5
	if dslOjb.Tasks.LimeWeight.AlgorithmName != "" {
		taskStage := TaskStage{
			common.LimeWeightStage,
			map[common.FalconTask]int{
				common.LimeWeightSubTask: ds.ParallelismPolicy.LimeInstanceWeightParallelism},
			ds.ParallelismPolicy.LimeInstanceWeightParallelism}
		ds.Stages[common.LimeWeightStage] = taskStage
	}

	taskStage := TaskStage{
		common.LimeInterpretStage,
		make(map[common.FalconTask]int),
		0}

	// stage 6
	if dslOjb.Tasks.LimeFeature.AlgorithmName != "" {
		taskStage.TasksParallelism[common.LimeFeatureSubTask] = ds.ParallelismPolicy.LimeFeatureSelectionParallelism
		taskStage.AssignedWorker = ds.ParallelismPolicy.LimeFeatureSelectionParallelism
	}

	if dslOjb.Tasks.LimeInterpret.AlgorithmName != "" {
		taskStage.TasksParallelism[common.LimeInterpretSubTask] = ds.ParallelismPolicy.LimeVFLModelTrainParallelism
		taskStage.AssignedWorker = ds.ParallelismPolicy.LimeVFLModelTrainParallelism
	}

	ds.Stages[common.LimeInterpretStage] = taskStage

	logger.Log.Printf("[DagScheduler] sages = %v\n", ds.Stages)
}
