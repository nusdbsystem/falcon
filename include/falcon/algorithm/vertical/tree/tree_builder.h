//
// Created by wuyuncheng on 13/5/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_CART_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_CART_BUILDER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model_builder.h>
#include <falcon/party/party.h>
#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/algorithm/vertical/tree/node.h>
#include <falcon/algorithm/vertical/tree/feature.h>

#include <vector>
#include <string>
#include <thread>
#include <future>

struct DecisionTreeParams {
  // type of the tree, 'classification' or 'regression'
  std::string tree_type;
  // the function to measure the quality of a split
  // for classification, 'gini' or 'entropy'
  // for regression, criterion is 'mse'
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

class DecisionTreeBuilder : public ModelBuilder {

 public:
  int train_data_size;
  int test_data_size;
 public:
  // type of the tree, 'classification' or 'regression'
  falcon::TreeType tree_type;
  // the function to measure the quality of a split
  // for classification, 'gini' or 'entropy'
  // for regression, criterion is 'mse'
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
  TreeModel tree;

 public:
  /** default constructor */
  DecisionTreeBuilder();

  /** copy constructor */
  DecisionTreeBuilder(const DecisionTreeBuilder& builder);

  /**
 * destructor
 */
  ~DecisionTreeBuilder();

  /**
   * DecisionTreeBuilder constructor
   * @param params: the decision tree builder parameters
   * @param m_training_data: the training data used
   * @param m_testing_data: the testing data used
   * @param m_training_labels: if active party, the training labels used
   * @param m_testing_labels: if active party, the testing labels used
   * @param m_training_accuracy: the training accuracy
   * @param m_testing_accuracy: the testing accuracy
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
   * @param party_type: whether the party is the active party
   */
  void precompute_label_helper(falcon::PartyType party_type);

  /**
   * pre-compute feature helpers to assist tree training
   * (both the active party and passive party do the computation)
   */
  void precompute_feature_helpers();

  /**
   * build the decision tree model
   * @param party: initialized party object
   */
  void train(Party party) override;

  /**
   * build the decision tree model given encrypted labels (mainly called
   * in secure gbdt training) such that other steps can be reused
   * @param party: the participating party
   * @param encrypted_labels: provide encrypted labels
   */
  void train(Party party, EncodedNumber* encrypted_labels);

  /**
   * init the encrypted mask iv for samples
   *
   * @param party: initialized party object
   * @param sample_mask_iv: returned sample mask iv
   * @param sample_num: the number of samples
   */
  void initialize_sample_mask_iv(Party party, EncodedNumber* sample_mask_iv, int sample_num);

  /**
   * the active party init the encrypted labels
   *
   * @param party: initialized party object
   * @param encrypted_labels: the returned encrypted labels
   * @param sample_num: the number of samples
   */
  void initialize_encrypted_labels(Party party, EncodedNumber* encrypted_labels, int sample_num);

  /**
   * iteratively build tree node
   * @param party: initialized party object
   * @param node_index: the index of the tree node to be built
   * @param available_feature_ids: the available features left
   * @param sample_mask_iv: the encrypted mask indicator vector
   * @param encrypted_labels: the encrypted labels of the training data
   * @param use_sample_weights: whether use sample weights (for LIME)
   * @param sss_sample_weights: the encrypted weights (for LIME)
   */
  void build_node(Party &party,
      int node_index,
      std::vector<int> available_feature_ids,
      EncodedNumber *sample_mask_iv,
      EncodedNumber * encrypted_labels,
      bool use_sample_weights = false,
      const std::vector<double> &sss_sample_weights = std::vector<double>());

  /**
   * check if this node satisfies the pruning conditions
   * some of the conditions are forwarded to spdz for checking
   * @param party: initialized party object
   * @param node_index: the index of tree node to be check
   * @param sample_mask_iv: the encrypted mask indicator vector
   * @return whether pruning condition is satisfied
   */
  bool check_pruning_conditions(Party &party,
      int node_index,
      EncodedNumber *sample_mask_iv);

  /**
   * calculate the impurity of the root node
   * @param party: initialized party object
   */
  void calc_root_impurity(const Party& party);

  /**
   * check if this node satisfies the pruning conditions
   * some of the conditions are forwarded to spdz for checking
   * @param party: initialized party object
   * @param node_index: the index of tree node to be computed
   * @param sample_mask_iv: the encrypted mask indicator vector
   * @param encrypted_labels: the encrypted labels of the training data
   * @param use_sample_weights: whether use sample weights (for LIME)
   * @param sss_sample_weights: the encrypted weights (for LIME)
   */
  void compute_leaf_statistics(Party &party,
      int node_index,
      EncodedNumber *sample_mask_iv,
      EncodedNumber *encrypted_labels,
      bool use_sample_weights = false,
      const std::vector<double> &sss_sample_weights = std::vector<double>());

