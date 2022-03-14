//
// Created by wuyuncheng on 9/6/21.
//

#include <falcon/algorithm/vertical/tree/forest_builder.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/logger/log_alg_params.h>

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
#include <falcon/model/model_io.h>

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
    double m_testing_accuracy) : ModelBuilder(std::move(m_training_data),
    std::move(m_testing_data),
    std::move(m_training_labels),
    std::move(m_testing_labels),
    m_training_accuracy,
    m_testing_accuracy) {
  n_estimator = params.n_estimator;
  sample_rate = params.sample_rate;
  dt_param = params.dt_param;
  tree_builders.reserve(n_estimator);
  forest_model = ForestModel(n_estimator, dt_param.tree_type);
  local_feature_num = training_data[0].size();
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
    // std::random_device rd;
    // using seed, for development
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine rng(seed);
    // auto rng = std::default_random_engine();
    std::shuffle(std::begin(sampled_data_indexes), std::end(sampled_data_indexes), rng);
    // sample training data for the decision tree
    sampled_data_indexes.resize(sampled_training_data_size);
    log_info("[RandomForestBuilder.shuffle_and_assign_training_data] sampled_data_indexes[0] = " + std::to_string(sampled_data_indexes[0]));
    log_info("[RandomForestBuilder.shuffle_and_assign_training_data] sampled_data_indexes[1] = " + std::to_string(sampled_data_indexes[1]));
    log_info("[RandomForestBuilder.shuffle_and_assign_training_data] sampled_data_indexes[2] = " + std::to_string(sampled_data_indexes[2]));
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

void RandomForestBuilder::train(Party party) {
  LOG(INFO) << "************ Begin to train the random forest model ************";
  init_forest_builder(party);
  LOG(INFO) << "Init " << n_estimator << " tree builders in the random forest";
  for (int tree_id = 0; tree_id < n_estimator; ++tree_id) {
    LOG(INFO) << "------------- build the " << tree_id << "-th tree -------------";
    tree_builders[tree_id].train(party);
    forest_model.forest_trees.emplace_back(tree_builders[tree_id].tree);
    google::FlushLogFiles(google::INFO);
  }
  LOG(INFO) << "End train the random forest";
  google::FlushLogFiles(google::INFO);
}

void RandomForestBuilder::eval(Party party, falcon::DatasetType eval_type,
    const std::string& report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Start *************";
  const clock_t testing_start_time = clock();

  // init test data
  int dataset_size = (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector< std::vector<double> > cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  // compute predictions
  // now the predicted labels are computed by mpc, thus it is already the final label
  EncodedNumber* predicted_labels = new EncodedNumber[dataset_size];
  forest_model.predict(party, cur_test_dataset, dataset_size, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  EncodedNumber* decrypted_labels = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(predicted_labels,
                              decrypted_labels,
                              dataset_size,
                              ACTIVE_PARTY_ID);

  // calculate accuracy by the active party
  std::vector<double> predictions;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // decode decrypted predicted labels
    for (int i = 0; i < dataset_size; i++) {
      double x;
      decrypted_labels[i].decode(x);
      predictions.push_back(x);
    }

    // compute accuracy
    if (tree_builders[0].tree_type == falcon::CLASSIFICATION) {
      int correct_num = 0;
      for (int i = 0; i < dataset_size; i++) {
        // LOG(INFO) << "prediction[" << i << "] = " << predictions[i] <<
        // ", ground truth label[" << i << "] = " << cur_test_dataset_labels[i];
        // the decoded label is returned from mpc program, and there is a
        // precision loss, so here check the precision less than a threshold
        // TODO: check how to return an integer from mpc properly
        if (abs(abs(cur_test_dataset_labels[i] - abs(predictions[i]))) < ROUNDED_PRECISION) {
          correct_num += 1;
        }
      }
      if (eval_type == falcon::TRAIN) {
        training_accuracy = (double) correct_num / dataset_size;
        LOG(INFO) << "Dataset size = " << dataset_size << ", correct predicted num = "
                  << correct_num << ", training accuracy = " << training_accuracy;
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = (double) correct_num / dataset_size;
        LOG(INFO) << "Dataset size = " << dataset_size << ", correct predicted num = "
                  << correct_num << ", testing accuracy = " << testing_accuracy;
      }
    } else {
      if (eval_type == falcon::TRAIN) {
        training_accuracy = mean_squared_error(predictions, cur_test_dataset_labels);
        LOG(INFO) << "Training accuracy = " << training_accuracy;
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = mean_squared_error(predictions, cur_test_dataset_labels);
        LOG(INFO) << "Testing accuracy = " << testing_accuracy;
      }
    }
  }

  // free memory
  delete [] predicted_labels;
  delete [] decrypted_labels;

  const clock_t testing_finish_time = clock();
  double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Evaluation time = " << testing_consumed_time;
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Finished *************";
  google::FlushLogFiles(google::INFO);
}

void RandomForestBuilder::distributed_train(const Party &party, const Worker &worker) {}

