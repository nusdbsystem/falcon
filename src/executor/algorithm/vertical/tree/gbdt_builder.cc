//
// Created by wuyuncheng on 31/7/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_model.h>

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