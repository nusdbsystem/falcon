// math operations

#include <falcon/common.h>
#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

#include <cmath>
#include <iomanip>   // std::setprecision
#include <iostream>  // std::cout
#include <map>
#include <numeric>
#include <falcon/utils/math/math_ops.h>
#include <queue>

inline void check_vectors(std::vector<double> a, std::vector<double> b,
                     const std::vector<double>& weights) {
  if (a.size() != b.size()) {
    LOG(ERROR) << "Mean squared error computation wrong: sizes of the two "
                  "vectors not same";
  }
  if (!weights.empty() && weights.size() != a.size()) {
    LOG(ERROR) << "Weights vector size not expected";
  }
}

double mean_squared_error(std::vector<double> a, std::vector<double> b,
                          const std::vector<double>& weights) {
  check_vectors(a, b, weights);
  int num = a.size();
  double squared_error = 0.0, mean_squared_error = 0.0;
  for (int i = 0; i < num; i++) {
    if (!weights.empty()) {
      squared_error = squared_error + weights[i] * (a[i] - b[i]) * (a[i] - b[i]);
    } else {
      squared_error = squared_error + (a[i] - b[i]) * (a[i] - b[i]);
    }
  }

  if (!weights.empty()) {
    double weights_sum = std::accumulate(weights.begin(), weights.end(),0.0);
    // mean_squared_error = squared_error / (num * weights_sum); // do not use num here
    mean_squared_error = squared_error / weights_sum;
  } else {
    mean_squared_error = squared_error / num;
  }
  return mean_squared_error;
}

double mean_squared_log_error(std::vector<double> a, std::vector<double> b,
                              const std::vector<double>& weights) {
  check_vectors(a, b, weights);
  int num = a.size();
  double squared_log_error = 0.0, mean_squared_log_error = 0.0;
  for (int i = 0; i < num; i++) {
    a[i] = log1p(a[i]);
    b[i] = log1p(b[i]);
  }
  return mean_squared_error(a, b, weights);
//  for (int i = 0; i < num; i++) {
//    // need to make sure value is less than negative 1
//    double log_error_i = log1p(a[i]) - log1p(b[i]);
//    if (!weights.empty()) {
//      squared_log_error = squared_log_error + weights[i] * log_error_i * log_error_i;
//    } else {
//      squared_log_error = squared_log_error + log_error_i * log_error_i;
//    }
//  }
//
//  if (!weights.empty()) {
//    double weights_sum = std::accumulate(weights.begin(), weights.end(),0.0);
//    mean_squared_log_error = squared_log_error / (num * weights_sum);
//  } else {
//    mean_squared_log_error = squared_log_error / num;
//  }
  return mean_squared_log_error;
}

double median_absolute_error(std::vector<double> a, std::vector<double> b) {
  check_vectors(a, b, std::vector<double>());
  int num = a.size();
  std::vector<double> absolute_errors;
  absolute_errors.reserve(num);
  for (int i = 0; i < num; i++) {
    absolute_errors.push_back(abs(a[i] - b[i]));
  }
  return median(absolute_errors);
}

double max_error(std::vector<double> a, std::vector<double> b) {
  check_vectors(a, b, std::vector<double>());
  int num = a.size();
  std::vector<double> errors;
  errors.reserve(num);
  for (int i = 0; i < num; i++) {
    errors.push_back(abs(a[i] - b[i]));
  }
  return *std::max_element(errors.begin(), errors.end());
}

bool rounded_comparison(double a, double b) {
  return (a >= b - ROUNDED_PRECISION) && (a <= b + ROUNDED_PRECISION);
}

std::vector<double> softmax(std::vector<double> inputs) {
  double sum = 0.0;
  for (int i = 0; i < inputs.size(); i++) {
    sum += inputs[i];
  }

  std::vector<double> probs;
  probs.reserve(inputs.size());
  for (int i = 0; i < inputs.size(); i++) {
    probs.push_back(inputs[i] / sum);
  }

  return probs;
}

double argmax(std::vector<double> inputs) {
  double index = 0, max = -1;
  for (int i = 0; i < inputs.size(); i++) {
    if (max < inputs[i]) {
      max = inputs[i];
      index = i;
    }
  }
  return index;
}

double logistic_function(double logit) {
  // Input logit to the logistic function
  // logistic function is a sigmoid function (S-shaped)
  // logistic function outputs an estimated probability between 0 and 1
  double est_prob;  // estimated probability
  est_prob = 1.0 / (1 + exp(0 - logit));
  return est_prob;
}

