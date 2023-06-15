/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by root on 3/12/22.
//

#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_ps.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_ps.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>
#include <falcon/algorithm/vertical/nn/mlp_ps.h>
#include <falcon/algorithm/vertical/tree/tree_ps.h>
#include <falcon/algorithm/vertical/vfl_entry.h>
#include <falcon/model/model_io.h>
#include <falcon/utils/logger/log_alg_params.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/nn_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>

// initializes the LinearRegressionBuilder instance
// and run .train() .eval() methods, then save model
void train_linear_regression(Party *party, const std::string &params_str,
                             const std::string &model_save_file,
                             const std::string &model_report_file,
                             int is_distributed_train, Worker *worker) {
  log_info("Run train_linear_regression");
  log_info("is_distributed_train = " + std::to_string(is_distributed_train));

  LinearRegressionParams params;
  deserialize_lir_params(params, params_str);
  log_linear_regression_params(params);

  // split train test data for party and populate the vectors
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  // if not distributed train, then the party split the data
  // otherwise, the party/worker receive the data and phe keys from ps
  if (is_distributed_train == 0) {
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    split_dataset(party, params.fit_bias, training_data, testing_data,
                  training_labels, testing_labels, split_percentage);
  } else {
    // here should receive the train/test data/labels from ps
    std::string recv_training_data_str, recv_testing_data_str;
    std::string recv_training_labels_str, recv_testing_labels_str;
    worker->recv_long_message_from_ps(recv_training_data_str);
    worker->recv_long_message_from_ps(recv_testing_data_str);
    deserialize_double_matrix(training_data, recv_training_data_str);
    deserialize_double_matrix(testing_data, recv_testing_data_str);
    if (party->party_type == falcon::ACTIVE_PARTY) {
      worker->recv_long_message_from_ps(recv_training_labels_str);
      worker->recv_long_message_from_ps(recv_testing_labels_str);
      deserialize_double_array(training_labels, recv_training_labels_str);
      deserialize_double_array(testing_labels, recv_testing_labels_str);
    }
    // also, receive the phe keys from ps
    // and set these to the party
    std::string recv_phe_keys_str;
    log_info("begin to receive phe keys from ps ");
    worker->recv_long_message_from_ps(recv_phe_keys_str);
    log_info("received phe keys from ps: " + recv_phe_keys_str);
    party->load_phe_key_string(recv_phe_keys_str);
    // set the weight size and party feature num
    party->setter_feature_num(static_cast<int>(training_data[0].size()));
  }

  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();
  double training_accuracy = 0.0, testing_accuracy = 0.0;
  LinearRegressionBuilder linear_reg_builder(
      params, weight_size, training_data, testing_data, training_labels,
      testing_labels, training_accuracy, testing_accuracy);

  if (is_distributed_train == 0) {
    linear_reg_builder.train(*party);
    //    linear_reg_builder.eval(*party, falcon::TRAIN, model_report_file);
    linear_reg_builder.eval(*party, falcon::TEST, model_report_file);
    // save model and report
    auto *model_weights = new EncodedNumber[weight_size];
    std::string pb_lr_model_string;
    serialize_lr_model(linear_reg_builder.linear_reg_model, pb_lr_model_string);
    save_pb_model_string(pb_lr_model_string, model_save_file);
    // save_training_report(log_reg_model.getter_training_accuracy(),
    //    log_reg_model.getter_testing_accuracy(),
    //    model_report_file);
    delete[] model_weights;
  } else {
    linear_reg_builder.distributed_train(*party, *worker);
    // in is_distributed_train, parameter server will save the model.
    //    linear_reg_builder.distributed_eval(*party, *worker, falcon::TRAIN);
    linear_reg_builder.distributed_eval(*party, *worker, falcon::TEST);
  }
}

