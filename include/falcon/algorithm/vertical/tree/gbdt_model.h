//
// Created by wuyuncheng on 31/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_MODEL_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_MODEL_H_

#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/common.h>
#include <falcon/party/party.h>

#include <future>
#include <thread>

class GbdtModel {
public:
  // number of trees in the model
  int tree_size{};
  // type of the tree, 'classification' or 'regression'
  falcon::TreeType tree_type;
  // number of estimator for the gbdt model
  int n_estimator{};
  // number of classes in the model, 1 for regression
  int class_num{};
  // shrinkage (learning rate)
  double learning_rate{};
  // dummy predictors, for the initial prediction
  // if tree_size == n_estimator, only one predictor, otherwise,
  // multi-class classification, with class_num predictors
  std::vector<double> dummy_predictors;
  // vector of tree models
  std::vector<TreeModel> gbdt_trees;

public:
  /**
   * default constructor
   */
  GbdtModel() = default;

  /**
   * constructor
   * @param m_tree_size
   * @param m_tree_type
   * @param m_n_estimator
   * @param m_class_num
   * @param m_learning_rate
   */
  GbdtModel(int m_tree_size, const std::string &m_tree_type, int m_n_estimator,
            int m_class_num, double m_learning_rate);

  /**
   * default destructor
   */
  ~GbdtModel() = default;

  /**
   * copy constructor
   * @param gbdt_model
   */
  GbdtModel(const GbdtModel &gbdt_model);

  /**
   * assignment constructor
   * @param gbdt_model
   * @return
   */
  GbdtModel &operator=(const GbdtModel &gbdt_model);

  /**
   * given the gbdt model, predict on samples
   * @param party
   * @param predicted_samples
   * @param predicted_sample_size
   * @param predicted_labels
   * @return predicted labels (encrypted)
   */
  void predict(Party &party,
               const std::vector<std::vector<double>> &predicted_samples,
               int predicted_sample_size, EncodedNumber *predicted_labels);

  /**
   * given the gbdt model, predict on samples
   * if regression or binary classification
   * @param party
   * @param predicted_samples
   * @param predicted_sample_size
   * @param predicted_labels
   * @return predicted labels (encrypted)
   */
  void predict_single_estimator(
      Party &party, const std::vector<std::vector<double>> &predicted_samples,
      int predicted_sample_size, EncodedNumber *predicted_labels);

  /**
   * given the gbdt model, predict on samples
   * @param party
   * @param predicted_samples
   * @param predicted_sample_size
   * @param predicted_labels
   * @return predicted labels (encrypted)
   */
  void predict_multi_estimator(
      Party &party, const std::vector<std::vector<double>> &predicted_samples,
      int predicted_sample_size, EncodedNumber *predicted_labels);
};

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_MODEL_H_
