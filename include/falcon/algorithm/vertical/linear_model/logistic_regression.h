//
// Created by wuyuncheng on 14/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model.h>

#include <vector>
#include <string>

class LogisticRegression : public Model {
 public:
  int batch_size;
  int max_iteration;
  int decay_sqrt;
  float converge_threshold;
  float alpha;
  float learning_rate;
  float decay;
  std::string penalty;
  std::string optimizer;
  std::string multi_class;
  std::string metric;

 private:
  int weight_size;
  EncodedNumber* local_weights;
};

#endif //FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_H_