void launch_linear_reg_parameter_server(
    Party *party, const std::string &params_pb_str,
    const std::string &ps_network_config_pb_str,
    const std::string &model_save_file, const std::string &model_report_file) {
  log_info("PS Init linear regression model");
  LinearRegressionParams params;
  deserialize_lir_params(params, params_pb_str);
  log_linear_regression_params(params);

  double training_accuracy = 0.0, testing_accuracy = 0.0;
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  // ps split the train and test data, after splitting, the ps on all parties
  // have the train and test data, later ps will broadcast the split info to
  // workers
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  // in distributed train, fit bias should be handled here
  split_dataset(party, params.fit_bias, training_data, testing_data,
                training_labels, testing_labels, split_percentage);
  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();
  // init log_reg_model instance
  auto linear_reg_model_builder = new LinearRegressionBuilder(
      params, weight_size, training_data, testing_data, training_labels,
      testing_labels, training_accuracy, testing_accuracy);

  log_info(
      "[launch_linear_reg_parameter_server]: init linear regression model");
  auto ps = new LinearRegParameterServer(*linear_reg_model_builder, *party,
                                         ps_network_config_pb_str);
  log_info("[launch_linear_reg_parameter_server]: Init ps finished.");

  // here need to send the train/test data/labels to workers
  // also, need to send the phe keys to workers
  // accordingly, workers have to receive and deserialize these
  ps->broadcast_train_test_data(training_data, testing_data, training_labels,
                                testing_labels);
  log_info("[launch_linear_reg_parameter_server]: broadcast train test data "
           "finished.");

  ps->broadcast_phe_keys();
  log_info(
      "[launch_linear_reg_parameter_server]: broadcast phe keys finished.");

  // start to train the task in a distributed way
  ps->distributed_train();
  log_info("[launch_linear_reg_parameter_server]: distributed train finished.");

  // evaluate the model on the training and testing datasets
  //  ps->distributed_eval(falcon::TRAIN, model_report_file);
  ps->distributed_eval(falcon::TEST, model_report_file);

  // save the trained model
  ps->save_model(model_save_file);

  delete linear_reg_model_builder;
  delete ps;
}

