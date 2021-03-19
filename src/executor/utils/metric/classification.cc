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

  // compute the four cells of confusion matrix
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

  // compute regular accuracy
  // regular accuracy = fraction of correct predictions over all samples
  regular_accuracy = (TP + TN) / (TP + FP + FN + TN);

  // In the binary case, balanced accuracy =
  // average of sensitivity (true positive rate) and specificity (true negative rate)
  // balanced_accuracy = (TPR + TNR) / 2
  double balanced_accuracy;

  // Sensitivity = TP/(TP+FN)
  // NOTE: recall = sensitivity
  // Recall = TP / all relevant elements
  // Recall = TP/(TP + FN) = Sensitivity
  double sensitivity;
  // NOTE: sensitivity = recall = true positive rate (TPR)

  // Specificity = TN/(TN+FP)
  // NOTE: specificity = true negative rate (TNR)
  double specificity;

  // Precision or Positive Predictive Value (PPV)
  // Positive Predictive Value (Precision) = TP / (TP + FP)
  double precision;
  double NPV;  // Negative Predictive Value = TN / (TN + FN)

  // F1 score
  // The F1 score can be interpreted as a weighted average of the precision and recall, where an F1 score reaches its best value at 1 and worst score at 0. The relative contribution of precision and recall to the F1 score are equal. The formula for the F1 score is:
  // F1 = 2TP / (2TP + FP + FN)
  // F1 = 2 * (precision * recall) / (precision + recall)
  double F1;
}
