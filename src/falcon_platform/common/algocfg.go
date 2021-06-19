package common

type LogisticRegression struct {
	// batch size in each iteration
	BatchSize int32 `json:"batch_size"`
	// maximum number of iterations for training
	MaxIteration int32 `json:"max_iteration"`
	// tolerance of convergence
	ConvergeThreshold float32 `json:"convergence_threshold"`
	// whether use regularization or not
	WithRegularization int32 `json:"with_regularization"`
	// regularization parameter
	Alpha float32 `json:"alpha"`
	// learning rate for parameter updating
	LearningRate float32 `json:"learning_rate"`
	// decay rate for learning rate, following lr = lr0 / (1 + decay*t), t is #iteration
	Decay float32 `json:"decay"`
	// penalty method used, 'l1' or 'l2', default l2, currently support 'l2'
	Penalty string `json:"penalty"`
	// optimization method, default 'sgd', currently support 'sgd'
	Optimizer string `json:"optimizer"`
	// strategy for handling multi-class classification, default 'ovr', currently support 'ovr'
	MultiClass string `json:"multi_class"`
	// evaluation metric for training and testing, 'acc', 'auc', or 'ks', currently support 'acc'
	Metric string `json:"metric"`
	// differential privacy budget
	DifferentialPrivacyBudget float32 `json:"differential_privacy_budget"`
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
