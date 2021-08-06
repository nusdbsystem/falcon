//
// Created by wuyuncheng on 3/8/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_LOSS_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_LOSS_H_

#include <falcon/party/party.h>
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/common.h>

// This class is the base class of the supported loss functions
class LossFunction {
 public:
  // whether the loss function is for multi-class classification
  bool is_multi_class;
  // the number of trees for each class
  // if regression, set to 1
  // if binary classification, set to 1
  // if multi-class classification, set to class_num
  int num_trees_per_estimator;

 public:
  /**
   * default constructor
   */
  LossFunction();

  /**
   * constructor
   * @param m_tree_type: regression or classification
   * @param m_class_num: number of classes if classification
   */
  LossFunction(falcon::TreeType m_tree_type, int m_class_num);

  /**
   * destructor
   */
  ~LossFunction() = default;

  /**
   * for each loss function, compute the negative_gradient
   * given the ground_truth_labels and the raw_predictions
   * @param party: the participating party
   * @param ground_truth_labels: the encrypted ground_truth_labels,
   *    throughout this function, we assume that the precision of
   *    ground_truth_labels is PHE_FIXED_POINT_PRECISION
   * @param raw_predictions: the encrypted raw_predictions, throughout
   *    this function, we assume that the precision of raw_predictions
   *    is PHE_FIXED_POINT_PRECISION
   * @param residuals: the returned encrypted residual
   * @param size: the size of the predicting samples
   */
  virtual void negative_gradient(Party party,
                                 EncodedNumber* ground_truth_labels,
                                 EncodedNumber* raw_predictions,
                                 EncodedNumber* residuals,
                                 int size) = 0;

  /**
   * after training each tree of the gbdt model, update the leaves' labels
   * in the tree, refer to sklearn _gb_losses.py for the details
   * @param party: the participating party
   * @param tree_model: the current decision tree model
   * @param data: the data used to update, usually the training data
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  virtual void update_terminal_regions(Party party,
                                       TreeModel tree_model,
                                       std::vector<std::vector<double>> data,
                                       EncodedNumber* residuals,
                                       EncodedNumber* raw_predictions,
                                       int size,
                                       double learning_rate,
                                       int estimator_index) = 0;

  /**
   * compute the init raw predictions during training for each loss function
   * @param party: the participating party
   * @param raw_predictions: the returned encrypted raw_predictions
   * @param size: the size of the predicting samples
   * @param data: the input dataset if has
   * @param labels: the input ground truth labels if has
   */
  virtual void get_init_raw_predictions(Party party,
                                        EncodedNumber* raw_predictions,
                                        int size,
                                        std::vector< std::vector<double> > data,
                                        std::vector<double> labels) = 0;
};

// This class inherits the base LossFunction class, and is for regression tasks
class RegressionLossFunction : public LossFunction {
 public:
  /**
   * default constructor
   */
  RegressionLossFunction() = default;

  /**
   * constructor
   * @param m_tree_type: regression or classification
   * @param m_class_num: number of classes if classification
   */
  RegressionLossFunction(falcon::TreeType m_tree_type, int m_class_num);

  /**
   * destructor
   */
  ~RegressionLossFunction() = default;
};

// This class inherits the base LossFunction class, and is for classification tasks
class ClassificationLossFunction : public LossFunction {
 public:
  /**
   * default constructor
   */
  ClassificationLossFunction() = default;

  /**
   * constructor
   * @param m_tree_type: regression or classification
   * @param m_class_num: number of classes if classification
   */
  ClassificationLossFunction(falcon::TreeType m_tree_type, int m_class_num);

  /**
   * destructor
   */
  ~ClassificationLossFunction() = default;

  /**
   * compute the prediction probabilities based on the raw predictions
   * @param party: the participating party
   * @param raw_predictions: the encrypted raw predictions
   * @param probas: the returned encrypted probas
   * @param size: the size of the predicting samples
   */
  virtual void raw_predictions_to_probas(Party party,
                                        EncodedNumber* raw_predictions,
                                        EncodedNumber* probas,
                                        int size) = 0;