  /**
   * the core logic for finding the best split securely
   *
   * @param party: initialized party object
   * @param node_index: the index of tree node to be computed
   * @param available_feature_ids: the available features of the current node
   * @param sample_mask_iv: the encrypted mask iv of the samples
   * @param encrypted_labels: the encrypted labels of the training data
   * @param party_split_nums: the returned vector of each party's split numbers
   * @param use_sample_weights: whether use sample weights (for LIME)
   * @param sss_sample_weights: the encrypted weights (for LIME)
   */
  std::vector<double> find_best_split(const Party& party, int node_index,
                                      std::vector<int> available_feature_ids,
                                      EncodedNumber* sample_mask_iv,
                                      EncodedNumber* encrypted_labels,
                                      std::vector<int>& party_split_nums,
                                      bool use_sample_weights = false,
                                      const std::vector<double> &sss_sample_weights = std::vector<double>());

  /**
   * encrypt the left and right impurity
   *
   * @param party: initialized party object
   * @param left_impurity: plaintext left impurity
   * @param right_impurity: plaintext right impurity
   * @param encrypted_left_impurity: returned encrypted left impurity
   * @param encrypted_right_impurity: returned encrypted right impurity
   */
  void encrypt_left_right_impu(const Party& party, double left_impurity, double right_impurity,
                               EncodedNumber& encrypted_left_impurity,
                               EncodedNumber& encrypted_right_impurity);

  /**
   * compute encrypted impurity gain for each feature and each split
   * @param party: initialized party object
   * @param node_index: the index of tree node to be computed
   * @param available_feature_ids: the available features of the current node
   * @param sample_mask_iv: the encrypted mask iv of the samples
   * @param encrypted_statistics: the encrypted statistics to be returned
   * @param encrypted_labels: the encrypted labels of the training data
   * @param encrypted_left_sample_nums: the encrypted left sample numbers
   * @param encrypted_right_sample_nums: the encrypted right sample numbers
   * @param use_sample_weights: whether use sample weights (for LIME)
   * @param sss_sample_weights: the encrypted weights (for LIME)
   */
  void compute_encrypted_statistics(const Party &party, int node_index,
                                    std::vector<int> available_feature_ids,
                                    EncodedNumber *sample_mask_iv,
                                    EncodedNumber ** encrypted_statistics,
                                    EncodedNumber * encrypted_labels,
                                    EncodedNumber * encrypted_left_sample_nums,
                                    EncodedNumber * encrypted_right_sample_nums,
                                    bool use_sample_weights = false,
                                    const std::vector<double> &sss_sample_weights = std::vector<double>());

  /**
   * find the party id that has the best split
   *
   * @param best_split_index: the global best split index
   * @param party_split_nums: the split numbers of parties
   * @return
   */
  void best_split_party_idx(int best_split_index, std::vector<int> party_split_nums,
                            int& i_star, int& index_star);

  /**
   * find the feature id and split id of the best split
   *
   * @param index_star: the local split index star
   * @param available_feature_ids: the available features of the current node
   * @param j_star: the best feature id
   * @param s_star: the best split id
   */
  void best_feature_split_idx(int index_star, std::vector<int> available_feature_ids,
                              int& j_star, int& s_star);

  /**
   * update the node info when building a tree node
   *
   * @param node_index: the current node index
   * @param node_type: the type of the current node
   * @param is_self_feature: whether the split is from self feature
   * @param best_party_id: the party id that has the best split
   * @param best_feature_id: the feature id of the best split
   * @param best_split_id: the best split index
   * @param split_threshold: the split threshold, only set if is_self_feature
   * @param left_child_index: the left child of the current node
   * @param right_child_index: the right child of the current node
   * @param encrypted_left_impurity: the encrypted impurity of the left child
   * @param encrypted_right_impurity: the encrypted impurity of the right child
   */
  void update_node_info(int node_index, falcon::TreeNodeType node_type,
                        int is_self_feature, int best_party_id, int best_feature_id,
                        int best_split_id, double split_threshold,
                        int left_child_index, int right_child_index,
                        EncodedNumber encrypted_left_impurity,
                        EncodedNumber encrypted_right_impurity);

  /**
   * update the encrypted mask iv and encrypted labels for the left and right child
   * (only executed by the party who has the best split)
   *
   * @param party: initialized party object
   * @param j_star: the feature id of the best split
   * @param s_star: the best split index
   * @param sample_num: the number of samples
   * @param sample_mask_iv: the sample mask iv of the current node
   * @param encrypted_labels: the encrypted labels of the current node
   * @param encrypted_left_impurity: the impurity of the left child
   * @param encrypted_right_impurity: the impurity of the right child
   * @param sample_mask_iv_left: the returned mask iv of the left child
   * @param sample_mask_iv_right: the returned mask iv of the right child
   * @param encrypted_labels_left: the returned encrypted labels of the left child
   * @param encrypted_labels_right: the returned encrypted labels of the right child
   * @param update_str_sample_iv: the updated iv string returned for sending to other parties
   * @param update_str_encrypted_labels_left: the updated label string left returned for sending to other parties
   * @param update_str_encrypted_labels_right: the updated label string left returned for sending to other parties
   */
  void update_mask_iv_and_labels(const Party& party,
                                 int j_star, int s_star, int sample_num,
                                 EncodedNumber *sample_mask_iv,
                                 EncodedNumber *encrypted_labels,
                                 EncodedNumber encrypted_left_impurity,
                                 EncodedNumber encrypted_right_impurity,
                                 EncodedNumber *sample_mask_iv_left,
                                 EncodedNumber *sample_mask_iv_right,
                                 EncodedNumber *encrypted_labels_left,
                                 EncodedNumber *encrypted_labels_right,
                                 std::string& update_str_sample_iv,
                                 std::string& update_str_encrypted_labels_left,
                                 std::string& update_str_encrypted_labels_right);

