//
// Created by wuyuncheng on 31/7/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_model.h>

GbdtModel::GbdtModel() {}

GbdtModel::GbdtModel(int m_tree_size, std::string m_tree_type,
    int m_n_estimator, int m_class_num, double m_learning_rate) {
  tree_size = m_tree_size;
  // copy builder parameters
  if (m_tree_type == "classification") {
    tree_type = falcon::CLASSIFICATION;
  } else {
    tree_type = falcon::REGRESSION;
  }
  n_estimator = m_n_estimator;
  class_num = m_class_num;
  learning_rate = m_learning_rate;
  gbdt_trees.reserve(tree_size);
}

GbdtModel::~GbdtModel() {}

GbdtModel::GbdtModel(const GbdtModel &gbdt_model) {
  tree_size = gbdt_model.tree_size;
  tree_type = gbdt_model.tree_type;
  n_estimator = gbdt_model.n_estimator;
  class_num = gbdt_model.class_num;
  learning_rate = gbdt_model.learning_rate;
  gbdt_trees = gbdt_model.gbdt_trees;
}

GbdtModel& GbdtModel::operator=(const GbdtModel &gbdt_model) {
  tree_size = gbdt_model.tree_size;
  tree_type = gbdt_model.tree_type;
  n_estimator = gbdt_model.n_estimator;
  class_num = gbdt_model.class_num;
  learning_rate = gbdt_model.learning_rate;
  gbdt_trees = gbdt_model.gbdt_trees;
}

void GbdtModel::predict(Party &party,
                        std::vector<std::vector<double> > predicted_samples,
                        int predicted_sample_size,
                        EncodedNumber *predicted_labels) {
  // to be added
}