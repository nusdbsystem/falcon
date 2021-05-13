//
// Created by wuyuncheng on 13/5/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_TREE_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_TREE_H_

#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/node.h>

class Tree {
 public:
  // classification or regression
  falcon::TreeType type;
  // number of classes if classification
  int class_num;
  // maximum tree depth
  int max_depth;
  // array of Node
  Node* nodes;
  // internal node count
  int internal_node_num;
  // total node count
  int total_node_num;

 public:
  Tree();
  Tree(falcon::TreeType m_type, int m_class_num, int m_max_depth);
  ~Tree();

  /**
   * copy constructor
   * @param tree
   */
  Tree(const Tree &tree);

  /**
   * assignment constructor
   * @param tree
   * @return
   */
  Tree &operator=(Tree *tree);
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_TREE_H_
