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
void ClassificationMetrics::compute_metrics(std::vector<int> predictions, std::vector<double> labels)
   {
  // std::cout << "ClassificationMetrics constructor called" << std::endl;
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

  // Sensitivity = TP/(TP+FN)
  sensitivity = TP / (TP + FN);
  // NOTE: sensitivity = recall = true positive rate (TPR)
  double TPR = sensitivity;
  // NOTE: recall = sensitivity
  // Recall = TP / all relevant elements
  // Recall = TP/(TP + FN) = Sensitivity
  double recall = sensitivity;

  // Specificity = TN/(TN+FP)
  specificity = TN / (TN + FP);
  // NOTE: specificity = true negative rate (TNR)
  double TNR = specificity;

  // In the binary case, balanced accuracy =
  // average of sensitivity (true positive rate) and specificity (true negative rate)
  balanced_accuracy = (TPR + TNR) / 2;

  // Precision or Positive Predictive Value (PPV)
  // Positive Predictive Value (Precision) = TP / (TP + FP)
  precision = TP / (TP + FP);
  // Negative Predictive Value = TN / (TN + FN)
  NPV = TN / (TN + FN);  

  // F1 score
  // The F1 score can be interpreted as a weighted average of the precision and recall,
  // where an F1 score reaches its best value at 1 and worst score at 0.
  // The relative contribution of precision and recall to the F1 score are equal.
  // The formula for the F1 score is:
  // F1 = 2TP / (2TP + FP + FN)
  // F1 = 2 * (precision * recall) / (precision + recall)
  F1 = 2*TP / (2*TP + FP + FN);
}


// pretty print confusion matrix
void ClassificationMetrics::pretty_print_cm() {
  /* example output:
Confusion Matrix
               Pred
               Class1    Class0
True Class1    5        3
     Class0    2        3
  */
  std::cout << "Confusion Matrix\n";
  std::cout << "               Pred\n";
  std::cout << "               Class1    Class0\n";
  std::cout << "True Class1" << "    " << TP << "        " << FN << std::endl;
  std::cout << "     Class0" << "    " << FP << "        " << TN << std::endl;

  // save a version for gLog
  LOG(INFO) << "Confusion Matrix";
  LOG(INFO) << "               Pred";
  LOG(INFO) << "               Class1    Class0";
  LOG(INFO) << "True Class1" << "    " << TP << "        " << FN;
  LOG(INFO) << "     Class0" << "    " << FP << "        " << TN;
}

// pretty print classification performance report
void ClassificationMetrics::classification_report() {
  std::cout << "=== Classification Report ===" << std::endl;

  std::cout << "False negative (FN) = " << FN << std::endl;
  std::cout << "False positive (FP) = " << FP << std::endl;
  std::cout << "True positive (TP) = " << TP << std::endl;
  std::cout << "True negative (TN) = " << TN << std::endl;

  std::cout << "regular_accuracy = " << regular_accuracy << std::endl;
  std::cout << "balanced_accuracy = " << balanced_accuracy << std::endl;

  std::cout << "sensitivity = " << sensitivity << std::endl;
  std::cout << "specificity = " << specificity << std::endl;

  std::cout << "precision (PPV) = " << precision << std::endl;
  std::cout << "NPV = " << NPV << std::endl;
  std::cout << "F1 score = " << F1 << std::endl;

  // save a version for gLog
  LOG(INFO) << "=== Classification Report ===";

  LOG(INFO) << "False negative (FN) = " << FN;
  LOG(INFO) << "False positive (FP) = " << FP;
  LOG(INFO) << "True positive (TP) = " << TP;
  LOG(INFO) << "True negative (TN) = " << TN;

  LOG(INFO) << "regular_accuracy = " << regular_accuracy;
  LOG(INFO) << "balanced_accuracy = " << balanced_accuracy;

  LOG(INFO) << "sensitivity = " << sensitivity;
  LOG(INFO) << "specificity = " << specificity;

  LOG(INFO) << "precision (PPV) = " << precision;
  LOG(INFO) << "NPV = " << NPV;
  LOG(INFO) << "F1 score = " << F1;
}