  /**
   * compute the prediction labels based on the raw predictions
   * @param party: the participating party
   * @param raw_predictions: the encrypted raw predictions
   * @param decisions: the returned encrypted decisions
   * @param size: the size of the predicting samples
   */
  virtual void raw_predictions_to_decision(Party party,
                                           EncodedNumber* raw_predictions,
                                           EncodedNumber* decisions,
                                           int size) = 0;
};

// This class defines the LeastSqureError loss function for regression
class LeastSquareError : public RegressionLossFunction {
 public:
  // init dummy prediction
  double dummy_prediction;

 public:
  /**
   * default constructor
   */
  LeastSquareError() = default;

  /**
   * constructor
   */
  LeastSquareError(falcon::TreeType m_tree_type, int m_class_num);

  /**
   * destructor
   */
   ~LeastSquareError() = default;

  /**
   * compute the negative_gradient
   * given the ground_truth_labels and the raw_predictions
   * @param party: the participating party
   * @param ground_truth_labels: the encrypted ground_truth_labels
   * @param raw_predictions: the encrypted raw_predictions
   * @param residuals: the returned encrypted residual
   * @param size: the size of the predicting samples
   */
  void negative_gradient(Party party,
                         EncodedNumber* ground_truth_labels,
                         EncodedNumber* raw_predictions,
                         EncodedNumber* residuals,
                         int size) override;

  /**
   * after training each tree of the gbdt model, update the leaves' labels
   * in the tree, refer to sklearn _gb_losses.py for the details
   * @param party: the participating party
   * @param tree_model: the current decision tree model
   * @param data: the data used to update, usually the training data
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  void update_terminal_regions(Party party,
                               TreeModel tree_model,
                               std::vector<std::vector<double>> data,
                               EncodedNumber* residuals,
                               EncodedNumber* raw_predictions,
                               int size,
                               double learning_rate,
                               int estimator_index) override;

  /**
   * compute the init raw predictions during training for each loss function
   * @param party: the participating party
   * @param raw_predictions: the returned encrypted raw_predictions
   * @param size: the size of the predicting samples
   * @param data: the input dataset if has
   * @param labels: the input ground truth labels if has
   */
  void get_init_raw_predictions(Party party,
                                EncodedNumber* raw_predictions,
                                int size,
                                std::vector< std::vector<double> > data,
                                std::vector<double> labels) override;
};

// This class defines the BinomialDeviance loss function for classification
class BinomialDeviance : public ClassificationLossFunction {
 public:
  /**
   * default constructor
   */
  BinomialDeviance() = default;

  /**
   * constructor
   */
  BinomialDeviance(falcon::TreeType m_tree_type, int m_class_num);

  /**
   * destructor
   */
  ~BinomialDeviance() = default;

  /**
   * compute the negative_gradient
   * given the ground_truth_labels and the raw_predictions
   * @param party: the participating party
   * @param ground_truth_labels: the encrypted ground_truth_labels
   * @param raw_predictions: the encrypted raw_predictions
   * @param residuals: the returned encrypted residual
   * @param size: the size of the predicting samples
   */
  void negative_gradient(Party party,
                         EncodedNumber* ground_truth_labels,
                         EncodedNumber* raw_predictions,
                         EncodedNumber* residuals,
                         int size) override;

  /**
   * after training each tree of the gbdt model, update the leaves' labels
   * in the tree, refer to sklearn _gb_losses.py for the details
   * @param party: the participating party
   * @param tree_model: the current decision tree model
   * @param data: the data used to update, usually the training data
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  void update_terminal_regions(Party party,
                               TreeModel tree_model,
                               std::vector<std::vector<double>> data,
                               EncodedNumber* residuals,
                               EncodedNumber* raw_predictions,
                               int size,
                               double learning_rate,
                               int estimator_index) override;

  /**
 * compute the init raw predictions during training for each loss function
 * @param party: the participating party
 * @param raw_predictions: the returned encrypted raw_predictions
 * @param size: the size of the predicting samples
 * @param data: the input dataset if has
 * @param labels: the input ground truth labels if has
 */
  void get_init_raw_predictions(Party party,
                                EncodedNumber* raw_predictions,
                                int size,
                                std::vector< std::vector<double> > data,
                                std::vector<double> labels) override;

