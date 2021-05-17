//
// Created by wuyuncheng on 12/5/21.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_

#include <vector>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/common.h>

class Node {
 public:
  // node type, default is internal node
  falcon::TreeNodeType node_type;
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
  // the split threshold if it is its own feature
  double split_threshold;
  // the number of samples where the element in sample_iv is [1]
  int node_sample_num;
  // the number of samples for each class on the node
  std::vector<int> node_sample_distribution;
  // node impurity, Gini index for classification, variance for regression
  EncodedNumber impurity;
  // if is_leaf is true, a label is assigned
  EncodedNumber label;
  // left branch id of the current node, if not a leaf node, -1: not decided
  int left_child;
  // right branch id of the current node, if not a leaf node, -1: not decided
  int right_child;

 public:
  Node();
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
  Node &operator=(const Node &node);

  /**
   * print node information
   */
  void print_node();
};


struct PredictHelper {
  int is_leaf;
  int is_self_feature;
  int best_client_id;
  int best_feature_id;
  int best_split_id;
  int mark;
  int index;

  PredictHelper() {
    is_leaf = -1;
    is_self_feature = -1;
    best_client_id = -1;
    best_feature_id = -1;
    best_split_id = -1;
    mark = -1;
    index = -1;
  }

  PredictHelper(int m_is_leaf, int m_is_self_feature, int m_best_client_id,
                int m_best_feature_id, int m_best_split_id, int m_mark, int m_index) {
    is_leaf = m_is_leaf;
    is_self_feature = m_is_self_feature;
    best_client_id = m_best_client_id;
    best_feature_id = m_best_feature_id;
    best_split_id = m_best_split_id;
    mark = m_mark;
    index = m_index;
  }

  ~PredictHelper() {}
};

#endif //FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_
