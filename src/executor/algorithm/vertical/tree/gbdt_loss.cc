//
// Created by wuyuncheng on 3/8/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_loss.h>

#include <glog/logging.h>

/////////////////////////////////////////
///    LossFunction methods /////////////
/////////////////////////////////////////
LossFunction::LossFunction() {
  is_multi_class = false;
  num_trees_per_estimator = 1;
}

LossFunction::LossFunction(falcon::TreeType m_tree_type, int m_class_num) {
  if ((m_tree_type == falcon::REGRESSION) ||
      (m_tree_type == falcon::CLASSIFICATION && m_class_num == 2)) {
    is_multi_class = false;
    num_trees_per_estimator = 1;
  } else {
    is_multi_class = true;
    num_trees_per_estimator = m_class_num;
  }
}


/////////////////////////////////////////
/// RegressionLossFunction methods //////
/////////////////////////////////////////
RegressionLossFunction::RegressionLossFunction(falcon::TreeType m_tree_type,
                                               int m_class_num) :
                                               LossFunction(m_tree_type, m_class_num) {
  if (m_tree_type != falcon::REGRESSION) {
    LOG(ERROR) << "gbdt task type must be regression to use this loss function";
    exit(1);
  }
}


/////////////////////////////////////////
/// ClassificationLossFunction methods///
/////////////////////////////////////////
ClassificationLossFunction::ClassificationLossFunction(falcon::TreeType m_tree_type,
                                                       int m_class_num) :
                                                       LossFunction(m_tree_type, m_class_num) {
  if (m_tree_type != falcon::CLASSIFICATION) {
    LOG(ERROR) << "gbdt task type must be classification to use this loss function";
    exit(1);
  }
}


/////////////////////////////////////////
/// LeastSquareError methods ////////////
/////////////////////////////////////////
void LeastSquareError::negative_gradient(Party party,
                                         EncodedNumber *ground_truth_labels,
                                         EncodedNumber **raw_predictions,
                                         EncodedNumber *residual,
                                         int size) {

}

void LeastSquareError::update_terminal_regions(Party party,
                                               DecisionTreeBuilder decision_tree_builder,
                                               EncodedNumber *residual,
                                               EncodedNumber **raw_predictions,
                                               int size,
                                               double learning_rate,
                                               int estimator_index) {

}


/////////////////////////////////////////
/// BinomialDeviance methods ////////////
/////////////////////////////////////////
void BinomialDeviance::negative_gradient(Party party,
                                         EncodedNumber *ground_truth_labels,
                                         EncodedNumber **raw_predictions,
                                         EncodedNumber *residual,
                                         int size) {

}

void BinomialDeviance::update_terminal_regions(Party party,
                                               DecisionTreeBuilder decision_tree_builder,
                                               EncodedNumber *residual,
                                               EncodedNumber **raw_predictions,
                                               int size,
                                               double learning_rate,
                                               int estimator_index) {

}

void BinomialDeviance::get_init_raw_predictions(Party party,
                                                EncodedNumber **raw_predictions,
                                                int size,
                                                std::vector<std::vector<double>> data,
                                                std::vector<double> labels) {

}

void BinomialDeviance::raw_predictions_to_probas(Party party,
                                                 EncodedNumber **raw_predictions,
                                                 EncodedNumber *probas,
                                                 int size) {

}

void BinomialDeviance::raw_predictions_to_decision(Party party,
                                                   EncodedNumber **raw_predictions,
                                                   EncodedNumber *decisions,
                                                   int size) {

}


/////////////////////////////////////////
/// MultinomialDeviance methods /////////
/////////////////////////////////////////
void MultinomialDeviance::negative_gradient(Party party,
                                            EncodedNumber *ground_truth_labels,
                                            EncodedNumber **raw_predictions,
                                            EncodedNumber *residual,
                                            int size) {

}

void MultinomialDeviance::update_terminal_regions(Party party,
                                                  DecisionTreeBuilder decision_tree_builder,
                                                  EncodedNumber *residual,
                                                  EncodedNumber **raw_predictions,
                                                  int size,
                                                  double learning_rate,
                                                  int estimator_index) {

}

void MultinomialDeviance::get_init_raw_predictions(Party party,
                                                   EncodedNumber **raw_predictions,
                                                   int size,
                                                   std::vector<std::vector<double>> data,
                                                   std::vector<double> labels) {

}

void MultinomialDeviance::raw_predictions_to_probas(Party party,
                                                    EncodedNumber **raw_predictions,
                                                    EncodedNumber *probas,
                                                    int size) {

}

void MultinomialDeviance::raw_predictions_to_decision(Party party,
                                                      EncodedNumber **raw_predictions,
                                                      EncodedNumber *decisions,
                                                      int size) {

}