// initializes the LogisticRegressionBuilder instance
// and run .train() .eval() methods, then save model
void train_logistic_regression(Party *party, const std::string &params_str,
                               const std::string &model_save_file,
                               const std::string &model_report_file,
                               int is_distributed_train, Worker *worker) {
  log_info("Run train_logistic_regression");
  log_info("is_distributed_train = " + std::to_string(is_distributed_train));

  LogisticRegressionParams params;
  deserialize_lr_params(params, params_str);
  log_logistic_regression_params(params);

  // split train test data for party and populate the vectors
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  // if not distributed train, then the party split the data
  // otherwise, the party/worker receive the data and phe keys from ps
  if (is_distributed_train == 0) {
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    split_dataset(party, params.fit_bias, training_data, testing_data,
                  training_labels, testing_labels, split_percentage);
  } else {
    // here should receive the train/test data/labels from ps
    std::string recv_training_data_str, recv_testing_data_str;
    std::string recv_training_labels_str, recv_testing_labels_str;
    worker->recv_long_message_from_ps(recv_training_data_str);
    worker->recv_long_message_from_ps(recv_testing_data_str);
    deserialize_double_matrix(training_data, recv_training_data_str);
    deserialize_double_matrix(testing_data, recv_testing_data_str);
    if (party->party_type == falcon::ACTIVE_PARTY) {
      worker->recv_long_message_from_ps(recv_training_labels_str);
      worker->recv_long_message_from_ps(recv_testing_labels_str);
      deserialize_double_array(training_labels, recv_training_labels_str);
      deserialize_double_array(testing_labels, recv_testing_labels_str);
    }
    // also, receive the phe keys from ps
    // and set these to the party
    std::string recv_phe_keys_str;
    log_info("begin to receive phe keys from ps ");
    worker->recv_long_message_from_ps(recv_phe_keys_str);
    log_info("received phe keys from ps: " + recv_phe_keys_str);
    party->load_phe_key_string(recv_phe_keys_str);
    // set the weight size and party feature num
    party->setter_feature_num(static_cast<int>(training_data[0].size()));
  }

  log_info("training_data.size() = " + std::to_string(training_data.size()));
  log_info("training_data[0].size() = " +
           std::to_string(training_data[0].size()));
  log_info("testing_data.size() = " + std::to_string(testing_data.size()));
  log_info("testing_data[0].size() = " +
           std::to_string(testing_data[0].size()));
  log_info("training_labels.size() = " +
           std::to_string(training_labels.size()));
  log_info("testing_labels.size() = " + std::to_string(testing_labels.size()));
  log_info("Init logistic regression model");

  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();
  double training_accuracy = 0.0, testing_accuracy = 0.0;
  LogisticRegressionBuilder log_reg_model_builder(
      params, weight_size, training_data, testing_data, training_labels,
      testing_labels, training_accuracy, testing_accuracy);

  if (is_distributed_train == 0) {
    log_reg_model_builder.train(*party);
    //    log_reg_model_builder.eval(*party, falcon::TRAIN, model_report_file);
    log_reg_model_builder.eval(*party, falcon::TEST, model_report_file);
    // save model and report
    auto *model_weights = new EncodedNumber[weight_size];
    std::string pb_lr_model_string;
    serialize_lr_model(log_reg_model_builder.log_reg_model, pb_lr_model_string);
    save_pb_model_string(pb_lr_model_string, model_save_file);
    // save_lr_model(log_reg_model_builder.log_reg_model, model_save_file);
    // save_training_report(log_reg_model.getter_training_accuracy(),
    //    log_reg_model.getter_testing_accuracy(),
    //    model_report_file);
    delete[] model_weights;
  } else {
    log_reg_model_builder.distributed_train(*party, *worker);
    // in is_distributed_train, parameter server will save the model.
    //    log_reg_model_builder.distributed_eval(*party, *worker,
    //    falcon::TRAIN);
    log_reg_model_builder.distributed_eval(*party, *worker, falcon::TEST);
  }
}

void launch_log_reg_parameter_server(
    Party *party, const std::string &params_pb_str,
    const std::string &ps_network_config_pb_str,
    const std::string &model_save_file, const std::string &model_report_file) {
  log_info("PS Init logistic regression model");
  LogisticRegressionParams params;
  deserialize_lr_params(params, params_pb_str);
  log_logistic_regression_params(params);

  double training_accuracy = 0.0, testing_accuracy = 0.0;
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  // ps split the train and test data, after splitting, the ps on all parties
  // have the train and test data, later ps will broadcast the split info to
  // workers
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  // in distributed train, fit bias should be handled here
  split_dataset(party, params.fit_bias, training_data, testing_data,
                training_labels, testing_labels, split_percentage);
  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();

  // init log_reg_model instance
  auto log_reg_model_builder = new LogisticRegressionBuilder(
      params, weight_size, training_data, testing_data, training_labels,
      testing_labels, training_accuracy, testing_accuracy);

  log_info("Init logistic regression model");
  log_info("[launch_log_reg_ps]: test party's content.");
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party->getter_phe_pub_key(phe_pub_key);
  log_info("[launch_log_reg_ps]: okay.");

  auto ps = new LogRegParameterServer(*log_reg_model_builder, *party,
                                      ps_network_config_pb_str);
  djcs_t_free_public_key(phe_pub_key);

  // here need to send the train/test data/labels to workers
  // also, need to send the phe keys to workers
  // accordingly, workers have to receive and deserialize these
  ps->broadcast_train_test_data(training_data, testing_data, training_labels,
                                testing_labels);
  ps->broadcast_phe_keys();

  // start to train the task in a distributed way
  ps->distributed_train();

  // evaluate the model on the training and testing datasets
  //  ps->distributed_eval(falcon::TRAIN, model_report_file);
  ps->distributed_eval(falcon::TEST, model_report_file);

  // save the trained model
  ps->save_model(model_save_file);

  delete log_reg_model_builder;
  delete ps;
}

