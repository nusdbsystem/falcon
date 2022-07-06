//
// Created by wuyuncheng on 13/9/20.
//

#ifndef FALCON_INCLUDE_COMMON_H_
#define FALCON_INCLUDE_COMMON_H_

namespace falcon {
  // for DEBUG to display intermediate information
  // turn DEBUG off in the real application
//  #define DEBUG
  // #define SAVE_BASELINE
  #define PRINT_EVERY 20  // if debug, display info at this frequency

  // Precision for transforming a floating-point value
  // encode a floating-point value into a big integer
  // the higher the precision, the more accurate
  // NOTE: cannot be too high (encoded <= long long)
  #define PHE_FIXED_POINT_PRECISION 16
  #define PHE_MAXIMUM_FIXED_POINT_PRECISION 32
  #define PHE_MAXIMUM_PRECISION 1800
  #define PHE_FIXED_POINT_BASE 2
  #define PHE_EPSILON 1
  #define PHE_KEY_SIZE 2048
  #define PHE_STR_BASE 10
  #define SPDZ_FIXED_POINT_PRECISION 16
  #define SPDZ_PLAYER_PATH "Player-Data/"  // deprecated
  #define PROTOBUF_SIZE_LIMIT 1024 * 1024 * 1024
  #define MAXIMUM_RAND_VALUE 32767
  #define ROUNDED_PRECISION 1e-3
  #define NETWORK_CONFIG_PROTO

  /** for linear models */
  #define SPDZ_PORT_BASE 14000
  #define RANDOM_SEED 51
  #define SPLIT_TRAIN_TEST_RATIO 0.8
  #define ACTIVE_PARTY_ID 0
  // default threshold for LR is 50%
  #define LOGREG_THRES 0.5
  // initialization of weights
  // commonly zero-initialized
  // sklearn default one-initialized
  #define WEIGHTS_INIT_MIN 0.0
  #define WEIGHTS_INIT_MAX 0.0
  // bias term value
  #define BIAS_VALUE 1.0

  /** for Decision Tree */
  enum TreeNodeType{ INTERNAL, LEAF };
  enum TreeFeatureType{ CONTINUOUS, CATEGORICAL };
  enum TreeType{ CLASSIFICATION, REGRESSION };
  enum SpdzTreeCompType{
    PRUNING_CHECK, // check if the node satisfies pruning conditions
    COMPUTE_LABEL, // when a leaf node reaches, compute its label
    FIND_BEST_SPLIT, // find the best split for a decision tree node
    GBDT_SQUARE_LABEL, // compute the square label for gbdt models
    GBDT_SOFTMAX, // compute the softmax predictions for each estimator
    RF_LABEL_MODE, // compute the mode of the predicted labels of the trees
    GBDT_EXPIT,  // compute the negative gradient for binomial deviance loss
    GBDT_UPDATE_TERMINAL_REGION // compute the terminal region labels
  };
  #define REGRESSION_TREE_CLASS_NUM 2
  #define SPDZ_PORT_TREE 18000
  #define MAX_IMPURITY 1.0
  // refers the maximum number of splits considered in a tree
  #define MAX_GLOBAL_SPLIT_NUM 6000

  /** for LIME **/
  enum SpdzLimeCompType {
    DIST_WEIGHT, // compute the sqrt distance and kernel weights
    PEARSON // compute the pearson coefficient
  };

  /** for MLP */
  enum SpdzMlpCompType {
    ACTIVATION,
    ACTIVATION_FAST
  };

  enum SpdzMlpActivationFunc {
    LOGISTIC,
    RELU,
    IDENTITY,
    SOFTMAX
  };

  enum SpdzLogRegCompType {
    LOG_FUNC,
    L1_REGULARIZATION
  };

  // for inference service
  # define DEFAULT_INFERENCE_ENDPOINT "localhost:8123"

  #define CERTAIN_PROBABILITY 1.0
  #define ZERO_PROBABILITY 0.0
  #define SMOOTHER 1e-3

  // horizontal FL: parties have the same feature and label space
  // vertical FL: parties have different feature spaces
  enum FLSetting { HORIZONTAL_FL, VERTICAL_FL };
  // when vertical fl is used,
  // active party has label and features while passive party only has features
  enum PartyType { ACTIVE_PARTY, PASSIVE_PARTY };
  // when distributed training is enabled
  enum ExecutorRole {DistPS, DistWorker};
  enum AlgorithmName {
    LOG_REG,
    LINEAR_REG,
    DT,
    RF,
    GBDT,
    MLP,
    LIME_SAMPLING,
    LIME_COMP_PRED,
    LIME_COMP_WEIGHT,
    FeatureSelection,
    LIME_FEAT_SEL,
    LIME_INTERPRET
  };
  enum DatasetType { TRAIN, TEST, VALIDATE };

  #define NUM_OMP_THREADS 6
  #define PARALLELISM_ENABLED true
}  // namespace falcon

#endif // FALCON_INCLUDE_COMMON_H_
