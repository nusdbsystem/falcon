//
// Created by wuyuncheng on 11/12/20.
//

#include <cmath>
#include "falcon/utils/metric/accuracy.h"
#include <falcon/common.h>

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

float mean_squared_error(std::vector<float> a, std::vector<float> b) {
  if (a.size() != b.size()) {
    LOG(ERROR) << "Mean squared error computation wrong: sizes of the two vectors not same";
  }

  int num = a.size();
  float squared_error = 0.0;
  for (int i = 0; i < num; i++) {
    squared_error = squared_error + (a[i] - b[i]) * (a[i] - b[i]);
  }

  float mean_squared_error = squared_error / num;
  return mean_squared_error;
}

bool rounded_comparison(float a, float b) {
  return (a >= b - ROUNDED_PRECISION) && (a <= b + ROUNDED_PRECISION);
}

std::vector<float> softmax(std::vector<float> inputs) {

  float sum = 0.0;
  for (int i = 0; i < inputs.size(); i++) {
    sum += inputs[i];
  }

  std::vector<float> probs;
  probs.reserve(inputs.size());
  for (int i = 0; i < inputs.size(); i++) {
    probs.push_back(inputs[i]/sum);
  }

  return probs;
}

float argmax(std::vector<float> inputs) {
  float index = 0, max = -1;
  for (int i = 0; i < inputs.size(); i++) {
    if (max < inputs[i]) {
      max = inputs[i];
      index = i;
    }
  }
  return index;
}

float accuracy_computation(std::vector<float> a, std::vector<float> b) {
  if (a.size() != b.size()) {
    LOG(ERROR) << "Accuracy computation wrong: sizes of the two vectors not same";
  }
  int total_num = a.size();
  int matched_num = 0;
  for (int i = 0; i < total_num; i++) {
    if (a[i] == b[i]) {
      matched_num += 1;
    }
  }
  return (float) matched_num / (float) total_num;
}

float logistic_function(float v) {
  return 1.0 / (1 + exp(0 - v));
}