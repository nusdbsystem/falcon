//
// Created by wuyuncheng on 9/6/21.
//

#include <falcon/algorithm/vertical/tree/forest_builder.h>
#include <falcon/utils/pb_converter/common_converter.h>

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

RandomForestBuilder::RandomForestBuilder() {}

RandomForestBuilder::~RandomForestBuilder() {
  // automatically memory free
}

RandomForestBuilder::RandomForestBuilder(RandomForestParams params,
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
  n_estimator = params.n_estimator;
  sample_rate = params.sample_rate;
  dt_param = params.dt_param;
  tree_builders.reserve(n_estimator);
}

void RandomForestBuilder::init_forest_builder(Party &party) {
  for (int tree_id = 0; tree_id < n_estimator; ++tree_id) {
    // shuffle the training data with sample rate and init a tree builder
    std::vector< std::vector<double> > sampled_training_data;
    std::vector<double> sampled_training_labels;
    shuffle_and_assign_training_data(party,
        tree_id,
        sampled_training_data,
        sampled_training_labels);
    tree_builders.emplace_back(dt_param,
        sampled_training_data, testing_data,
        sampled_training_labels, testing_labels,
        training_accuracy, testing_accuracy);
  }
  LOG(INFO) << "Init " << n_estimator << " trees in the random forest";
  std::cout << "Init " << n_estimator << " trees in the random forest" << std::endl;
}

void RandomForestBuilder::shuffle_and_assign_training_data(Party &party,
    int tree_id,
    std::vector< std::vector<double> >& sampled_training_data,
    std::vector<double> &sampled_training_labels) {
  int sampled_training_data_size = (int) (training_data.size() * sample_rate);
  std::vector<int> sampled_data_indexes;
  LOG(INFO) << "Sampled training data size = " << sampled_training_data_size;
  // for active party
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // store the indexes of the training dataset for random batch selection
    for (int i = 0; i < training_data.size(); i++) {
      sampled_data_indexes.push_back(i);
    }
    // shuffle the training data
    std::random_device rd;
    std::default_random_engine rng(rd());
    //auto rng = std::default_random_engine();
    std::shuffle(std::begin(sampled_data_indexes), std::end(sampled_data_indexes), rng);
    // sample training data for the decision tree
    sampled_data_indexes.resize(sampled_training_data_size);
    // assign the training dataset and labels
    for (int i = 0; i < sampled_data_indexes.size(); i++) {
      sampled_training_data.push_back(training_data[sampled_data_indexes[i]]);
      sampled_training_labels.push_back(training_labels[sampled_data_indexes[i]]);
    }
    // serialize the data_indexes and send to passive party
    std::string sampled_data_indexes_str;
    serialize_int_array(sampled_data_indexes, sampled_data_indexes_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, sampled_data_indexes_str);
      }
    }
  }
  // for passive party
  if (party.party_type == falcon::PASSIVE_PARTY) {
    std::string recv_sampled_data_indexes_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_sampled_data_indexes_str);
    deserialize_int_array(sampled_data_indexes, recv_sampled_data_indexes_str);
    for (int i = 0; i < sampled_training_data_size; i++) {
      sampled_training_data.push_back(training_data[sampled_data_indexes[i]]);
    }
  }
  LOG(INFO) << "Shuffle training data and init dataset for tree " << tree_id << " finished";
}