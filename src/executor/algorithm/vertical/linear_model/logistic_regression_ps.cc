//
// Created on 03/07/21.
//

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/logistic_regression_ps.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
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


LogRegParameterServer::LogRegParameterServer(
    const LogRegParameterServer &obj) : alg_builder(obj.alg_builder){}

LogRegParameterServer::LogRegParameterServer(
        const LogisticRegressionBuilder& m_alg_builder,
        const Party& m_party,
        const string &ps_network_config_pb_str):
        LinearParameterServer(m_party, ps_network_config_pb_str),
        alg_builder(m_alg_builder) {
  log_info("[LogRegParameterServer::LogRegParameterServer]: constructor.");
}

LogRegParameterServer::LogRegParameterServer(
    const Party &m_party,
    const std::string &ps_network_config_pb_str) :
    LinearParameterServer(m_party, ps_network_config_pb_str) {}

LogRegParameterServer::~LogRegParameterServer() = default;

void LogRegParameterServer::distributed_train(){

  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);

  // step 1: init encrypted local weights
  // (here use precision for consistence in the following)
  int encrypted_weights_precision = PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;

  // create encrypted weight and store it to alg_builder 's local_weight
  this->alg_builder.log_reg_model.sync_up_weight_sizes(party);
  this->alg_builder.init_encrypted_weights(this->party, encrypted_weights_precision);

  // record training data ids in training_data_indexes for iteratively batch selection
  std::vector<int> training_data_indexes;
  for (int i = 0; i < this->alg_builder.getter_training_data().size(); i++) {
    training_data_indexes.push_back(i);
  }
  std::vector<std::vector<int>> batch_iter_indexes = precompute_iter_batch_idx(this->alg_builder.batch_size,
                                                                               this->alg_builder.max_iteration,
                                                                               training_data_indexes);

  log_info("current channel size = " + std::to_string(this->worker_channels.size()));

  for (int iter = 0; iter < this->alg_builder.max_iteration; iter++) {
    log_info("--------PS Iteration " + std::to_string(iter) + " --------");
    // step 2: broadcast weight
    this->broadcast_encrypted_weights(this->alg_builder.log_reg_model);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps broadcast_encrypted_weights successful --------");
    // step 3: randomly select a batch of samples from training data
    // active ps select batch idx and broadcast to passive ps
    std::vector<int> batch_indexes = sync_batch_idx(this->party,
                                                    this->alg_builder.batch_size,
                                                    batch_iter_indexes[iter]);
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
    int weight_phe_precision = abs(this->alg_builder.log_reg_model.local_weights[0].getter_exponent());
    this->update_encrypted_weights(encoded_message,
                                   this->alg_builder.log_reg_model.weight_size,
                                   weight_phe_precision,
                                   this->alg_builder.log_reg_model.local_weights);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps update_encrypted_weights successful --------");
    // check if the precision exceed the max precision and truncate
    if (std::abs(this->alg_builder.log_reg_model.local_weights[0].getter_exponent()) >= PHE_MAXIMUM_PRECISION) {
      this->alg_builder.log_reg_model.truncate_weights_precision(party, PHE_FIXED_POINT_PRECISION);
      log_info("--------PS Iteration " + std::to_string(iter) + ", ps truncate local weights successful --------");
    }
  }

  struct timespec finish;
  clock_gettime(CLOCK_MONOTONIC, &finish);

  double consumed_time = (double) (finish.tv_sec - start.tv_sec);
  consumed_time += (double) (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

  log_info("Training time = " + std::to_string(consumed_time));
}

void LogRegParameterServer::distributed_eval(
    falcon::DatasetType eval_type,
    const std::string& report_save_path) {

  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);

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
    this->alg_builder.eval_matrix_computation_and_save(decrypted_labels,
                                                     dataset_size,
                                                     eval_type,
                                                     report_save_path);
  }

  struct timespec finish;
  clock_gettime(CLOCK_MONOTONIC, &finish);

  double consumed_time = (double) (finish.tv_sec - start.tv_sec);
  consumed_time += (double) (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

  log_info("Evaluation time = " + std::to_string(consumed_time));

  delete [] decrypted_labels;
}

void LogRegParameterServer::save_model(const std::string& model_save_file){
  // auto* model_weights = new EncodedNumber[this->alg_builder.log_reg_model.weight_size];
  std::string pb_lr_model_string;
  serialize_lr_model(this->alg_builder.log_reg_model, pb_lr_model_string);
  save_pb_model_string(pb_lr_model_string, model_save_file);
}



