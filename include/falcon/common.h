//
// Created by wuyuncheng on 13/9/20.
//

#ifndef FALCON_INCLUDE_COMMON_H_
#define FALCON_INCLUDE_COMMON_H_

namespace falcon {
// Precision for transforming a floating-point value
// encode a floating-point value into a big integer
// the higher the precision, the more accurate
// NOTE: cannot be too high (encoded <= long long)
#define PHE_FIXED_POINT_PRECISION 12
#define PHE_MAXIMUM_FIXED_POINT_PRECISION 32
#define PHE_FIXED_POINT_BASE 2
#define PHE_EPSILON 1
#define PHE_KEY_SIZE 1024
#define PHE_STR_BASE 10
#define SPDZ_FIXED_POINT_PRECISION 16
#define SPDZ_PLAYER_PATH "Player-Data/"  // deprecated
#define PROTOBUF_SIZE_LIMIT 1024 * 1024 * 1024
#define MAXIMUM_RAND_VALUE 32767
#define ROUNDED_PRECISION 1e-3
#define NETWORK_CONFIG_PROTO 1

  /** for Logistic Regression */
  #define SPDZ_PORT_BASE 14000
  #define RANDOM_SEED 42
  #define SPLIT_TRAIN_TEST_RATIO 0.8
  #define ACTIVE_PARTY_ID 0
  // default threshold for LR is 50%
  #define LOGREG_THRES 0.5
  // initialization of weights
  // commonly zero-initialized
  // sklearn default one-initialized
  #define WEIGHTS_INIT_MIN 0.0
  #define WEIGHTS_INIT_MAX 0.0

  /** for Decision Tree */
  enum TreeNodeType{INTERNAL, LEAF};
  enum TreeFeatureType{CONTINUOUS, CATEGORICAL};
  enum TreeType{CLASSIFICATION, REGRESSION};
  enum SpdzTreeCompType{PRUNING_CHECK, COMPUTE_LABEL, FIND_BEST_SPLIT,
      GBDT_SQUARE_LABEL, GBDT_SOFTMAX, RF_LABEL_MODE};
  #define REGRESSION_TREE_CLASS_NUM 2
  #define SPDZ_PORT_TREE 18000
  #define MAX_IMPURITY 100000.0
  // refers the maximum number of splits considered in a tree
  #define MAX_GLOBAL_SPLIT_NUM 6000

  // for inference service
  #define DEFAULT_INFERENCE_ENDPOINT "localhost:8123"
  #define IS_INFERENCE 1

// for DEBUG
// to display intermediate information
// turn DEBUG off in the real application
#define DEBUG true
#define PRINT_EVERY 500  // if debug, display info at this frequency
// gtest IO folder
#define TEST_IO_OUTDIR "/opt/falcon/test/falcon/io"

#define CERTAIN_PROBABILITY 1.0
#define ZERO_PROBABILITY 0.0

enum FLSetting { HORIZONTAL_FL, VERTICAL_FL };
enum PartyType { ACTIVE_PARTY, PASSIVE_PARTY };
enum AlgorithmName { LR, DT, RF };
enum DatasetType { TRAIN, TEST, VALIDATE };
}  // namespace falcon

#endif //FALCON_INCLUDE_COMMON_H_
