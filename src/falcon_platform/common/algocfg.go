package common

// algorithm configurations

type LogisticRegression struct {
	// batch size in each iteration
	BatchSize int32 `json:"batch_size"`
	// maximum number of iterations for training
	MaxIteration int32 `json:"max_iteration"`
	// tolerance of convergence
	ConvergeThreshold float64 `json:"convergence_threshold"`
	// whether use regularization or not
	WithRegularization bool `json:"with_regularization"`
	// regularization parameter
	Alpha float64 `json:"alpha"`
	// learning rate for parameter updating
	LearningRate float64 `json:"learning_rate"`
	// decay rate for learning rate, following lr = lr0 / (1 + decay*t), t is #iteration
	Decay float64 `json:"decay"`
	// penalty method used, 'l1' or 'l2', default l2, currently support 'l2'
	Penalty string `json:"penalty"`
	// optimization method, default 'sgd', currently support 'sgd'
	Optimizer string `json:"optimizer"`
	// strategy for handling multi-class classification, default 'ovr', currently support 'ovr'
	MultiClass string `json:"multi_class"`
	// evaluation metric for training and testing, 'acc', 'auc', or 'ks', currently support 'acc'
	Metric string `json:"metric"`
	// differential privacy budget
	DifferentialPrivacyBudget float64 `json:"differential_privacy_budget"`
	// fit_bias term
	FitBias bool `json:"fit_bias"`
}

type DecisionTree struct {
	// type of the tree, 'classification' or 'regression'
	TreeType string `json:"tree_type"`
	// the function to measure the quality of a split 'gini' or 'entropy'
	Criterion string `json:"criterion"`
	// the strategy used to choose a split at each node, 'best' or 'random'
	SplitStrategy string `json:"split_strategy"`
	// the number of classes in the dataset, if regression, set to 1
	ClassNum int32 `json:"class_num"`
	// the maximum depth of the tree
	MaxDepth int32 `json:"max_depth"`
	// the maximum number of bins to split a feature
	MaxBins int32 `json:"max_bins"`
	// the minimum number of samples required to split an internal node
	MinSamplesSplit int32 `json:"min_samples_split"`
	// the minimum number of samples required to be at a leaf node
	MinSamplesLeaf int32 `json:"min_samples_leaf"`
	// the maximum number of leaf nodes
	MaxLeafNodes int32 `json:"max_leaf_nodes"`
	// a node will be split if this split induces a decrease of impurity >= this value
	MinImpurityDecrease float64 `json:"min_impurity_decrease"`
	// threshold for early stopping in tree growth
	MinImpuritySplit float64 `json:"min_impurity_split"`
	// differential privacy (DP) budget, 0 denotes not use DP
	DpBudget float64 `json:"dp_budget"`
}

type RandomForest struct {
	// number of trees in the forest
	NEstimator int32 `json:"n_estimator"`
	// sample rate for each tree in the forest
	SampleRate float64 `json:"sample_rate"`

	// This is decision tree builder params

	// type of the tree, 'classification' or 'regression'
	TreeType string `json:"tree_type"`
	// the function to measure the quality of a split 'gini' or 'entropy'
	Criterion string `json:"criterion"`
	// the strategy used to choose a split at each node, 'best' or 'random'
	SplitStrategy string `json:"split_strategy"`
	// the number of classes in the dataset, if regression, set to 1
	ClassNum int32 `json:"class_num"`
	// the maximum depth of the tree
	MaxDepth int32 `json:"max_depth"`
	// the maximum number of bins to split a feature
	MaxBins int32 `json:"max_bins"`
	// the minimum number of samples required to split an internal node
	MinSamplesSplit int32 `json:"min_samples_split"`
	// the minimum number of samples required to be at a leaf node
	MinSamplesLeaf int32 `json:"min_samples_leaf"`
	// the maximum number of leaf nodes
	MaxLeafNodes int32 `json:"max_leaf_nodes"`
	// a node will be split if this split induces a decrease of impurity >= this value
	MinImpurityDecrease float64 `json:"min_impurity_decrease"`
	// threshold for early stopping in tree growth
	MinImpuritySplit float64 `json:"min_impurity_split"`
	// differential privacy (DP) budget, 0 denotes not use DP
	DpBudget float64 `json:"dp_budget"`
}

