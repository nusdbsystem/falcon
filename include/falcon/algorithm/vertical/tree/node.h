//
// Created by wuyuncheng on 12/5/21.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_

#include <vector>
#include <falcon/operator/phe/fixed_point_encoder.h>

class Node {
 public:
  // if the node is a leaf node, 0: not leaf, 1: leaf node, -1: not decided
  int is_leaf;
  // the depth of the current node, root node is 0, -1: not decided
  int depth;
  // if the node belongs to the party itself, 0: no, 1: yes, -1: not decided
  int is_self_feature;
  // the party that owns the selected feature on this node, -1: not decided
  int best_party_id;
  // the feature on this node, -1: not self feature, 0 -- d_i: self feature id
  int best_feature_id;
  // the split of the feature on this node, -1: not decided
  int best_split_id;
  // the available local feature ids on the current node
  std::vector<int> available_feature_ids;
  // the number of global features globally
  int available_global_feature_num;
  // encrypted indicator vector of which samples are available on this node
  EncodedNumber *sample_iv;
  // the number of samples of sample_iv on the node
  int sample_iv_size;
  // the number of classes on the node (classification: classes_num, regression: 2)
  int class_num;
  // encrypted labels, classification: classes_num*sample_num, regression: 2*sample_num
  EncodedNumber *encrypted_labels;
  // left branch id of the current node, if not a leaf node, -1: not decided
  int left_child;
  // right branch id of the current node, if not a leaf node, -1: not decided
  int right_child;

 private:
  // the number of samples where the element in sample_iv is [1]
  int node_sample_num;
  // the number of samples for each class on the node
  std::vector<int> node_sample_distribution;
  // node impurity, Gini index for classification, variance for regression
  EncodedNumber impurity;
  // if is_leaf is true, a label is assigned
  EncodedNumber label;

 public:
  Node();
  Node(int m_depth,
       int m_sample_iv_size,
       int m_class_num,
       EncodedNumber *m_sample_iv,
       EncodedNumber *m_encrypted_labels);
  ~Node();

  /**
   * copy constructor
   *
   * @param node
   */
  Node(const Node &node);

  /**
   * assignment constructor
   *
   * @param node
   * @return
   */
  Node &operator=(Node *node);

  /**
   * set for node sample num
   * @param s_node_sample_num
   */
  void set_node_sample_num(int s_node_sample_num);

  /**
   * set for node sample distribution
   * @param s_node_sample_distribution
   */
  void set_node_sample_distribution(std::vector<int> s_node_sample_distribution);

  /**
   * set for node impurity
   * @param s_impurity
   */
  void set_impurity(EncodedNumber s_impurity);

  /**
   * set for node label
   * @param s_label
   */
  void set_label(EncodedNumber s_label);

  /**
   * get node sample num
   * @return
   */
  int get_node_sample_num() const;

  /**
   * get node sample distribution
   * @return
   */
  std::vector<int> get_node_sample_distribution() const;

  /**
   * get node impurity
   * @param g_impurity
   */
  void get_impurity(EncodedNumber & g_impurity) const;

  /**
   * get node label
   * @param g_label
   */
  void get_label(EncodedNumber & g_label) const;

  /**
   * print node information
   */
  void print_node();
};

#endif //FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_
