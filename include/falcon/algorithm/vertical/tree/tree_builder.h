//
// Created by wuyuncheng on 13/5/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_CART_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_CART_BUILDER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model.h>
#include <falcon/party/party.h>
#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/tree.h>
#include <falcon/algorithm/vertical/tree/node.h>
#include <falcon/algorithm/vertical/tree/feature.h>

#include <vector>
#include <string>
#include <thread>
#include <future>

struct DecisionTreeParams {
  // type of the tree, 'classification' or 'regression'
  std::string tree_type;
  // the function to measure the quality of a split 'gini' or 'entropy'
  std::string criterion;
  // the strategy used to choose a split at each node, 'best' or 'random'
  std::string split_strategy;
  // the number of classes in the dataset, if regression, set to 1
  int class_num;
  // the maximum depth of the tree
  int max_depth;
  // the maximum number of bins to split a feature
  int max_bins;
  // the minimum number of samples required to split an internal node
  int min_samples_split;
  // the minimum number of samples required to be at a leaf node
  int min_samples_leaf;
  // the maximum number of leaf nodes
  int max_leaf_nodes;
  // a node will be split if this split induces a decrease of impurity >= this value
  double min_impurity_decrease;
  // threshold for early stopping in tree growth
  double min_impurity_split;
  // differential privacy (DP) budget, 0 denotes not use DP
  double dp_budget;
};

class DecisionTreeBuilder : public Model {
 public:
  // type of the tree, 'classification' or 'regression'
  falcon::TreeType tree_type;
  // the function to measure the quality of a split 'gini' or 'entropy'
  std::string criterion;
  // the strategy used to choose a split at each node, 'best' or 'random'
  std::string split_strategy;
  // the number of classes in the dataset, if regression, set to 1
  int class_num;
  // the maximum depth of the tree
  int max_depth;
  // the maximum number of bins to split a feature
  int max_bins;
  // the minimum number of samples required to split an internal node
  int min_samples_split;
  // the minimum number of samples required to be at a leaf node
  int min_samples_leaf;
  // the maximum number of leaf nodes
  int max_leaf_nodes;
  // a node will be split if this split induces a decrease of impurity >= this value
  double min_impurity_decrease;
  // threshold for early stopping in tree growth
  double min_impurity_split;
  // differential privacy (DP) budget, 0 denotes not use DP
  double dp_budget;

 public:
  // local feature num in the training dataset
  int local_feature_num;
  // stores the feature types, 0: continuous, 1: categorical
  std::vector<falcon::TreeFeatureType> feature_types;
  // feature helper that assists for training
  FeatureHelper* feature_helpers;
  // binary vectors of classes, if classification
  std::vector< std::vector<int> > indicator_class_vecs;
  // variance vectors of labels, y and y^2, if regression
  std::vector< std::vector<double> > variance_stat_vecs;
  // the decision tree
  Tree tree;

 public:
  /** default constructor */
  DecisionTreeBuilder();

  /**
 * destructor
 */
  ~DecisionTreeBuilder();

  /**
   * DecisionTreeBuilder constructor
   * @param params
   * @param m_class_num
   * @param m_feature_types
   * @param m_indicator_class_vecs
   * @param m_variance_stat_vecs
   * @param m_training_data
   * @param m_testing_data
   * @param m_training_labels
   * @param m_testing_labels
   * @param m_training_accuracy
   * @param m_testing_accuracy
   */
  DecisionTreeBuilder(DecisionTreeParams params,
      std::vector< std::vector<double> > m_training_data,
      std::vector< std::vector<double> > m_testing_data,
      std::vector<double> m_training_labels,
      std::vector<double> m_testing_labels,
      double m_training_accuracy = 0.0,
      double m_testing_accuracy = 0.0);

  /**
   * pre-compute label information to assist tree training
   * (only the active party does the real computation)
   * @param party_type
   */
  void precompute_label_helper(falcon::PartyType party_type);

  /**
   * pre-compute feature helpers to assist tree training
   * (both the active party and passive party do the computation)
   */
  void precompute_feature_helpers();

  /**
   * build the decision tree model
   * @param party
   */
  void train(Party party);

  /**
   * iteratively build tree node
   * @param party
   * @param node_index
   * @param available_feature_ids
   * @param sample_mask_iv
   * @param encrypted_labels
   */
  void build_node(Party &party,
      int node_index,
      std::vector<int> available_feature_ids,
      EncodedNumber *sample_mask_iv,
      EncodedNumber * encrypted_labels);
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_CART_BUILDER_H_