type LinearRegression struct {
	// batch size in each iteration
	BatchSize int32 `json:"batch_size"`
	// maximum number of iterations for training
	MaxIteration int32 `json:"max_iteration"`
	// tolerance of convergence
	ConvergeThreshold float64 `json:"converge_threshold"`
	// whether use regularization or not
	WithRegularization bool `json:"with_regularization"`
	// regularization parameter
	Alpha float64 `json:"alpha"`
	// learning rate for parameter updating
	LearningRate float64 `json:"learning_rate"`
	// decay rate for learning rate, following lr = lr0 / (1 + decay*t), t is #iteration
	Decay float64 `json:"decay"`
	// penalty method used, 'l1' or 'l2', default l2, currently support 'l2'
	Penalty string `json:"penalty"`
	// optimization method, default 'sgd', currently support 'sgd'
	Optimizer string `json:"optimizer"`
	// evaluation metric for training and testing, 'mse'
	Metric string `json:"metric"`
	// differential privacy budget
	DifferentialPrivacyBudget float64 `json:"differential_privacy_budget"`
	// whether to fit the bias the term
	FitBias bool `json:"fit_bias"`
}

type GBDT struct {
	// number of estimators (note that the number of total trees in the model
	// does not necessarily equal to the number of estimators for classification)
	NEstimator int32 `json:"n_estimator"`
	// loss function to be optimized
	Loss string `json:"loss"`
	// learning rate shrinks the contribution of each tree
	LearningRate float64 `json:"learning_rate"`
	// the fraction of samples to be used for fitting individual base learners
	// default 1.0, reserved here for future usage
	Subsample float64 `json:"subsample"`
	// decision tree builder params (note that the tree type here may be changed
	// when building the gbdt model as they are all regression trees in gbdt)

	// This is decision tree builder params

	// type of the tree, 'classification' or 'regression'
	TreeType string `json:"tree_type"`
	// the function to measure the quality of a split 'gini' or 'entropy'
	Criterion string `json:"criterion"`
	// the strategy used to choose a split at each node, 'best' or 'random'
	SplitStrategy string `json:"split_strategy"`
	// the number of classes in the dataset, if regression, set to 1
	ClassNum int32 `json:"class_num"`
	// the maximum depth of the tree
	MaxDepth int32 `json:"max_depth"`
	// the maximum number of bins to split a feature
	MaxBins int32 `json:"max_bins"`
	// the minimum number of samples required to split an internal node
	MinSamplesSplit int32 `json:"min_samples_split"`
	// the minimum number of samples required to be at a leaf node
	MinSamplesLeaf int32 `json:"min_samples_leaf"`
	// the maximum number of leaf nodes
	MaxLeafNodes int32 `json:"max_leaf_nodes"`
	// a node will be split if this split induces a decrease of impurity >= this value
	MinImpurityDecrease float64 `json:"min_impurity_decrease"`
	// threshold for early stopping in tree growth
	MinImpuritySplit float64 `json:"min_impurity_split"`
	// differential privacy (DP) budget, 0 denotes not use DP
	DpBudget float64 `json:"dp_budget"`
}

type LimeSampling struct {
	// the instance index for explain
	ExplainInstanceIdx int32 `json:"explain_instance_idx"`
	// whether sampling around the above instance
	SampleAroundInstance bool `json:"sample_around_instance"`
	// number of total samples to be generated
	NumTotalSamples int32 `json:"num_total_samples"`
	// the sampling method, now only support "gaussian"
	SamplingMethod string `json:"sampling_method"`
	// generated samples save file
	GeneratedSampleFile string `json:"generated_sample_file"`
}

