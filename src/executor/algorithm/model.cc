//
// Created by wuyuncheng on 14/11/20.
//

#include <falcon/algorithm/model.h>

Model::Model() {
  training_accuracy = 0.0;
  testing_accuracy = 0.0;
}

Model::Model(std::vector<std::vector<double> > m_training_data,
             std::vector<std::vector<double> > m_testing_data,
             std::vector<double> m_training_labels,
             std::vector<double> m_testing_labels,
             double m_training_accuracy,
             double m_testing_accuracy) {
  training_data = m_training_data;
  training_labels = m_training_labels;
  training_accuracy = m_training_accuracy;
  testing_data = m_testing_data;
  testing_labels = m_testing_labels;
  testing_accuracy = m_testing_accuracy;
}

Model::~Model() {}