void train_decision_tree(Party *party, const std::string &params_str,
                         const std::string &model_save_file,
                         const std::string &model_report_file,
                         int is_distributed_train, Worker *worker) {
  log_info("Run the decision tree train");
  DecisionTreeParams params;
  //  // currently for testing
  //  params.tree_type = "classification";
  //  params.criterion = "gini";
  //  params.split_strategy = "best";
  //  params.class_num = 2;
  //  params.max_depth = 5;
  //  params.max_bins = 8;
  //  params.min_samples_split = 5;
  //  params.min_samples_leaf = 5;
  //  params.max_leaf_nodes = 16;
  //  params.min_impurity_decrease = 0.01;
  //  params.min_impurity_split = 0.001;
  //  params.dp_budget = 0.1;
  deserialize_dt_params(params, params_str);
  log_decision_tree_params(params);

  double training_accuracy = 0.0, testing_accuracy = 0.0;
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  // record full train and test data for distributed training
  std::vector<std::vector<double>> full_training_data, full_testing_data;
  std::vector<double> full_training_labels, full_testing_labels;

  // if not distributed train, then the party split the data
  // otherwise, the party/worker receive the data and phe keys from ps
  if (is_distributed_train == 0) {
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    party->split_train_test_data(split_percentage, training_data, testing_data,
                                 training_labels, testing_labels);
  } else { // here should receive the train/test data/labels from ps
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    party->split_train_test_data(split_percentage, full_training_data,
                                 full_testing_data, full_training_labels,
                                 full_testing_labels);

    std::string recv_training_data_str, recv_testing_data_str;
    std::string recv_training_labels_str, recv_testing_labels_str;
    worker->recv_long_message_from_ps(recv_training_data_str);
    worker->recv_long_message_from_ps(recv_testing_data_str);
    deserialize_double_matrix(training_data, recv_training_data_str);
    deserialize_double_matrix(testing_data, recv_testing_data_str);

    std::string recv_worker_feature_index_prefix_train_data;
    std::string recv_worker_feature_index_prefix_test_data;
    worker->recv_long_message_from_ps(
        recv_worker_feature_index_prefix_train_data);
    worker->recv_long_message_from_ps(
        recv_worker_feature_index_prefix_test_data);

    // decode worker_feature_index_prefix_train_data
    std::stringstream iss(recv_worker_feature_index_prefix_train_data);
    int worker_feature_index_prefix_train_data_num;
    std::vector<int> worker_feature_index_prefix_train_data_vector;
    while (iss >> worker_feature_index_prefix_train_data_num)
      worker_feature_index_prefix_train_data_vector.push_back(
          worker_feature_index_prefix_train_data_num);

    // decode worker_feature_index_prefix_test_data
    std::stringstream iss2(recv_worker_feature_index_prefix_test_data);
    int worker_feature_index_prefix_test_data_num;
    std::vector<int> worker_feature_index_prefix_test_data_vector;
    while (iss2 >> worker_feature_index_prefix_test_data_num)
      worker_feature_index_prefix_test_data_vector.push_back(
          worker_feature_index_prefix_test_data_num);

    worker->assign_train_feature_prefix(
        worker_feature_index_prefix_train_data_vector[worker->worker_id - 1]);
    worker->assign_test_feature_prefix(
        worker_feature_index_prefix_test_data_vector[worker->worker_id - 1]);

    if (party->party_type == falcon::ACTIVE_PARTY) {
      worker->recv_long_message_from_ps(recv_training_labels_str);
      worker->recv_long_message_from_ps(recv_testing_labels_str);
      deserialize_double_array(training_labels, recv_training_labels_str);
      deserialize_double_array(testing_labels, recv_testing_labels_str);
    }

    // also, receive the phe keys from ps
    // and set these to the party
    std::string recv_phe_keys_str;
    log_info("begin to receive phe keys from ps ");

    worker->recv_long_message_from_ps(recv_phe_keys_str);
    log_info("received phe keys from ps: " + recv_phe_keys_str);
    party->load_phe_key_string(recv_phe_keys_str);
  }

  if (party->party_type == falcon::ACTIVE_PARTY) {
    log_info("[train_decision_tree] first data label = " +
             std::to_string(training_labels[0]));
    log_info("[train_decision_tree] second data label = " +
             std::to_string(training_labels[1]));
    log_info("[train_decision_tree] third data label = " +
             std::to_string(training_labels[2]));
  }

  log_info("Init decision tree model builder");
  DecisionTreeBuilder decision_tree_builder(
      params, training_data, testing_data, training_labels, testing_labels,
      training_accuracy, testing_accuracy);

  if (is_distributed_train == 0) {
    decision_tree_builder.train(*party);
    decision_tree_builder.eval(*party, falcon::TRAIN);
    decision_tree_builder.eval(*party, falcon::TEST);
    // save model and report
    // save_dt_model(decision_tree_builder.tree, model_save_file);
    std::string pb_dt_model_string;
    serialize_tree_model(decision_tree_builder.tree, pb_dt_model_string);
    save_pb_model_string(pb_dt_model_string, model_save_file);
    save_training_report(decision_tree_builder.getter_training_accuracy(),
                         decision_tree_builder.getter_testing_accuracy(),
                         model_report_file);
    TreeModel tree_model =
        decision_tree_builder.aggregate_decrypt_tree_model(*party);
    if (party->party_type == falcon::ACTIVE_PARTY) {
      tree_model.print_tree_model();
    }
    log_info("Trained model and report saved");
  } else {
    // on evaluation stage, each worker should have all features.
    auto decision_tree_builder_eval = new DecisionTreeBuilder(
        params, full_training_data, full_testing_data, full_training_labels,
        full_testing_labels, training_accuracy, testing_accuracy);

    decision_tree_builder.distributed_train(*party, *worker);
    // update the tree instance in decision_tree_builder_eval.
    log_info("Print the tree of the decision_tree_builder, which is the tree "
             "after training");
    decision_tree_builder.tree.print_tree_model();

    // copy tree
    decision_tree_builder_eval->tree.type = decision_tree_builder.tree.type;
    decision_tree_builder_eval->tree.class_num =
        decision_tree_builder.tree.class_num;
    decision_tree_builder_eval->tree.max_depth =
        decision_tree_builder.tree.max_depth;
    decision_tree_builder_eval->tree.internal_node_num =
        decision_tree_builder.tree.internal_node_num;
    decision_tree_builder_eval->tree.total_node_num =
        decision_tree_builder.tree.total_node_num;
    for (int j = 0; j < decision_tree_builder_eval->tree.capacity; j++) {
      decision_tree_builder_eval->tree.nodes[j] =
          decision_tree_builder.tree.nodes[j];
    }
    decision_tree_builder_eval->tree.capacity =
        decision_tree_builder.tree.capacity;

    log_info("Print the tree of the decision_tree_builder_eval, which is the "
             "tree used for evaluation");
    decision_tree_builder_eval->tree.print_tree_model();
    decision_tree_builder_eval->distributed_eval(*party, *worker,
                                                 falcon::TRAIN);
    decision_tree_builder_eval->distributed_eval(*party, *worker, falcon::TEST);
    // in is_distributed_train, parameter server will save the model.

    delete decision_tree_builder_eval;
  }
}

