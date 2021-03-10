//
// Created by wuyuncheng on 11/12/20.
//

#include <cmath>
#include "falcon/utils/metric/accuracy.h"

#include <glog/logging.h>


float accuracy_computation(std::vector<int> predictions, std::vector<float> labels) {
  // LOG(INFO) << "--- accuracy_computation() called ---";
  // LOG(INFO) << "predictions vector size = " << predictions.size();
  // LOG(INFO) << "labels vector size = " << labels.size();
  if (predictions.size() != labels.size()) {
    LOG(ERROR) << "Accuracy computation wrong: sizes of the two vectors not same";
  }
  int total_num = predictions.size();
  int matched_num = 0;
  for (int i = 0; i < total_num; i++) {
    if (predictions[i] == (int) labels[i]) {
      matched_num += 1;
    }
  }
  return (float) matched_num / (float) total_num;
}
