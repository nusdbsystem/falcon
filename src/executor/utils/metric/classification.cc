//
// Created by wuyuncheng on 11/12/20.
//

#include <cmath>
#include "falcon/utils/metric/classification.h"

#include <glog/logging.h>
#include <iostream>


// a metrics class for classification performance

// Constructor with initilizer
ClassificationMetrics::ClassificationMetrics()
  : TP(0), FP(0), FN(0), TN(0) {}

// Compute the Metrics
void ClassificationMetrics::compute_metrics(std::vector<int> predictions, std::vector<float> labels)
   {
  std::cout << "ClassificationMetrics constructor called" << std::endl;
  // LOG(INFO) << "--- ClassificationMetrics constructor called ---";
  // LOG(INFO) << "predictions vector size = " << predictions.size();
  // LOG(INFO) << "labels vector size = " << labels.size();
  if (predictions.size() != labels.size()) {
    LOG(ERROR) << "Classification Metrics computation wrong: sizes of the two vectors not same";
  }
  int total_num = predictions.size();

  // prediction and label classes
  // for binary classifier, positive and negative classes
  int positive_class = 1;
  int negative_class = 0;

  for (int i = 0; i < total_num; i++) {
    if (predictions[i] == positive_class) {
      if ((int) labels[i] == positive_class) {
        TP += 1;
      }
      else if ((int) labels[i] == negative_class) {
        FP += 1;
      }
    }
    else if (predictions[i] == negative_class) {
      if ((int) labels[i] == positive_class) {
        FN += 1;
      }
      else if ((int) labels[i] == negative_class) {
        TN += 1;
      }
    }
  }
}
