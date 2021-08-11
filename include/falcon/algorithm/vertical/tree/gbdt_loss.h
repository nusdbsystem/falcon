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
   * @param tree_builder: the current tree builder
   * @param ground_truth_labels: the encrypted ground truth labels
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  virtual void update_terminal_regions(Party party,
                                       DecisionTreeBuilder &decision_tree_builder,
                                       EncodedNumber *ground_truth_labels,
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
                                        int size,
                                        int phe_precision) = 0;

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
                                           int size,
                                           int phe_precision) = 0;
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
   * @param tree_builder: the current tree builder
   * @param ground_truth_labels: the encrypted ground truth labels
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  void update_terminal_regions(Party party,
                               DecisionTreeBuilder &decision_tree_builder,
                               EncodedNumber *ground_truth_labels,
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
  // init dummy prediction
  double dummy_prediction = 0.0;

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
   * @param tree_builder: the current tree builder
   * @param ground_truth_labels: the encrypted ground truth labels
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  void update_terminal_regions(Party party,
                               DecisionTreeBuilder &decision_tree_builder,
                               EncodedNumber *ground_truth_labels,
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
                                 int size,
                                 int phe_precision) override;

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
                                   int size,
                                   int phe_precision) override;
};

// This class defines the MultinomialDeviance loss function for classification
class MultinomialDeviance : public ClassificationLossFunction {
 public:
  // init dummy predictions
  std::vector<double> dummy_predictions;

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
   * @param tree_builder: the current tree builder
   * @param ground_truth_labels: the encrypted ground truth labels
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param estimator_index: the index of the estimator
   */
  void update_terminal_regions(Party party,
                               DecisionTreeBuilder &decision_tree_builder,
                               EncodedNumber *ground_truth_labels,
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
                                 int size,
                                 int phe_precision) override;

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
                                   int size,
                                   int phe_precision) override;
};

/**
 * after building a tree model, update the raw predictions
 * @param party: the participating party
 * @param tree_builder: the current tree builder
 * @param raw_predictions: the encrypted raw predictions, to be returned
 * @param size: the size of the predicting samples
 * @param learning_rate: the learning rate for raw predictions update
 */
void update_raw_predictions_with_learning_rate(Party party,
                                               DecisionTreeBuilder &decision_tree_builder,
                                               EncodedNumber* raw_predictions,
                                               int size,
                                               double learning_rate);

/**
 * compute the expit of the raw_predictions
 * @param party: the participating party
 * @param raw_predictions: the encrypted raw predictions
 * @param expit_raw_predictions: the encrypted expit raw predictions, to be returned
 * @param size: the size of the predicting samples
 * @param class_num: the number of classes
 * @param phe_precision: the precision of the ciphertext
 */
void compute_raw_predictions_expit(Party party,
                                   EncodedNumber* raw_predictions,
                                   EncodedNumber* expit_raw_predictions,
                                   int size,
                                   int class_num,
                                   int phe_precision);

/**
 * compute the softmax of the raw predictions
 * @param party: the participating party
 * @param raw_predictions: the encrypted raw predictions
 * @param softmax_raw_predictions: the encrypted softmax raw predictions, to be returned
 * @param sample_size: the size of the predicting samples
 * @param class_num: the number of classes
 * @param phe_precision: the precision of the ciphertext
 */
void compute_raw_predictions_softmax(Party party,
                                     EncodedNumber* raw_predictions,
                                     EncodedNumber* softmax_raw_predictions,
                                     int sample_size,
                                     int class_num,
                                     int phe_precision);

/**
   * after training each tree of the gbdt model, update the leaves' labels
   * in the tree, refer to sklearn _gb_losses.py for the details
   * @param party: the participating party
   * @param tree_builder: the current tree builder
   * @param ground_truth_labels: the encrypted ground truth labels
   * @param residuals: the encrypted residual for this tree
   * @param raw_predictions: the encrypted raw_predictions for this tree
   * @param size: the size of the predicting samples
   * @param learning_rate: the shrinkage rate in the gbdt params, when multiply
   *    learning_rate, need to truncate the result precision before adding to
   *    the original raw_predictions
   * @param class_num: the class num for the update
 */
void update_terminal_regions_for_classification(Party party,
                                                DecisionTreeBuilder &decision_tree_builder,
                                                EncodedNumber *ground_truth_labels,
                                                EncodedNumber* residuals,
                                                EncodedNumber* raw_predictions,
                                                int sample_size,
                                                double learning_rate,
                                                int class_num);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_LOSS_H_
