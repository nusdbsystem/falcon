// math operations


#include <cmath>
#include "falcon/utils/metric/accuracy.h"
#include <falcon/common.h>

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

#include <iostream>     // std::cout
#include <iomanip>      // std::setprecision


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
  // the log taken here is with base e (natural log)
  // L = -(1/n)[\sum_{i=1}^{n} y_i \log{f} + (1-y_i) \log{1-f}]
  int n = pred_probs.size();
  // std::cout << "dataset size = " << n << std::endl;
  float loss_sum = 0.0;
  for (int i = 0; i < n; i++) {
    float loss_i = 0.0;
    // NOTE: log(0) will produce -inf, causing loss_sum to be nan
    // fix by setting bounds of pred_probs between 0~1
    float UPPER = 0.99999;
    float LOWER = 0.00001;
    float est_prob;
    if (pred_probs[i] > UPPER) {
      est_prob = UPPER;
    } else if (pred_probs[i] < LOWER) {
      est_prob = LOWER;
    } else {
      est_prob = pred_probs[i];
    }
    // std::cout << "est_prob = " << std::setprecision(5) << est_prob << "\n";

    loss_i += (float) (labels[i] * log(est_prob));
    loss_i += (float) ((1.0 - labels[i]) * log(1.0 - est_prob));
    loss_sum += loss_i;
    // if (i < 5) {
    //   std::cout << "predicted probability = " << est_prob <<
    //     ", ground truth label = " << labels[i] << ", loss_sum = " << loss_sum << std::endl;
    // }
  }
  float loss = (0 - loss_sum) / n;
  return loss;
}