  /**
   * compute the prediction probabilities based on the raw predictions
   * @param party: the participating party
   * @param raw_predictions: the encrypted raw predictions
   * @param probas: the returned encrypted probas
   * @param size: the size of the predicting samples
   */
  void raw_predictions_to_probas(Party party,
                                 EncodedNumber* raw_predictions,
                                 EncodedNumber* probas,
                                 int size) override;

  /**
   * compute the prediction labels based on the raw predictions
   * @param party: the participating party
   * @param raw_predictions: the encrypted raw predictions
   * @param decisions: the returned encrypted decisions
   * @param size: the size of the predicting samples
   */
  void raw_predictions_to_decision(Party party,
                                   EncodedNumber* raw_predictions,
                                   EncodedNumber* decisions,
                                   int size) override;
};

// This class defines the MultinomialDeviance loss function for classification
class MultinomialDeviance : public ClassificationLossFunction {
 public:
 public:
  /**
   * default constructor
   */
  MultinomialDeviance() = default;

  /**
   * constructor
   */
  MultinomialDeviance(falcon::TreeType m_tree_type, int m_class_num);

  /**
   * destructor
   */
  ~MultinomialDeviance() = default;

  /**
   * compute the negative_gradient
   * given the ground_truth_labels and the raw_predictions
   * @param party: the participating party
   * @param ground_truth_labels: the encrypted ground_truth_labels
   * @param raw_predictions: the encrypted raw_predictions
   * @param residuals: the returned encrypted residual
   * @param size: the size of the predicting samples
   */
  void negative_gradient(Party party,
                         EncodedNumber* ground_truth_labels,
                         EncodedNumber* raw_predictions,
                         EncodedNumber* residuals,
                         int size) override;

  /**
   * after training each tree of the gbdt model, update the leaves' labels
   * in the tree, refer to sklearn _gb_losses.py for the details
   * @param party: the participating party
   * @param tree_model: the current decision tree model
   * @param data: the data used to update, usually the training data
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  void update_terminal_regions(Party party,
                               TreeModel tree_model,
                               std::vector<std::vector<double>> data,
                               EncodedNumber* residuals,
                               EncodedNumber* raw_predictions,
                               int size,
                               double learning_rate,
                               int estimator_index) override;

  /**
 * compute the init raw predictions during training for each loss function
 * @param party: the participating party
 * @param raw_predictions: the returned encrypted raw_predictions
 * @param size: the size of the predicting samples
 * @param data: the input dataset if has
 * @param labels: the input ground truth labels if has
 */
  void get_init_raw_predictions(Party party,
                                EncodedNumber* raw_predictions,
                                int size,
                                std::vector< std::vector<double> > data,
                                std::vector<double> labels) override;

  /**
   * compute the prediction probabilities based on the raw predictions
   * @param party: the participating party
   * @param raw_predictions: the encrypted raw predictions
   * @param probas: the returned encrypted probas
   * @param size: the size of the predicting samples
   */
  void raw_predictions_to_probas(Party party,
                                 EncodedNumber* raw_predictions,
                                 EncodedNumber* probas,
                                 int size) override;

  /**
   * compute the prediction labels based on the raw predictions
   * @param party: the participating party
   * @param raw_predictions: the encrypted raw predictions
   * @param decisions: the returned encrypted decisions
   * @param size: the size of the predicting samples
   */
  void raw_predictions_to_decision(Party party,
                                   EncodedNumber* raw_predictions,
                                   EncodedNumber* decisions,
                                   int size) override;
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_LOSS_H_
