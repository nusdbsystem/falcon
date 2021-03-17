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


class ClassificationMetrics{
  // inspired by scikit-learn's:
  // https://scikit-learn.org/stable/modules/model_evaluation.html
  public:
    // four cells of the confusion matrix
    double FN;  // False negative (FN)
    double FP;  // False positive (FP)
    double TP;  // True positive (TP)
    double TN;  // True negative (TN)

    // regular accuracy = fraction of correct predictions over all samples
    double regular_accuracy;

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
};
