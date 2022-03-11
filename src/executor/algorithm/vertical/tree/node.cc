//
// Created by wuyuncheng on 12/5/21.
//

#include <falcon/algorithm/vertical/tree/node.h>

#include <glog/logging.h>
#include <limits>
#include <falcon/utils/logger/logger.h>

Node::Node() {
  node_type = falcon::INTERNAL;
  depth = -1;
  is_self_feature = -1;
  best_party_id = -1;
  best_feature_id = -1;
  best_split_id = -1;
  left_child = -1;
  right_child = -1;
  split_threshold = std::numeric_limits<double>::min();
  node_sample_num = 0;
}

Node::Node(const Node &node) {
  node_type = node.node_type;
  depth = node.depth;
  is_self_feature = node.is_self_feature;
  best_party_id = node.best_party_id;
  best_feature_id = node.best_feature_id;
  best_split_id = node.best_split_id;
  split_threshold = node.split_threshold;
  node_sample_num = node.node_sample_num;
  node_sample_distribution = node.node_sample_distribution;
  impurity = node.impurity;
  label = node.label;
  left_child = node.left_child;
  right_child = node.right_child;
}

Node& Node::operator=(const Node &node) {
  node_type = node.node_type;
  depth = node.depth;
  is_self_feature = node.is_self_feature;
  best_party_id = node.best_party_id;
  best_feature_id = node.best_feature_id;
  best_split_id = node.best_split_id;
  split_threshold = node.split_threshold;
  node_sample_num = node.node_sample_num;
  node_sample_distribution = node.node_sample_distribution;
  impurity = node.impurity;
  label = node.label;
  left_child = node.left_child;
  right_child = node.right_child;
}

Node::~Node() = default;

void Node::print_node() {
  log_info("[Node.print_node]: node node_type = " + std::to_string(node_type));
  log_info("[Node.print_node]: node depth = " + std::to_string(depth));
  log_info("[Node.print_node]: node is_self_feature = " + std::to_string(is_self_feature));
  log_info("[Node.print_node]: node best_party_id = " + std::to_string(best_party_id));
  log_info("[Node.print_node]: node best_feature_id = " + std::to_string(best_feature_id));
  log_info("[Node.print_node]: node best_split_id = " + std::to_string(best_split_id));
  log_info("[Node.print_node]: node left_child = " + std::to_string(left_child));
  log_info("[Node.print_node]: node right_child = " + std::to_string(right_child));
  log_info("[Node.print_node]: node split_threshold = " + std::to_string(split_threshold));
  log_info("[Node.print_node]: node node_sample_num = " + std::to_string(node_sample_num));

  // assume that the impurity and label are plaintext
  double decoded_impurity, decoded_label;
  impurity.decode(decoded_impurity);
  log_info("[Node.print_node]: node impurity: " + std::to_string(decoded_impurity));
  if (node_type == falcon::LEAF) {
    log_info("[Node.print_node] node label type: " + std::to_string(label.getter_type()));
    label.decode(decoded_label);
    log_info("[Node.print_node]: node label: " + std::to_string(decoded_label));
  }
}