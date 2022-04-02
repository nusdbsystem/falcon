package DAGscheduler

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

func NewParallelismSchedulePolicy() *ParallelismSchedulePolicy {
	sp := new(ParallelismSchedulePolicy)
	sp.generateNewPolicy()
	return sp
}

func (sp *ParallelismSchedulePolicy) generateNewPolicy() {

	//cmd := exec.Command("../autoscale/KttScheduler.py")
	//out, err := cmd.Output()
	//if err != nil {
	//	println(err.Error())
	//	return
	//}
	//fmt.Println(string(out))

	sp.PreProcessingParallelism = 1
	sp.ModelTrainParallelism = 1

	sp.LimeInstanceSampleParallelism = 1
	sp.LimeOriModelPredictionParallelism = 3
	sp.LimeInstanceWeightParallelism = 3
	sp.LimeClassParallelism = 2
	sp.LimeFeatureSelectionParallelism = 3
	sp.LimeVFLModelTrainParallelism = 3
}
