//
// Created by wuyuncheng on 9/7/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/algorithm/vertical/tree/forest_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>

#include <cmath>
#include <glog/logging.h>
#include <iostream>

ForestModel::ForestModel() {}

ForestModel::ForestModel(int m_tree_size) {
  tree_size = m_tree_size;
  forest_trees.reserve(tree_size);
}

ForestModel::~ForestModel() {}

ForestModel::ForestModel(const ForestModel &forest_model) {
  tree_size = forest_model.tree_size;
  forest_trees = forest_model.forest_trees;
}

ForestModel& ForestModel::operator=(const ForestModel &forest_model) {
  tree_size = forest_model.tree_size;
  forest_trees = forest_model.forest_trees;
}

void ForestModel::predict(Party &party,
                          std::vector<std::vector<double> > predicted_samples,
                          int predicted_sample_size,
                          EncodedNumber *predicted_labels) {

}