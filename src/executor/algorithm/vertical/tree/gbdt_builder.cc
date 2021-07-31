//
// Created by wuyuncheng on 31/7/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/model/model_io.h>

#include <glog/logging.h>

#include <iomanip>
#include <random>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <map>
#include <stack>

GbdtBuilder::GbdtBuilder() {}

GbdtBuilder::~GbdtBuilder() {
  // automatically memory free
}

GbdtBuilder::GbdtBuilder(GbdtParams gbdt_params,
    std::vector<std::vector<double> > m_training_data,
    std::vector<std::vector<double> > m_testing_data,
    std::vector<double> m_training_labels,
    std::vector<double> m_testing_labels,
    double m_training_accuracy,
    double m_testing_accuracy) : ModelBuilder(std::move(m_training_data),
        std::move(m_testing_data),
        std::move(m_training_labels),
        std::move(m_testing_labels),
        m_training_accuracy,
        m_testing_accuracy){
  n_estimator = gbdt_params.n_estimator;
  loss = gbdt_params.loss;
  learning_rate = gbdt_params.learning_rate;
  subsample = gbdt_params.subsample;
  dt_param = gbdt_params.dt_param;
  int tree_size;
  if (dt_param.tree_type == "classification") {
    tree_size = n_estimator * dt_param.class_num;
  } else {
    tree_size = n_estimator;
  }
  gbdt_model = GbdtModel(tree_size, dt_param.tree_type,
      n_estimator, dt_param.class_num);
  tree_builders.reserve(tree_size);
  local_feature_num = training_data[0].size();
}

void GbdtBuilder::train(Party party) {

}

void GbdtBuilder::eval(Party party, falcon::DatasetType eval_type, const string &report_save_path) {

}

void train_gbdt(Party party, const std::string& params_str,
                const std::string& model_save_file, const std::string& model_report_file) {
  LOG(INFO) << "Run the example gbdt model train";
  std::cout << "Run the example gbdt model train" << std::endl;

  GbdtParams params;
  // currently for testing
  params.n_estimator = 8;
  params.learning_rate = 1.0;
  params.subsample = 1.0;
  params.loss = "deviance";
  params.dt_param.tree_type = "classification";
  params.dt_param.criterion = "gini";
  params.dt_param.split_strategy = "best";
  params.dt_param.class_num = 2;
  params.dt_param.max_depth = 3;
  params.dt_param.max_bins = 8;
  params.dt_param.min_samples_split = 5;
  params.dt_param.min_samples_leaf = 5;
  params.dt_param.max_leaf_nodes = 16;
  params.dt_param.min_impurity_decrease = 0.01;
  params.dt_param.min_impurity_split = 0.001;
  params.dt_param.dp_budget = 0.1;
  //  deserialize_gbdt_params(params, params_str);
  int weight_size = party.getter_feature_num();
  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;

  std::vector< std::vector<double> > training_data;
  std::vector< std::vector<double> > testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  party.split_train_test_data(split_percentage,
                              training_data,
                              testing_data,
                              training_labels,
                              testing_labels);

  LOG(INFO) << "Init gbdt model builder";
  LOG(INFO) << "params.n_estimator = " << params.n_estimator;
  LOG(INFO) << "params.loss = " << params.loss;
  LOG(INFO) << "params.learning_rate = " << params.learning_rate;
  LOG(INFO) << "params.subsample = " << params.subsample;
  LOG(INFO) << "params.dt_param.tree_type = " << params.dt_param.tree_type;
  LOG(INFO) << "params.dt_param.criterion = " << params.dt_param.criterion;
  LOG(INFO) << "params.dt_param.split_strategy = " << params.dt_param.split_strategy;
  LOG(INFO) << "params.dt_param.class_num = " << params.dt_param.class_num;
  LOG(INFO) << "params.dt_param.max_depth = " << params.dt_param.max_depth;
  LOG(INFO) << "params.dt_param.max_bins = " << params.dt_param.max_bins;
  LOG(INFO) << "params.dt_param.min_samples_split = " << params.dt_param.min_samples_split;
  LOG(INFO) << "params.dt_param.min_samples_leaf = " << params.dt_param.min_samples_leaf;
  LOG(INFO) << "params.dt_param.max_leaf_nodes = " << params.dt_param.max_leaf_nodes;
  LOG(INFO) << "params.dt_param.min_impurity_decrease = " << params.dt_param.min_impurity_decrease;
  LOG(INFO) << "params.dt_param.min_impurity_split = " << params.dt_param.min_impurity_split;
  LOG(INFO) << "params.dt_param.dp_budget = " << params.dt_param.dp_budget;

  GbdtBuilder gbdt_builder(params,
      training_data,
      testing_data,
      training_labels,
      testing_labels,
      training_accuracy,
      testing_accuracy);

  LOG(INFO) << "Init random forest model finished";
  std::cout << "Init random forest model finished" << std::endl;
  google::FlushLogFiles(google::INFO);

  gbdt_builder.train(party);
  gbdt_builder.eval(party, falcon::TRAIN);
  gbdt_builder.eval(party, falcon::TEST);

  std::string pb_gbdt_model_string;
  serialize_gbdt_model(gbdt_builder.gbdt_model, pb_gbdt_model_string);
  save_pb_model_string(pb_gbdt_model_string, model_save_file);
  save_training_report(gbdt_builder.getter_training_accuracy(),
                       gbdt_builder.getter_testing_accuracy(),
                       model_report_file);

  LOG(INFO) << "Trained model and report saved";
  std::cout << "Trained model and report saved" << std::endl;
  google::FlushLogFiles(google::INFO);
}