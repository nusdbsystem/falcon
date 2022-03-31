package master

type LimeSchedulePolicy struct {
	// prediction
	OriModelPredictionParallelism int32
	instanceWeightParallelism     int32
	// how many class running at the same time
	classParallelism            int32
	featureSelectionParallelism int32
	// how many worker each class use
	VFLModelTrainParallelism int32
}

func newLimeSchedulePolicy() *LimeSchedulePolicy {
	sp := new(LimeSchedulePolicy)
	sp.generateNewPolicy()
	return sp
}

func (sp *LimeSchedulePolicy) generateNewPolicy() {

	sp.OriModelPredictionParallelism = 1
	sp.instanceWeightParallelism = 1
	sp.classParallelism = 1
	sp.featureSelectionParallelism = 1
	sp.VFLModelTrainParallelism = 1
}

func (sp *LimeSchedulePolicy) GetClassParallelism() int32 {
	return sp.classParallelism
}
