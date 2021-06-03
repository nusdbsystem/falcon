//
// Created by wuyuncheng on 13/5/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_TREE_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_TREE_H_

#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/node.h>
#include <map>

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
  // tree capacity
  int capacity;

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
  Tree &operator=(const Tree &tree);

  /**
   * compute the binary predict vector for a sample on a party
   */
  std::vector<int> comp_predict_vector(std::vector<double> sample,
      std::map<int, int> node_index_2_leaf_index_map);
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

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_TREE_H_
