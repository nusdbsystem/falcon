//
// Created by wuyuncheng on 13/9/20.
//

#ifndef FALCON_INCLUDE_COMMON_H_
#define FALCON_INCLUDE_COMMON_H_

namespace falcon{
  #define PHE_FIXED_POINT_PRECISION 8
  #define PHE_MAXIMUM_FIXED_POINT_PRECISION 32
  #define PHE_FIXED_POINT_BASE 2
  #define PHE_EPSILON 1
  #define PHE_KEY_SIZE 1024
  #define SPDZ_FIXED_POINT_PRECISION 16
  #define SPDZ_PORT_BASE 14000
  #define SPDZ_PLAYER_PATH "Player-Data/" // deprecated
  #define PROTOBUF_SIZE_LIMIT 1024 * 1024 * 1024
  #define MAXIMUM_RAND_VALUE 32767
  #define ROUNDED_PRECISION 1e-3
  #define NETWORK_CONFIG_PROTO 1

  // for Logistic Regression
  #define RANDOM_SEED 42
  #define SPLIT_TRAIN_TEST_RATIO 0.8
  #define ACTIVE_PARTY_ID 0
  // default threshold for LR is 50%
  #define LOGREG_THRES 0.5

  enum FLSetting {HORIZONTAL_FL, VERTICAL_FL};
  enum PartyType {ACTIVE_PARTY, PASSIVE_PARTY};
  enum AlgorithmName {LR, DT};
  enum DatasetType {TRAIN, TEST, VALIDATE};
}

#endif //FALCON_INCLUDE_COMMON_H_
