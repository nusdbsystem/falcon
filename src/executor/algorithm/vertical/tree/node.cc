//
// Created by wuyuncheng on 12/5/21.
//

#include <falcon/algorithm/vertical/tree/node.h>

#include <glog/logging.h>

Node::Node() {
  node_type = falcon::INTERNAL;
  depth = -1;
  is_self_feature = -1;
  best_party_id = -1;
  best_feature_id = -1;
  best_split_id = -1;
  left_child = -1;
  right_child = -1;
  available_global_feature_num = -1;
}

Node::Node(int m_depth,
    int m_sample_iv_size,
    int m_class_num,
    EncodedNumber *m_sample_iv,
    EncodedNumber *m_encrypted_labels) {
  node_type = falcon::INTERNAL;
  depth = m_depth;
  is_self_feature = -1;
  best_party_id = -1;
  best_feature_id = -1;
  best_split_id = -1;
  sample_iv_size = m_sample_iv_size;
  sample_iv = new EncodedNumber[sample_iv_size];
  for (int i = 0; i < sample_iv_size; i++) {
    sample_iv[i] = m_sample_iv[i];
  }
  class_num = m_class_num;
  encrypted_labels = new EncodedNumber[class_num * sample_iv_size];
  for (int i = 0; i < class_num * sample_iv_size; i++) {
    encrypted_labels[i] = m_encrypted_labels[i];
  }
  left_child = -1;
  right_child = -1;
  available_global_feature_num = -1;
}


Node::Node(const Node &node) {
  node_type = node.node_type;
  depth = node.depth;
  is_self_feature = node.is_self_feature;
  best_party_id = node.best_party_id;
  best_feature_id = node.best_feature_id;
  best_split_id = node.best_split_id;
  available_feature_ids = node.available_feature_ids;
  available_global_feature_num = node.available_global_feature_num;
  sample_iv_size = node.sample_iv_size;
  class_num = node.class_num;
  impurity = node.impurity;
  sample_iv = new EncodedNumber[sample_iv_size];
  for (int i = 0; i < sample_iv_size; i++) {
    sample_iv[i] = node.sample_iv[i];
  }
  encrypted_labels = new EncodedNumber[class_num * sample_iv_size];
  for (int i = 0; i < class_num * sample_iv_size; i++) {
    encrypted_labels[i] = node.encrypted_labels[i];
  }
  left_child = node.left_child;
  right_child = node.right_child;
  node_sample_num = node.get_node_sample_num();
  node_sample_distribution = node.get_node_sample_distribution();
  node.get_impurity(impurity);
  node.get_label(label);
}

Node& Node::operator=(Node *node) {
  node_type = node->node_type;
  depth = node->depth;
  is_self_feature = node->is_self_feature;
  best_party_id = node->best_party_id;
  best_feature_id = node->best_feature_id;
  best_split_id = node->best_split_id;
  available_feature_ids = node->available_feature_ids;
  available_global_feature_num = node->available_global_feature_num;
  sample_iv_size = node->sample_iv_size;
  class_num = node->class_num;
  sample_iv = new EncodedNumber[sample_iv_size];
  for (int i = 0; i < sample_iv_size; i++) {
    sample_iv[i] = node->sample_iv[i];
  }
  encrypted_labels = new EncodedNumber[class_num * sample_iv_size];
  for (int i = 0; i < class_num; i++) {
    encrypted_labels[i] = node->encrypted_labels[i];
  }
  left_child = node->left_child;
  right_child = node->right_child;
  node_sample_num = node->get_node_sample_num();
  node_sample_distribution = node->get_node_sample_distribution();
  node->get_impurity(impurity);
  node->get_label(label);
}

Node::~Node() {
  // need to further check if it is safe to delete these values
  delete [] sample_iv;
  delete [] encrypted_labels;
}

void Node::set_node_sample_num(int s_node_sample_num) {
  node_sample_num = s_node_sample_num;
}

void Node::set_node_sample_distribution(std::vector<int> s_node_sample_distribution) {
  node_sample_distribution = s_node_sample_distribution;
}

void Node::set_impurity(EncodedNumber s_impurity) {
  impurity = s_impurity;
}

void Node::set_label(EncodedNumber s_label) {
  label = s_label;
}

int Node::get_node_sample_num() const {
  return node_sample_num;
}

std::vector<int> Node::get_node_sample_distribution() const {
  return node_sample_distribution;
}

void Node::get_impurity(EncodedNumber &g_impurity) const {
  g_impurity = impurity;
}

void Node::get_label(EncodedNumber &g_label) const {
  g_label = label;
}

void Node::print_node() {
  LOG(INFO) << "Node depth: " << depth;
  LOG(INFO) << "Node type: " << node_type;
  LOG(INFO) << "Node is self feature: " << is_self_feature;
  LOG(INFO) << "Node best party id: " << best_party_id;
  LOG(INFO) << "Node best feature id: " << best_feature_id;
  LOG(INFO) << "Node best split id: " << best_split_id;
  LOG(INFO) << "Node sample num: " << node_sample_num;
  // assume that the impurity and label are plaintext
  double decoded_impurity, decoded_label;
  impurity.decode(decoded_impurity);
  LOG(INFO) << "Node impurity: " << decoded_impurity;
  if (node_type == falcon::LEAF) {
    label.decode(decoded_label);
    LOG(INFO) << "Node label: " << decoded_label;
  }
}