 /**
 * specific train function for lime
 *
 * @param party: initialized party object
 * @param use_encrypted_labels: whether use encrypted labels during training
 * @param encrypted_true_labels: encrypted labels used
 * @param use_sample_weights: whether use encrypted sample weights
 * @param sss_sample_weights: encrypted sample weights
 */
  void lime_train(Party party,
                  bool use_encrypted_labels,
                  EncodedNumber* encrypted_true_labels,
                  bool use_sample_weights,
                  const std::vector<double> &sss_sample_weights);

  /**
   * build the decision tree model
   * @param party: initialized party object
  */
  void distributed_train(const Party& party, const Worker& worker) override;

  /**
   * specific train function for lime
   *
   * @param party: initialized party object
   * @param worker: worker instance for distributed training
   * @param use_encrypted_labels: whether use encrypted labels during training
   * @param encrypted_true_labels: encrypted labels used
   * @param use_sample_weights: whether use encrypted sample weights
   * @param sss_sample_weights: encrypted sample weights
   */
  void distributed_lime_train(Party party,
                              const Worker& worker,
                              bool use_encrypted_labels,
                              EncodedNumber* encrypted_true_labels,
                              bool use_sample_weights,
                              const std::vector<double> &sss_sample_weights);

  /**
   * build the tree nodes for distributed train
   *
   * @param party: initialized party object
   * @param worker: worker instance for distributed training
   * @param sample_num: the number of samples
   * @param init_available_feature_ids: initial available feature ids
   * @param init_sample_mask_iv: init sample mask iv
   * @param init_encrypted_labels: init encrypted labels
   * @param use_sample_weights: whether use sample weights
   * @param sss_sample_weights: the encrypted sample weights if use_sample_weights is true
   */
  void distributed_build_nodes(const Party& party, const Worker &worker,
                               int sample_num,
                               std::vector<int> init_available_feature_ids,
                               EncodedNumber* init_sample_mask_iv,
                               EncodedNumber* init_encrypted_labels,
                               bool use_sample_weights,
                               const std::vector<double> &sss_sample_weights);

  /**
   * evaluate the accuracy on the dataset
   * @param party: initialized party object
   * @param eval_type: the evaluation type
   * @param report_save_path: the report to be saved
   */
  void eval(Party party, falcon::DatasetType eval_type,
      const std::string& report_save_path = std::string()) override;

  /**
   * distributed eval
   * @param party: initialized party object
   * @param worker: the worker object
   * @param eval_type train or test
   */
  void distributed_eval(Party &party, const Worker &worker,
                        falcon::DatasetType eval_type);

  /**
   * calculate the accuracy on a dataset (or sub-dataset when distributed eval)
   *
   * @param party: initialized party object
   * @param eval_type: on training dataset or testing dataset
   * @param eval_dataset: the actual dataset to be eval
   * @param eval_dataset_labels: the actual labels of the eval_dataset
   */
  void calc_eval_accuracy(Party& party, falcon::DatasetType eval_type,
                          const std::vector<std::vector<double>>& eval_dataset,
                          const std::vector<double>& eval_dataset_labels);

  /**
   * active party aggregate the overall tree model
   * and decrypt the model for print illustration
   *
   * @param party: the participating party
   * @return: return the resulted tree model
   */
  TreeModel aggregate_decrypt_tree_model(Party& party);
};

/**
    * compute spdz function for tree builder
    * @param party_num: the number of parties
    * @param party_id: the party's id
    * @param mpc_tree_port_base: the port base to connect to mpc program
    * @param party_host_names: the ip addresses of parties
    * @param public_value_size: the number of public values
    * @param public_values: the public values to be sent
    * @param private_value_size: the number of private values
    * @param private_values: the private values to be sent
    * @param tree_comp_type: the computation type
    * @param res: the result returned from mpc program
    */
void spdz_tree_computation(int party_num,
    int party_id,
    std::vector<int> mpc_tree_port_bases,
    std::vector<std::string> party_host_names,
    int public_value_size,
    const std::vector<int>& public_values,
    int private_value_size,
    const std::vector<double>& private_values,
    falcon::SpdzTreeCompType tree_comp_type,
    std::promise<std::vector<double>> *res);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_CART_BUILDER_H_
