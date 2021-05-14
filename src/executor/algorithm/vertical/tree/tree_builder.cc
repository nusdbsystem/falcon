//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>

DecisionTreeBuilder::DecisionTreeBuilder() {}

DecisionTreeBuilder::~DecisionTreeBuilder() {
  delete [] feature_helpers;
}

DecisionTreeBuilder::DecisionTreeBuilder(DecisionTreeParams params,
    std::vector<std::vector<double> > m_training_data,
    std::vector<std::vector<double> > m_testing_data,
    std::vector<double> m_training_labels,
    std::vector<double> m_testing_labels,
    double m_training_accuracy,
    double m_testing_accuracy) : Model(std::move(m_training_data),
          std::move(m_testing_data),
          std::move(m_training_labels),
          std::move(m_testing_labels),
          m_training_accuracy,
          m_testing_accuracy) {
  // copy builder parameters
  if (params.tree_type == "classification") {
    tree_type = falcon::CLASSIFICATION;
    class_num = params.class_num;
  } else {
    tree_type = falcon::REGRESSION;
    // for regression, the class num is set to 2 for y and y^2
    class_num = REGRESSION_TREE_CLASS_NUM;
  }
  criterion = params.criterion;
  split_strategy = params.split_strategy;
  max_depth = params.max_depth;
  max_bins = params.max_bins;
  min_samples_split = params.min_samples_split;
  min_samples_leaf = params.min_samples_leaf;
  max_leaf_nodes = params.max_leaf_nodes;
  min_impurity_decrease = params.min_impurity_decrease;
  min_impurity_split = params.min_impurity_split;
  dp_budget = params.dp_budget;
  // init feature types to continuous by default
  local_feature_num = m_training_data[0].size();
  for (int i = 0; i < local_feature_num; i++) {
    feature_types.push_back(falcon::CONTINUOUS);
  }
  feature_helpers = new FeatureHelper[local_feature_num];
  // init tree object
  tree = Tree(tree_type, class_num, max_depth);
}

void DecisionTreeBuilder::precompute_label_helper(falcon::PartyType party_type) {
  if (party_type == falcon::PASSIVE_PARTY) {
    return;
  }
  // only the active party has label information
  // pre-compute indicator vectors or variance vectors for labels
  // here already assume that it is computed on active party
  LOG(INFO) << "Pre-compute label assist info for training";
  if (tree_type == falcon::CLASSIFICATION) {
    // classification, compute binary vectors and store
    int * sample_num_classes = new int[class_num];
    for (int i = 0; i < class_num; i++) {
      sample_num_classes[i] = 0;
    }
    for (int i = 0; i < class_num; i++) {
      std::vector<int> indicator_vec;
      for (int j = 0; j < training_labels.size(); j++) {
        if (training_labels[j] == (double) i) {
          indicator_vec.push_back(1);
          sample_num_classes[i] += 1;
        } else {
          indicator_vec.push_back(0);
        }
      }
      indicator_class_vecs.push_back(indicator_vec);
    }
    for (int i = 0; i < class_num; i++) {
      LOG(INFO) << "Class " << i << "'s sample num = " << sample_num_classes[i];
    }
    delete [] sample_num_classes;
  } else {
    // regression, compute variance necessary stats
    std::vector<double> label_square_vec;
    for (int i = 0; i < training_labels.size(); i++) {
      label_square_vec.push_back(training_labels[i] * training_labels[i]);
    }
    variance_stat_vecs.push_back(training_labels); // the first vector is the actual label vector
    variance_stat_vecs.push_back(label_square_vec);     // the second vector is the squared label vector
  }
}

void DecisionTreeBuilder::precompute_feature_helpers() {
  LOG(INFO) << "Pre-compute feature helpers for training";
  for (int i = 0; i < local_feature_num; i++) {
    // 1. extract feature values of the i-th feature, compute samples_num
    // 2. check if distinct values number <= max_bins, if so, update splits_num as distinct number
    // 3. init feature, and assign to features[i]
    std::vector<double> feature_values;
    for (int j = 0; j < training_data.size(); j++) {
      feature_values.push_back(training_data[j][i]);
    }
    feature_helpers[i].id = i;
    feature_helpers[i].is_used = 0;
    feature_helpers[i].feature_type = feature_types[i];
    feature_helpers[i].num_splits = max_bins - 1;
    feature_helpers[i].max_bins = max_bins;
    feature_helpers[i].set_feature_data(feature_values, training_data.size());
    feature_helpers[i].sort_feature();
    feature_helpers[i].find_splits();
    feature_helpers[i].compute_split_ivs();
  }
}