double logistic_regression_loss(std::vector<double> pred_probs,
                                std::vector<double> labels) {
  // the log taken here is with base e (natural log)
  // L = -(1/n)[\sum_{i=1}^{n} y_i \log{f} + (1-y_i) \log{1-f}]
  int n = pred_probs.size();
  // std::cout << "dataset size = " << n << std::endl;
  double loss_sum = 0.0;
  for (int i = 0; i < n; i++) {
    double loss_i = 0.0;
    // NOTE: log(0) will produce -inf, causing loss_sum to be nan
    // from sklearn's metrics/_classification.py:
    // "Log loss is undefined for p=0 or p=1, so probabilities are
    // clipped to max(eps, min(1 - eps, p))"
    // sklearn use eps=1e-15
    // fix by setting bounds of pred_probs between 0~1
    double eps = 1e-15;
    double UPPER = (1 - eps);
    double LOWER = eps;
    double est_prob = pred_probs[i];

    // Clipping
    if (est_prob > UPPER) {
      est_prob = UPPER;
    }
    if (est_prob < LOWER) {
      est_prob = LOWER;
    }
    // std::cout << "est_prob = " << std::setprecision(17) << est_prob << "\n";

    loss_i += (double)(labels[i] * log(est_prob));
    loss_i += (double)((1.0 - labels[i]) * log(1.0 - est_prob));
    loss_sum += loss_i;
    // if (i < 5) {
    //   std::cout << "predicted probability = " << std::setprecision(17) <<
    //   est_prob <<
    //     ", ground truth label = " << labels[i] << ", loss_sum = " <<
    //     std::setprecision(17) << loss_sum << std::endl;
    // }
  }
  double loss = (0 - loss_sum) / n;
  return loss;
}

double mode(const std::vector<double>& inputs) {
  // compute the frequency of the values
  std::map<double, int> m;
  for (auto v : inputs) {
    if (m.find(v) == m.end()) {
      m[v] = 1;
    } else {
      ++m[v];
    }
  }
  // compute mode
  double mode_value = 0.0;
  int maximum_votes = 0;
  // find the mode in result map
  for (auto it = m.begin(); it != m.end(); ++it) {
    if (it->second > maximum_votes) {
      maximum_votes = it->second;
      mode_value = it->first;
    }
  }
  return mode_value;
}

double median(std::vector<double>& inputs) {
  int vec_size = (int) inputs.size();
  double median_value = 0.0;
  int n = vec_size / 2;
  nth_element(inputs.begin(), inputs.begin() + n, inputs.end());

  if (vec_size % 2 == 0) {
    // return the average of the middle two
    median_value = median_value + inputs[n];
    nth_element(inputs.begin(), inputs.begin() + n - 1, inputs.end());
    median_value = median_value + inputs[n-1];
    median_value /= 2;
  } else {
    // return the middle one
    median_value = inputs[n];
  }
  return median_value;
}

double average(std::vector<double> inputs) {
  return std::accumulate(inputs.begin(), inputs.end(), 0.0) / inputs.size();
}

double square_sum(std::vector<double> a, std::vector<double> b) {
  double ss = 0.0;
  if (a.size() != b.size()) {
    LOG(ERROR) << "Squared sum computation wrong: sizes of the two "
                  "vectors not same";
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < (int) a.size(); i++) {
    double diff = a[i] - b[i];
    ss += (diff * diff);
  }
  return ss;
}

std::vector<int> find_top_k_indexes(const std::vector<double>& a, int k) {
  // from: https://stackoverflow.com/questions/14902876/indices-of-the-k-largest-elements-in-an-unsorted-length-n-array/23486017
  std::vector<int> indexes;
  std::priority_queue< std::pair<double, int>, std::vector< std::pair<double, int> >, std::greater <std::pair<double, int> > > q;
  for (int i = 0; i < a.size(); ++i) {
    if(q.size()<k)
      q.push(std::pair<double, int>(a[i], i));
    else if(q.top().first < a[i]){
      q.pop();
      q.push(std::pair<double, int>(a[i], i));
    }
  }
  k = q.size();
  std::vector<int> res(k);
  for (int i = 0; i < k; ++i) {
    res[k - i - 1] = q.top().second;
    q.pop();
  }
//  for (int i = 0; i < k; ++i) {
//    std::cout<< res[i] <<std::endl;
//  }
  return res;
}