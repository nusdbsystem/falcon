//
// Created by root on 11/17/21.
//

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/linear_regression_ps.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/logger/log_alg_params.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>
#include <chrono>
#include <stdexcept>
#include <random>

using namespace std;


LinearRegParameterServer::LinearRegParameterServer(
    const LinearRegParameterServer &obj) : alg_builder(obj.alg_builder){}

LinearRegParameterServer::LinearRegParameterServer(
    const LinearRegressionBuilder& m_alg_builder,
    const Party& m_party,
    const string &ps_network_config_pb_str):
    LinearParameterServer(m_party, ps_network_config_pb_str),
    alg_builder(m_alg_builder) {}

LinearRegParameterServer::LinearRegParameterServer(
    const Party &m_party,
    const std::string &ps_network_config_pb_str) :
    LinearParameterServer(m_party, ps_network_config_pb_str) {}

LinearRegParameterServer::~LinearRegParameterServer() = default;

void LinearRegParameterServer::distributed_train(){

  // step 1: init encrypted local weights
  // (here use 3 * precision for consistence in the following)
  int encrypted_weights_precision = 3 * PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;

  // create encrypted weight and store it to alg_builder 's local_weight
  init_encrypted_random_numbers(party, this->alg_builder.linear_reg_model.weight_size,
                                this->alg_builder.linear_reg_model.local_weights,
                                encrypted_weights_precision);

  // record training data ids in training_data_indexes for iteratively batch selection
  std::vector<int> training_data_indexes;
  for (int i = 0; i < this->alg_builder.getter_training_data().size(); i++) {
    training_data_indexes.push_back(i);
  }

  log_info("current channel size = " + std::to_string(this->worker_channels.size()));

  for (int iter = 0; iter < this->alg_builder.max_iteration; iter++) {
    log_info("--------PS Iteration " + std::to_string(iter) + " --------");
    // step 2: broadcast weight
    this->broadcast_encrypted_weights(this->alg_builder.linear_reg_model);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps broadcast_encrypted_weights successful --------");
    // step 3: randomly select a batch of samples from training data
    // active ps select batch idx and broadcast to passive ps
    std::vector<int> batch_indexes = this->select_batch_idx(training_data_indexes,
                                                            this->alg_builder.batch_size);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps select_batch_idx successful --------");
    // step 4: partition sample ids, since the partition method is deterministic
    // all ps can do this step in its own cluster and broadcast to its own workers
    this->partition_examples(batch_indexes);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps partition_examples successful --------");
    // step 5: wait worker finish execution
    auto encoded_message = this->wait_worker_complete();
    log_info("LRParameterServer received all loss * X_{ij} from falcon_worker");
    log_info("--------PS Iteration " + std::to_string(iter) + ", LRParameterServer received all loss * X_{ij} from falcon_worker successful --------");
    // step 6: receive loss, and update weight
    int weight_phe_precision = abs(this->alg_builder.linear_reg_model.local_weights[0].getter_exponent());
    this->update_encrypted_weights(encoded_message,
                                   this->alg_builder.linear_reg_model.weight_size,
                                   weight_phe_precision,
                                   this->alg_builder.linear_reg_model.local_weights);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps update_encrypted_weights successful --------");
  }
}

void LinearRegParameterServer::distributed_eval(
    falcon::DatasetType eval_type,
    const std::string& report_save_path) {

  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str + " Start *************");

  // step 1: init test data
  int dataset_size =
      (eval_type == falcon::TRAIN) ?
      (int) this->alg_builder.getter_training_data().size() :
      (int) this->alg_builder.getter_testing_data().size();

  std::vector<int> cur_test_data_indexes;
  cur_test_data_indexes.reserve(dataset_size);
  for (int i = 0; i < dataset_size; i++) {
    cur_test_data_indexes.push_back(i);
  }

  // step 2: do the prediction
  auto* decrypted_labels = new EncodedNumber[dataset_size];
  distributed_predict(cur_test_data_indexes, decrypted_labels);

  // step 3: compute and save matrix
  if (party.party_type == falcon::ACTIVE_PARTY) {
    this->alg_builder.eval_predictions_and_save(decrypted_labels,
                                                dataset_size,
                                                eval_type,
                                                report_save_path);
  }

  delete [] decrypted_labels;
}

void LinearRegParameterServer::save_model(const std::string& model_save_file){
  // auto* model_weights = new EncodedNumber[this->alg_builder.log_reg_model.weight_size];
  std::string pb_lr_model_string;
  serialize_lr_model(this->alg_builder.linear_reg_model, pb_lr_model_string);
  save_pb_model_string(pb_lr_model_string, model_save_file);
}


void launch_linear_reg_parameter_server(
    Party* party,
    const std::string& params_pb_str,
    const std::string& ps_network_config_pb_str,
    const std::string& model_save_file,
    const std::string& model_report_file) {

  LinearRegressionParams params;
  deserialize_lir_params(params, params_pb_str);
  log_info("deserialize lr params on ps success");
  log_info("PS Init logistic regression model");
  log_linear_regression_params(params);
  google::FlushLogFiles(google::INFO);

  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;

  std::vector< std::vector<double> > training_data;
  std::vector< std::vector<double> > testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;

  // ps split the train and test data, after splitting, the ps on all parties
  // have the train and test data, later ps will broadcast the split info to workers
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  // in distributed train, fit bias should be handled here
  split_dataset(party, params.fit_bias, training_data, testing_data,
                training_labels, testing_labels, split_percentage);
  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();

  // init log_reg_model instance
  auto linear_reg_model_builder = new LinearRegressionBuilder (
      params,
      weight_size,
      training_data,
      testing_data,
      training_labels,
      testing_labels,
      training_accuracy,
      testing_accuracy);

  log_info("Init logistic regression model");

  auto ps = new LinearRegParameterServer(*linear_reg_model_builder, *party, ps_network_config_pb_str);

  // here need to send the train/test data/labels to workers
  // also, need to send the phe keys to workers
  // accordingly, workers have to receive and deserialize these
  ps->broadcast_train_test_data(training_data, testing_data, training_labels, testing_labels);
  ps->broadcast_phe_keys();

  // start to train the task in a distributed way
  ps->distributed_train();

  // evaluate the model on the training and testing datasets
  ps->distributed_eval(falcon::TRAIN, model_report_file);
  ps->distributed_eval(falcon::TEST, model_report_file);

  // save the trained model
  ps->save_model(model_save_file);

  delete linear_reg_model_builder;
  delete ps;
}
