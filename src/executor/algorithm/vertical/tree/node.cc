//
// Created by wuyuncheng on 12/5/21.
//

#include <falcon/algorithm/vertical/tree/node.h>

#include <glog/logging.h>
#include <limits>

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

Node::~Node() {}

void Node::print_node() {
  LOG(INFO) << "Node depth: " << depth;
  LOG(INFO) << "Node type: " << node_type;
  LOG(INFO) << "Node is self feature: " << is_self_feature;
  LOG(INFO) << "Node best party id: " << best_party_id;
  LOG(INFO) << "Node best feature id: " << best_feature_id;
  LOG(INFO) << "Node best split id: " << best_split_id;
  LOG(INFO) << "Node sample num: " << node_sample_num;
  LOG(INFO) << "Node split threshold: " << split_threshold;
  // assume that the impurity and label are plaintext
  double decoded_impurity, decoded_label;
  impurity.decode(decoded_impurity);
  LOG(INFO) << "Node impurity: " << decoded_impurity;
  if (node_type == falcon::LEAF) {
    label.decode(decoded_label);
    LOG(INFO) << "Node label: " << decoded_label;
  }
}