void launch_dt_parameter_server(Party *party, const std::string &params_str,
                                const std::string &ps_network_config_pb_str,
                                const std::string &model_save_file,
                                const std::string &model_report_file) {
  log_info("Run the example decision tree train");
  falcon_print(std::cout, "Run the example decision tree train");

  DecisionTreeParams params;
  //  // currently for testing
  //  params.tree_type = "classification";
  //  params.criterion = "gini";
  //  params.split_strategy = "best";
  //  params.class_num = 2;
  //  params.max_depth = 5;
  //  params.max_bins = 8;
  //  params.min_samples_split = 5;
  //  params.min_samples_leaf = 5;
  //  params.max_leaf_nodes = 16;
  //  params.min_impurity_decrease = 0.01;
  //  params.min_impurity_split = 0.001;
  //  params.dp_budget = 0.1;
  deserialize_dt_params(params, params_str);
  log_decision_tree_params(params);

  double training_accuracy = 0.0, testing_accuracy = 0.0;
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  log_info("[DTParameterServer]: split_train_test_data");
  party->split_train_test_data(split_percentage, training_data, testing_data,
                               training_labels, testing_labels);

  log_info("[DTParameterServer]: Init decision tree model");
  auto decision_tree_builder = new DecisionTreeBuilder(
      params, training_data, testing_data, training_labels, testing_labels,
      training_accuracy, testing_accuracy);

  falcon_print(std::cout, "train labels size are", training_labels.size());
  falcon_print(std::cout, "Init decision tree model finished");

  auto ps = new DTParameterServer(*decision_tree_builder, *party,
                                  ps_network_config_pb_str);

  // here need to send the train/test data/labels to workers
  // also, need to send the phe keys to workers
  // accordingly, workers have to receive and deserialize these
  log_info("[DTParameterServer]: begin to broadcast_train_test_data and keys");

  ps->broadcast_train_test_data(training_data, testing_data, training_labels,
                                testing_labels);
  ps->broadcast_phe_keys();

  log_info("[DTParameterServer]: start to train the task in a distributed way");
  ps->distributed_train();

  log_info("[DTParameterServer]: start to eval the task in a distributed way");
  // evaluate the model on the training and testing datasets
  log_info(
      "[Ps.distributed_eval]: ------ Evaluation on train dataset Start------ ");
  ps->distributed_eval(falcon::TRAIN, model_report_file);
  log_info(
      "[Ps.distributed_eval]: ------ Evaluation on test dataset Start ------ ");
  ps->distributed_eval(falcon::TEST, model_report_file);

  log_info("[DTParameterServer]: start to save model");
  ps->save_model(model_save_file);

  delete decision_tree_builder;
  delete ps;
}

