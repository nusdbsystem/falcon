package common



type LogisticRegression struct {
	// batch size in each iteration
	BatchSize int32 `json:"batch_size"`
	// maximum number of iterations for training
	MaxIteration int32 `json:"max_iteration"`
	// tolerance of convergence
	ConvergeThreshold float32 `json:"convergence_threshold"`
	// whether use regularization or not
	WithRegularization int32 `json:"with_regulartization"`
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