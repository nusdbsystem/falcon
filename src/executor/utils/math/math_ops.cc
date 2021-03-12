// math operations


#include <cmath>
#include "falcon/utils/metric/accuracy.h"
#include <falcon/common.h>

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

#include <iostream>


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


float logistic_function(float logit) {
  // Input logit to the logistic function
  // logistic function is a sigmoid function (S-shaped)
  // logistic function outputs an estimated probability between 0 and 1
  float est_prob;  // estimated probability
  est_prob =  1.0 / (1 + exp(0 - logit));
  return est_prob;
}


float logistic_regression_loss(std::vector<float> pred_probs, std::vector<float> labels) {
  // L = -(1/n)[\sum_{i=1}^{n} y_i \log{f} + (1-y_i) \log{1-f}]
  int n = pred_probs.size();
  // std::cout << "dataset size = " << n << std::endl;
  float loss_sum = 0.0;
  for (int i = 0; i < n; i++) {
    float loss_i = 0.0;
    loss_i += (float) (labels[i] * log(pred_probs[i]));
    loss_i += (float) ((1.0 - labels[i]) * log(1.0 - pred_probs[i]));
    loss_sum += loss_i;
//    if (i < 10) {
//      std::cout << "predicted probability = " << pred_probs[i] <<
//        ", ground truth label = " << labels[i] << ", loss_sum = " << loss_sum << std::endl;
//    }
  }
  float loss = (0 - loss_sum) / n;
  return loss;
}