void train_random_forest(Party party, const std::string &params_str,
                         const std::string &model_save_file,
                         const std::string &model_report_file) {
  log_info("Run the example random forest train");

  RandomForestParams params;
  //  // currently for testing
  //  params.n_estimator = 8;
  //  params.sample_rate = 0.8;
  //  params.dt_param.tree_type = "classification";
  //  params.dt_param.criterion = "gini";
  //  params.dt_param.split_strategy = "best";
  //  params.dt_param.class_num = 2;
  //  params.dt_param.max_depth = 3;
  //  params.dt_param.max_bins = 8;
  //  params.dt_param.min_samples_split = 5;
  //  params.dt_param.min_samples_leaf = 5;
  //  params.dt_param.max_leaf_nodes = 16;
  //  params.dt_param.min_impurity_decrease = 0.01;
  //  params.dt_param.min_impurity_split = 0.001;
  //  params.dt_param.dp_budget = 0.1;
  deserialize_rf_params(params, params_str);
  log_random_forest_params(params);

  double training_accuracy = 0.0, testing_accuracy = 0.0;
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  party.split_train_test_data(split_percentage, training_data, testing_data,
                              training_labels, testing_labels);

  log_info("Init random forest model builder");
  RandomForestBuilder random_forest_builder(
      params, training_data, testing_data, training_labels, testing_labels,
      training_accuracy, testing_accuracy);

  random_forest_builder.train(party);
  random_forest_builder.eval(party, falcon::TRAIN);
  random_forest_builder.eval(party, falcon::TEST);

  std::string pb_rf_model_string;
  serialize_random_forest_model(random_forest_builder.forest_model,
                                pb_rf_model_string);
  save_pb_model_string(pb_rf_model_string, model_save_file);
  save_training_report(random_forest_builder.getter_training_accuracy(),
                       random_forest_builder.getter_testing_accuracy(),
                       model_report_file);

  log_info("Trained model and report saved");
  google::FlushLogFiles(google::INFO);
}

