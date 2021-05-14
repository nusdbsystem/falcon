//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/tree.h>
#include <cmath>

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
  delete nodes;
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
}

Tree& Tree::operator=(Tree *tree) {
  type = tree->type;
  class_num = tree->class_num;
  max_depth = tree->max_depth;
  internal_node_num = tree->internal_node_num;
  total_node_num = tree->total_node_num;
  int maximum_nodes = (int) pow(2, max_depth + 1) - 1;
  nodes = new Node[maximum_nodes];
  for (int i = 0; i < maximum_nodes; i++) {
    nodes[i] = tree->nodes[i];
  }
}