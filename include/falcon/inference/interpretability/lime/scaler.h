//
// Created by root on 12/22/21.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_SCALER_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_SCALER_H_

#include <vector>

class StandardScaler {
 public:
  // input parameters
  // whether given mean
  bool with_mean;
  // whether given standard deviation
  bool with_std;
  // compute scale parameters
  // number of features
  int feature_num{};
  // scale of features
  std::vector<double> scale;
  // mean value of features, equal to None when with_mean = False
  std::vector<double> mean;
  // variance of features, equal to None when with_std = False
  std::vector<double> variance;

 public:
  explicit StandardScaler(bool m_with_mean = true, bool m_with_std = true);
  ~StandardScaler();

  void fit(std::vector<std::vector<double>> train_data,
           std::vector<double> train_labels = std::vector<double>());
};

#endif //FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_SCALER_H_
