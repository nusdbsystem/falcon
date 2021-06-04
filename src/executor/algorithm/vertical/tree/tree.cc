//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/tree.h>
#include <cmath>

#include <glog/logging.h>
#include <iostream>
#include <stack>

Tree::Tree() {}

Tree::Tree(falcon::TreeType m_type, int m_class_num, int m_max_depth) {
  type = m_type;
  class_num = m_class_num;
  max_depth = m_max_depth;
  internal_node_num = 0;
  total_node_num = 0;
  // the maximum nodes, complete binary tree
  capacity = (int) pow(2, max_depth + 1) - 1;
  nodes = new Node[capacity];
}

Tree::~Tree() {
  delete [] nodes;
}

Tree::Tree(const Tree &tree) {
  type = tree.type;
  class_num = tree.class_num;
  max_depth = tree.max_depth;
  internal_node_num = tree.internal_node_num;
  total_node_num = tree.total_node_num;
  int maximum_nodes = (int) pow(2, max_depth + 1) - 1;
  nodes = new Node[maximum_nodes];
  for (int i = 0; i < maximum_nodes; i++) {
    nodes[i] = tree.nodes[i];
  }
  capacity = tree.capacity;
}

Tree& Tree::operator=(const Tree &tree) {
  type = tree.type;
  class_num = tree.class_num;
  max_depth = tree.max_depth;
  internal_node_num = tree.internal_node_num;
  total_node_num = tree.total_node_num;
  int maximum_nodes = (int) pow(2, max_depth + 1) - 1;
  nodes = new Node[maximum_nodes];
  for (int i = 0; i < maximum_nodes; i++) {
    nodes[i] = tree.nodes[i];
  }
  capacity = tree.capacity;
}

std::vector<int> Tree::comp_predict_vector(std::vector<double> sample,
    std::map<int, int> node_index_2_leaf_index_map) {
  std::vector<int> binary_vector(internal_node_num + 1);
  // traverse the whole tree iteratively, and compute binary_vector
  std::stack<PredictHelper> traverse_prediction_objs;
  PredictHelper prediction_obj((bool) nodes[0].node_type,
                               (bool) nodes[0].is_self_feature,
                               nodes[0].best_party_id,
                               nodes[0].best_feature_id,
                               nodes[0].best_split_id,
                               1,
                               0);
  traverse_prediction_objs.push(prediction_obj);
  while (!traverse_prediction_objs.empty()) {
    PredictHelper pred_obj = traverse_prediction_objs.top();
    if (pred_obj.is_leaf == 1) {
      // find leaf index and record
      int leaf_index = node_index_2_leaf_index_map.find(pred_obj.index)->second;
      binary_vector[leaf_index] = pred_obj.mark;
      traverse_prediction_objs.pop();
    } else if (pred_obj.is_self_feature != 1) {
      // both left and right branches are marked as 1 * current_mark
      traverse_prediction_objs.pop();
      int left_node_index = pred_obj.index * 2 + 1;
      int right_node_index = pred_obj.index * 2 + 2;

      PredictHelper left((bool) nodes[left_node_index].node_type,
                         (bool) nodes[left_node_index].is_self_feature,
                         nodes[left_node_index].best_party_id,
                         nodes[left_node_index].best_feature_id,
                         nodes[left_node_index].best_split_id,
                         pred_obj.mark,
                         left_node_index);
      PredictHelper right((bool) nodes[right_node_index].node_type,
                          (bool) nodes[right_node_index].is_self_feature,
                          nodes[right_node_index].best_party_id,
                          nodes[right_node_index].best_feature_id,
                          nodes[right_node_index].best_split_id,
                          pred_obj.mark,
                          right_node_index);
      traverse_prediction_objs.push(left);
      traverse_prediction_objs.push(right);
    } else {
      // is self feature, retrieve split value and compare
      traverse_prediction_objs.pop();
      int node_index = pred_obj.index;
      int feature_id = pred_obj.best_feature_id;
      int split_id = pred_obj.best_split_id;
      double split_value = nodes[node_index].split_threshold;
      int left_mark, right_mark;
      if (sample[feature_id] <= split_value) {
        left_mark = pred_obj.mark * 1;
        right_mark = pred_obj.mark * 0;
      } else {
        left_mark = pred_obj.mark * 0;
        right_mark = pred_obj.mark * 1;
      }

      int left_node_index = pred_obj.index * 2 + 1;
      int right_node_index = pred_obj.index * 2 + 2;
      PredictHelper left((bool) nodes[left_node_index].node_type,
                         (bool) nodes[left_node_index].is_self_feature,
                         nodes[left_node_index].best_party_id,
                         nodes[left_node_index].best_feature_id,
                         nodes[left_node_index].best_split_id,
                         left_mark,
                         left_node_index);
      PredictHelper right((bool) nodes[right_node_index].node_type,
                          (bool) nodes[right_node_index].is_self_feature,
                          nodes[right_node_index].best_party_id,
                          nodes[right_node_index].best_feature_id,
                          nodes[right_node_index].best_split_id,
                          right_mark,
                          right_node_index);
      traverse_prediction_objs.push(left);
      traverse_prediction_objs.push(right);
    }
  }
  return binary_vector;
}