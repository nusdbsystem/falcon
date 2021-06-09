//
// Created by wuyuncheng on 9/6/21.
//

#include <falcon/algorithm/vertical/tree/forest_builder.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/math/math_ops.h>

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

void RandomForestBuilder::train(Party &party) {
  LOG(INFO) << "************ Begin to train the random forest model ************";
  init_forest_builder(party);
  LOG(INFO) << "Init " << n_estimator << " tree builders in the random forest";
  for (int tree_id = 0; tree_id < n_estimator; ++tree_id) {
    LOG(INFO) << "------------- build the " << tree_id << "-th tree -------------";
    tree_builders[tree_id].train(party);
    google::FlushLogFiles(google::INFO);
  }
  LOG(INFO) << "End train the random forest";
  google::FlushLogFiles(google::INFO);
}

void RandomForestBuilder::eval(Party party, falcon::DatasetType eval_type) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Start *************";
  const clock_t testing_start_time = clock();

  // init test data
  int dataset_size = (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector< std::vector<double> > cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  // init predicted forest labels to record the predictions, the first dimension
  // is the number of trees in the forest, and the second dimension is the number
  // of the samples in the predicted dataset
  EncodedNumber** predicted_forest_labels = new EncodedNumber*[n_estimator];
  EncodedNumber** decrypted_predicted_forest_labels = new EncodedNumber*[n_estimator];
  for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
    predicted_forest_labels[tree_id] = new EncodedNumber[dataset_size];
    decrypted_predicted_forest_labels[tree_id] = new EncodedNumber[dataset_size];
  }

  // compute predictions
  for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
    // compute predictions for each tree in the forest
    tree_builders[tree_id].tree.predict(party,
        cur_test_dataset,
        dataset_size,
        predicted_forest_labels[tree_id]);
    // decrypt the predicted labels and compute the accuracy
    party.collaborative_decrypt(predicted_forest_labels[tree_id],
        decrypted_predicted_forest_labels[tree_id],
        dataset_size, ACTIVE_PARTY_ID);
  }

  // calculate accuracy by the super client
  std::vector<double> predictions;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // decode decrypted predicted labels
    std::vector< std::vector<double> > decoded_predicted_forest_labels (
        n_estimator, std::vector<double>(dataset_size));
    for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
      for (int i = 0; i < dataset_size; i++) {
        decrypted_predicted_forest_labels[tree_id][i].decode(decoded_predicted_forest_labels[tree_id][i]);
      }
    }
    // compute predicted label
    for (int i = 0; i < dataset_size; i++) {
      std::vector<double> forest_labels_i;
      for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
        forest_labels_i.push_back(decoded_predicted_forest_labels[tree_id][i]);
      }
      // compute mode or average, depending on the tree type
      if (tree_builders[0].tree_type == falcon::CLASSIFICATION) {
        predictions.push_back(mode(forest_labels_i));
      } else {
        predictions.push_back(average(forest_labels_i));
      }
    }

    // compute accuracy
    if (tree_builders[0].tree_type == falcon::CLASSIFICATION) {
      int correct_num = 0;
      for (int i = 0; i < dataset_size; i++) {
        if (predictions[i] == cur_test_dataset_labels[i]) {
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
  for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
    delete [] predicted_forest_labels[tree_id];
    delete [] decrypted_predicted_forest_labels[tree_id];
  }
  delete [] predicted_forest_labels;
  delete [] decrypted_predicted_forest_labels;

  const clock_t testing_finish_time = clock();
  double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Evaluation time = " << testing_consumed_time;
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Finished *************";
  google::FlushLogFiles(google::INFO);
}

void train_random_forest(Party party, const std::string& params_str,
    const std::string& model_save_file, const std::string& model_report_file) {

  LOG(INFO) << "Run the example random forest train";
  std::cout << "Run the example random forest train" << std::endl;

  RandomForestParams params;
  // currently for testing
  params.n_estimator = 8;
  params.sample_rate = 0.8;
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
//  deserialize_rf_params(params, params_str);
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

  LOG(INFO) << "Init random forest model builder";
  LOG(INFO) << "params.n_estimator = " << params.n_estimator;
  LOG(INFO) << "params.sample_rate = " << params.sample_rate;
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

  std::cout << "Init decision tree model" << std::endl;
  LOG(INFO) << "Init decision tree model";

  RandomForestBuilder random_forest_builder(params,
      training_data,
      testing_data,
      training_labels,
      testing_labels,
      training_accuracy,
      testing_accuracy);

  LOG(INFO) << "Init random forest model finished";
  std::cout << "Init random forest model finished" << std::endl;
  google::FlushLogFiles(google::INFO);

  random_forest_builder.train(party);
  random_forest_builder.eval(party, falcon::TRAIN);
  random_forest_builder.eval(party, falcon::TEST);

  std::vector<Tree> forest_trees;
  for (int i = 0; i < random_forest_builder.n_estimator; i++) {
    forest_trees.push_back(random_forest_builder.tree_builders[i].tree);
  }
  save_rf_model(forest_trees, random_forest_builder.n_estimator, model_save_file);
  save_training_report(random_forest_builder.getter_training_accuracy(),
      random_forest_builder.getter_testing_accuracy(),
      model_report_file);
}