void train_gbdt(Party party, const std::string &params_str,
                const std::string &model_save_file,
                const std::string &model_report_file) {
  log_info("Run the example gbdt model train");

  GbdtParams params;
  // currently for testing
  params.n_estimator = 8;
  params.learning_rate = 0.1;
  params.subsample = 1.0;
  params.loss = "ls";
  params.dt_param.tree_type = "regression";
  params.dt_param.criterion = "mse";
  params.dt_param.split_strategy = "best";
  params.dt_param.class_num = 2;
  params.dt_param.max_depth = 3;
  params.dt_param.max_bins = 8;
  params.dt_param.min_samples_split = 5;
  params.dt_param.min_samples_leaf = 5;
  params.dt_param.max_leaf_nodes = 64;
  params.dt_param.min_impurity_decrease = 0.01;
  params.dt_param.min_impurity_split = 0.001;
  params.dt_param.dp_budget = 0.1;
  //  deserialize_gbdt_params(params, params_str);
  log_gbdt_params(params);

  double training_accuracy = 0.0, testing_accuracy = 0.0;
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  party.split_train_test_data(split_percentage, training_data, testing_data,
                              training_labels, testing_labels);

  log_info("Init gbdt model builder");
  GbdtBuilder gbdt_builder(params, training_data, testing_data, training_labels,
                           testing_labels, training_accuracy, testing_accuracy);

  gbdt_builder.train(party);
  gbdt_builder.eval(party, falcon::TRAIN);
  gbdt_builder.eval(party, falcon::TEST);

  std::string pb_gbdt_model_string;
  serialize_gbdt_model(gbdt_builder.gbdt_model, pb_gbdt_model_string);
  save_pb_model_string(pb_gbdt_model_string, model_save_file);
  save_training_report(gbdt_builder.getter_training_accuracy(),
                       gbdt_builder.getter_testing_accuracy(),
                       model_report_file);

  log_info("Trained model and report saved");
  google::FlushLogFiles(google::INFO);
}