type LimeCompPrediction struct {
	// vertical original model name
	OriginalModelName string `json:"original_model_name"`
	// vertical original model saved file
	OriginalModelSavedFile string `json:"original_model_saved_file"`
	// type of model task, 'regression' or 'classification'
	ModelType string `json:"model_type"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `json:"class_num"`
	// generated samples save file
	GeneratedSampleFile string `json:"generated_sample_file"`
	// prediction save file
	ComputedPredictionFile string `json:"computed_prediction_file"`
}

// This message denotes the compute sample weights parameters
type LimeCompWeights struct {
	// the instance index for explain
	ExplainInstanceIdx int32 `json:"explain_instance_idx"`
	// generated samples save file
	GeneratedSampleFile string `json:"generated_sample_file"`
	// prediction save file
	ComputedPredictionFile string `json:"computed_prediction_file"`
	// whether it is pre-computed
	IsPrecompute bool `json:"is_precompute"`
	// number of samples to be generated or selected
	NumSamples int32 `json:"num_samples"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `json:"class_num"`
	// the metric for computing the distance, only "euclidean"
	DistanceMetric string `json:"distance_metric"`
	// kernel, similarity kernel that takes euclidean distances and kernel width
	// as input and outputs weights in (0,1). If not specified, default is exponential kernel
	Kernel string `json:"kernel"`
	// width for the kernel
	KernelWidth float64 `json:"kernel_width"`
	// sample weights file to be saved
	SampleWeightsFile string `json:"sample_weights_file"`
	// selected samples to be saved if is_precompute = true
	SelectedSamplesFile string `json:"selected_samples_file"`
	// selected predictions to be saved if is_precompute = true
	SelectedPredictionsFile string `json:"selected_predictions_file"`
}

// This message denotes the LIME feature selection parameters
type LimeFeatSel struct {
	// selected samples file
	SelectedSamplesFile string `json:"selected_samples_file"`
	// selected predictions file
	SelectedPredictionsFile string `json:"selected_predictions_file"`
	// the sample weights file
	SampleWeightsFile string `json:"sample_weights_file"`
	// number of samples generated or selected
	NumSamples int32 `json:"num_samples"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `json:"class_num"`
	// the label id to be explained
	ClassId int32 `json:"class_id"`
	// feature selection method, current options are 'pearson', 'lasso_path',
	FeatureSelection string `json:"feature_selection"`
	// number of features to be explained in the interpret model
	NumExplainedFeatures int32 `json:"num_explained_features"`
	// selected features to be saved
	SelectedFeaturesFile string `json:"selected_features_file"`
}

// This message denotes the LIME interpret model training parameters
type LimeInterpretLR struct {
	// selected data file, either selected_samples_file or selected_features_file
	SelectedDataFile string `json:"selected_data_file"`
	// selected predictions saved
	SelectedPredictionsFile string `json:"selected_predictions_file"`
	// the sample weights file
	SampleWeightsFile string `json:"sample_weights_file"`
	// number of samples generated or selected
	NumSamples int32 `json:"num_samples"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `json:"class_num"`
	// the label id to be explained
	ClassId int32 `json:"class_id"`
	// interpretable model name, linear_regression or decision_tree
	InterpretModelName string `json:"interpret_model_name"`
	// interpretable model params, should be serialized LinearRegressionParams or DecisionTreeParams
	InterpretModelParam LinearRegression `json:"interpret_model_param"`
	// explanation report
	ExplanationReport string `json:"explanation_report"`
}

// This message denotes the LIME interpret model training parameters
type LimeInterpretDT struct {
	// selected data file, either selected_samples_file or selected_features_file
	SelectedDataFile string `json:"selected_data_file"`
	// selected predictions saved
	SelectedPredictionsFile string `json:"selected_predictions_file"`
	// the sample weights file
	SampleWeightsFile string `json:"sample_weights_file"`
	// number of samples generated or selected
	NumSamples int32 `json:"num_samples"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `json:"class_num"`
	// the label id to be explained
	ClassId int32 `json:"class_id"`
	// interpretable model name, linear_regression or decision_tree
	InterpretModelName string `json:"interpret_model_name"`
	// interpretable model params, should be serialized LinearRegressionParams or DecisionTreeParams
	InterpretModelParam DecisionTree `json:"interpret_model_param"`
	// explanation report
	ExplanationReport string `json:"explanation_report"`
}
