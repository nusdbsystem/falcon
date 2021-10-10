//
// Created by wuyuncheng on 9/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FOREST_MODEL_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FOREST_MODEL_H_

#include <falcon/common.h>
#include <falcon/party/party.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>

#include <thread>
#include <future>

class ForestModel {
 public:
  // number of trees in the model
  int tree_size;
  // type of the tree, 'classification' or 'regression'
  falcon::TreeType tree_type;
  // vector of tree models
  std::vector<TreeModel> forest_trees;

 public:
  ForestModel();
  ForestModel(int m_tree_size, std::string m_tree_type);
  ~ForestModel();

  /**
   * copy constructor
   * @param forest_model
   */
  ForestModel(const ForestModel &forest_model);

  /**
   * assignment constructor
   * @param forest_model
   * @return
   */
  ForestModel &operator=(const ForestModel &forest_model);

  /**
 * given the forest model, predict on samples
 * @param party
 * @param predicted_samples
 * @param predicted_sample_size
 * @param predicted_labels
 * @return predicted labels (encrypted)
 */
  void predict(Party& party,
               const std::vector< std::vector<double> >& predicted_samples,
               int predicted_sample_size,
               EncodedNumber* predicted_labels);
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FOREST_MODEL_H_