void train_mlp(Party *party, const std::string &params_str,
               const std::string &model_save_file,
               const std::string &model_report_file, int is_distributed_train,
               Worker *worker) {
  log_info("Run train_mlp");
  log_info("is_distributed_train = " + std::to_string(is_distributed_train));

  MlpParams params;
  deserialize_mlp_params(params, params_str);
  log_mlp_params(params);

  // split train test data for party and populate the vectors
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  // if not distributed train, then the party split the data
  // otherwise, the party/worker receive the data and phe keys from ps
  if (is_distributed_train == 0) {
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    // for mlp model, do not insert a bias term into the dataset
    split_dataset(party, false, training_data, testing_data, training_labels,
                  testing_labels, split_percentage);
  } else {
    // here should receive the train/test data/labels from ps
    std::string recv_training_data_str, recv_testing_data_str;
    std::string recv_training_labels_str, recv_testing_labels_str;
    worker->recv_long_message_from_ps(recv_training_data_str);
    worker->recv_long_message_from_ps(recv_testing_data_str);
    deserialize_double_matrix(training_data, recv_training_data_str);
    deserialize_double_matrix(testing_data, recv_testing_data_str);
    if (party->party_type == falcon::ACTIVE_PARTY) {
      worker->recv_long_message_from_ps(recv_training_labels_str);
      worker->recv_long_message_from_ps(recv_testing_labels_str);
      deserialize_double_array(training_labels, recv_training_labels_str);
      deserialize_double_array(testing_labels, recv_testing_labels_str);
    }
    // also, receive the phe keys from ps
    // and set these to the party
    std::string recv_phe_keys_str;
    log_info("begin to receive phe keys from ps ");
    worker->recv_long_message_from_ps(recv_phe_keys_str);
    log_info("received phe keys from ps: " + recv_phe_keys_str);
    party->load_phe_key_string(recv_phe_keys_str);
    // set the weight size and party feature num
    party->setter_feature_num(static_cast<int>(training_data[0].size()));
  }

  // weight size is different if fit_bias is true on active party
  double training_accuracy = 0.0, testing_accuracy = 0.0;
  MlpBuilder mlp_builder(params, training_data, testing_data, training_labels,
                         testing_labels, training_accuracy, testing_accuracy);

  log_mlp_params(params);

  if (is_distributed_train == 0) {
    mlp_builder.train(*party);
    //    mlp_builder.eval(*party, falcon::TRAIN, model_report_file);
    mlp_builder.eval(*party, falcon::TEST, model_report_file);
    //    // save model and report
    //    auto* model_weights = new EncodedNumber[weight_size];
    std::string pb_mlp_model_string;
    serialize_mlp_model(mlp_builder.mlp_model, pb_mlp_model_string);
    save_pb_model_string(pb_mlp_model_string, model_save_file);
    // save_training_report(log_reg_model.getter_training_accuracy(),
    //    log_reg_model.getter_testing_accuracy(),
    //    model_report_file);
    //    delete[] model_weights;
  } else {
    mlp_builder.distributed_train(*party, *worker);
    // in is_distributed_train, parameter server will save the model.
    //    mlp_builder.distributed_eval(*party, *worker, falcon::TRAIN);
    mlp_builder.distributed_eval(*party, *worker, falcon::TEST);
  }
}

void launch_mlp_parameter_server(Party *party, const std::string &params_pb_str,
                                 const std::string &ps_network_config_pb_str,
                                 const std::string &model_save_file,
                                 const std::string &model_report_file) {
  log_info("PS Init MLP model");
  MlpParams params;
  deserialize_mlp_params(params, params_pb_str);
  log_mlp_params(params);

  double training_accuracy = 0.0, testing_accuracy = 0.0;
  std::vector<std::vector<double>> training_data, testing_data;
  std::vector<double> training_labels, testing_labels;

  // ps split the train and test data, after splitting, the ps on all parties
  // have the train and test data, later ps will broadcast the split info to
  // workers
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  // in distributed train, fit bias should be handled here
  split_dataset(party, false, training_data, testing_data, training_labels,
                testing_labels, split_percentage);

  // init log_reg_model instance
  auto mlp_builder =
      new MlpBuilder(params, training_data, testing_data, training_labels,
                     testing_labels, training_accuracy, testing_accuracy);

  log_info("Init MLP model");
  log_info("[launch_mlp_parameter_server]: test party's content.");
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party->getter_phe_pub_key(phe_pub_key);
  log_info("[launch_mlp_parameter_server]: okay.");

  auto ps =
      new MlpParameterServer(*mlp_builder, *party, ps_network_config_pb_str);
  djcs_t_free_public_key(phe_pub_key);

  // here need to send the train/test data/labels to workers
  // also, need to send the phe keys to workers
  // accordingly, workers have to receive and deserialize these
  ps->broadcast_train_test_data(training_data, testing_data, training_labels,
                                testing_labels);
  ps->broadcast_phe_keys();

  // start to train the task in a distributed way
  ps->distributed_train();

  // evaluate the model on the training and testing datasets
  //  ps->distributed_eval(falcon::TRAIN, model_report_file);
  ps->distributed_eval(falcon::TEST, model_report_file);

  // save the trained model
  ps->save_model(model_save_file);

  log_info("[launch_mlp_parameter_server]: begin to destructor");

  delete mlp_builder;
  delete ps;
}
