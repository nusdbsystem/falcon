//
// Created by root on 3/11/22.
//

#include <falcon/utils/alg/tree_util.h>
#include <falcon/utils/logger/logger.h>

double root_impurity(const std::vector<double>& labels, falcon::TreeType tree_type, int class_num) {
  // check classification or regression
  // if classification, impurity = 1 - \sum_{c=1}^{class_num} (p_c)^2
  // if regression, impurity = (1/n) \sum_{i=1}^n (y_i)^2 - ((1/n) \sum_{i=1}^n y_i)^2
  double impurity = 0.0;
  int n = (int) labels.size();
  log_info("[tree_util.root_impurity] n = " + std::to_string(n));
  // if classification
  if (tree_type == falcon::CLASSIFICATION) {
    // count samples for each class
    std::vector<double> class_bins(class_num, 0.0);
    for (int i = 0; i < n; i++) {
      int c = (int) labels[i];
      class_bins[c] += 1.0;
    }
    // compute probabilities and impurity
    impurity += 1.0;
    for (int c = 0; c < class_num; c++) {
      class_bins[c] = class_bins[c] / ((double) n);
      impurity = impurity - class_bins[c] * class_bins[c];
    }
  }

  // if regression
  if (tree_type == falcon::REGRESSION) {
    // compute labels square
    std::vector<double> labels_square;
    labels_square.reserve(n);
    for (int i = 0; i < n; i++) {
      labels_square.push_back(labels[i] * labels[i]);
    }
    // compute summation
    double label_sum = 0.0, label_square_sum = 0.0;
    for (int i = 0; i < n; i++) {
      label_sum += labels[i];
      label_square_sum += labels_square[i];
    }
    // compute impurity
    impurity = (label_square_sum / n) - (label_sum / n) * (label_sum / n);
  }

  return impurity;
}