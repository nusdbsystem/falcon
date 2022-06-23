//
// Created by root on 5/26/22.
//

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/nn/mlp_ps.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/nn_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/logger/log_alg_params.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/pb_converter/nn_converter.h>
#include <falcon/utils/math/math_ops.h>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>
#include <chrono>
#include <stdexcept>
#include <random>

MlpParameterServer::MlpParameterServer(
    const MlpParameterServer &obj) : party(obj.party) {}

MlpParameterServer::MlpParameterServer(const MlpBuilder &m_mlp_builder,
                                       const Party &m_party,
                                       const std::string &ps_network_config_pb_str) :
                                       ParameterServer(ps_network_config_pb_str),
                                       party(m_party),
                                       mlp_builder(m_mlp_builder) {
  log_info("[MlpParameterServer]: constructor");
}

MlpParameterServer::MlpParameterServer(
    const Party &m_party, const std::string &ps_network_config_pb_str) :
    ParameterServer(ps_network_config_pb_str), party(m_party) {
      log_info("[MlpParameterServer::MlpParameterServer]: constructor.");
}

void MlpParameterServer::broadcast_train_test_data(const std::vector<std::vector<double>> &training_data,
                                                   const std::vector<std::vector<double>> &testing_data,
                                                   const std::vector<double> &training_labels,
                                                   const std::vector<double> &testing_labels) {
  // every ps sends the training data and testing data to workers
  std::string training_data_str, testing_data_str, training_labels_str, testing_labels_str;
  serialize_double_matrix(training_data, training_data_str);
  serialize_double_matrix(testing_data, testing_data_str);
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
    this->send_long_message_to_worker(wk_index, training_data_str);
    this->send_long_message_to_worker(wk_index, testing_data_str);
  }
  // only active ps sends the training labels and testing labels to workers
  if (party.party_type == falcon::ACTIVE_PARTY) {
    serialize_double_array(training_labels, training_labels_str);
    serialize_double_array(testing_labels, testing_labels_str);
    for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
      this->send_long_message_to_worker(wk_index, training_labels_str);
      this->send_long_message_to_worker(wk_index, testing_labels_str);
    }
  }
}

void MlpParameterServer::broadcast_phe_keys() {
  std::string phe_keys_str = party.export_phe_key_string();
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
    this->send_long_message_to_worker(wk_index, phe_keys_str);
  }
}

void MlpParameterServer::broadcast_encrypted_weights(const MlpModel &mlp_model) {
  std::string mlp_model_str;
  serialize_mlp_model(mlp_model, mlp_model_str);

  for(int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++){
    this->send_long_message_to_worker(wk_index, mlp_model_str);
  }
}

std::vector<int> MlpParameterServer::partition_examples(std::vector<int> batch_indexes) {
  int mini_batch_size = int(batch_indexes.size()/this->worker_channels.size());
  log_info("ps worker size = " + std::to_string(this->worker_channels.size()));
  log_info("mini batch size = " + std::to_string(mini_batch_size));
  std::vector<int> message_sizes;
  // deterministic partition given the batch indexes
  int index = 0;
  for(int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++){
    // generate mini-batch for this worker
    std::vector<int>::const_iterator first1 = batch_indexes.begin() + index;
    std::vector<int>::const_iterator last1  = batch_indexes.begin() + index + mini_batch_size;

    if (wk_index == this->worker_channels.size() - 1) {
      last1  = batch_indexes.end();
    }
    std::vector<int> mini_batch_indexes(first1, last1);
    // serialize mini_batch_indexes to str
    std::string mini_batch_indexes_str;
    serialize_int_array(mini_batch_indexes, mini_batch_indexes_str);
    // record size of mini_batch_indexes, used in deserialization process
    message_sizes.push_back((int) mini_batch_indexes.size());
    // send to worker
    this->send_long_message_to_worker(wk_index, mini_batch_indexes_str);
    // update index
    index += mini_batch_size;
  }

  for (int i = 0; i < message_sizes.size(); i++) {
    log_info("message_sizes[" + std::to_string(i) + "] = " + std::to_string(message_sizes[i]));
  }
  log_info("Broadcast client's batch requests to other workers");

  return message_sizes;
}

std::vector<string> MlpParameterServer::wait_worker_complete() {
  std::vector<string> encoded_messages;
  // wait until all received str has been stored to encrypted_gradient_strs
  while (this->worker_channels.size() != encoded_messages.size()){
    for(int wk_index=0; wk_index< this->worker_channels.size(); wk_index++){
      std::string message;
      this->recv_long_message_from_worker(wk_index, message);
      encoded_messages.push_back(message);
    }
  }
  return encoded_messages;
}

void MlpParameterServer::distributed_train() {
  // step 1: init encrypted weights (here use precision for consistence in the following)
  int n_features = party.getter_feature_num();
  std::vector<int> sync_arr = sync_up_int_arr(party, n_features);
  int encry_weights_prec = PHE_FIXED_POINT_PRECISION;
  int plain_samples_prec = PHE_FIXED_POINT_PRECISION;
  mlp_builder.init_encrypted_weights(party, encry_weights_prec);
  log_info("[train] init encrypted MLP weights success");

  // record training data ids in data_indexes for iteratively batch selection
  std::vector<int> train_data_indexes;
  for (int i = 0; i < this->mlp_builder.getter_training_data().size(); i++) {
    train_data_indexes.push_back(i);
  }
  std::vector<std::vector<int>> batch_iter_indexes = precompute_iter_batch_idx(
      this->mlp_builder.batch_size, this->mlp_builder.max_iteration, train_data_indexes);

  log_info("current channel size = " + std::to_string(this->worker_channels.size()));
  for (int iter = 0; iter < this->mlp_builder.max_iteration; iter++) {
    log_info("--------PS Iteration " + std::to_string(iter) + " --------");
    log_info("[distributed_train] display m_weight_mat");
    display_encrypted_matrix(party,
                             this->mlp_builder.mlp_model.m_layers[0].m_num_inputs,
                             this->mlp_builder.mlp_model.m_layers[0].m_num_outputs,
                             this->mlp_builder.mlp_model.m_layers[0].m_weight_mat);
    log_info("[distributed_train] display m_bias");
    display_encrypted_vector(party, this->mlp_builder.mlp_model.m_layers[0].m_num_outputs, this->mlp_builder.mlp_model.m_layers[0].m_bias);

    // step 2: broadcast weight
    this->broadcast_encrypted_weights(this->mlp_builder.mlp_model);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps broadcast_encrypted_weights successful --------");
    // step 3: randomly select a batch of samples from training data
    // active ps select batch idx and broadcast to passive ps
    std::vector<int> batch_indexes = sync_batch_idx(this->party,
                                                    this->mlp_builder.batch_size,
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
    int weight_phe_precision = abs(this->mlp_builder.mlp_model.m_layers[0].m_bias[0].getter_exponent());
    this->update_encrypted_weights(encoded_message);
    log_info("--------PS Iteration " + std::to_string(iter) + ", ps update_encrypted_weights successful --------");
  }
}

void MlpParameterServer::distributed_predict(const std::vector<int> &cur_test_data_indexes,
                                             EncodedNumber *predicted_labels) {
  log_info("current channel size = " + std::to_string(this->worker_channels.size()));
  // step 1: partition sample ids, every ps partition in the same way
  std::vector<int> message_sizes = this->partition_examples(cur_test_data_indexes);
  log_info("cur_test_data_indexes.size = " + std::to_string(cur_test_data_indexes.size()));
  for (int i = 0; i < message_sizes.size(); i++) {
    log_info("message_sizes[" + std::to_string(i) + "] = " + std::to_string(message_sizes[i]));
  }
  // step 2: if active party, wait worker finish execution
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::vector< string > encoded_messages = this->wait_worker_complete();
    int cur_index = 0;
    // deserialize encrypted predicted labels
    for (int i = 0; i < encoded_messages.size(); i++){
      auto partial_predicted_labels = new EncodedNumber[message_sizes[i]];
      deserialize_encoded_number_array(
          partial_predicted_labels,
          message_sizes[i],
          encoded_messages[i]);
      for (int k = 0; k < message_sizes[i]; k++){
        predicted_labels[cur_index] = partial_predicted_labels[k];
        cur_index += 1;
      }
      delete[] partial_predicted_labels;
    }
  }
}

void MlpParameterServer::distributed_eval(falcon::DatasetType eval_type, const std::string &report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str + " Start *************");

  // step 1: init test data
  int dataset_size =
      (eval_type == falcon::TRAIN) ?
      (int) this->mlp_builder.getter_training_data().size() :
      (int) this->mlp_builder.getter_testing_data().size();

  std::vector<std::vector<double>> cur_test_dataset =
      (eval_type == falcon::TRAIN) ? this->mlp_builder.getter_training_data()
      : this->mlp_builder.getter_testing_data();
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? this->mlp_builder.getter_training_labels()
      : this->mlp_builder.getter_testing_labels();

  std::vector<int> cur_test_data_indexes;
  cur_test_data_indexes.reserve(dataset_size);
  for (int i = 0; i < dataset_size; i++) {
    cur_test_data_indexes.push_back(i);
  }

  // step 2: do the prediction
  auto* decrypted_labels = new EncodedNumber[dataset_size];
  distributed_predict(cur_test_data_indexes, decrypted_labels);

  // step 4: active party computes the metrics
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
    if (this->mlp_builder.is_classification) {
      int correct_num = 0;
      for (int i = 0; i < dataset_size; i++) {
        log_info("[mlp_builder.eval] predictions[" + std::to_string(i) + "] = "
                     + std::to_string(predictions[i]) + ", cur_test_dataset_labels["
                     + std::to_string(i) + "] = " + std::to_string(cur_test_dataset_labels[i]));
        if (predictions[i] == cur_test_dataset_labels[i]) {
          correct_num += 1;
        }
      }
      if (eval_type == falcon::TRAIN) {
        this->mlp_builder.setter_training_accuracy((double) correct_num / dataset_size);
        log_info("[mlp_builder.eval] Dataset size = " + std::to_string(dataset_size)
                     + ", correct predicted num = " + std::to_string(correct_num)
                     + ", training accuracy = " + std::to_string(this->mlp_builder.getter_training_accuracy()));
      }
      if (eval_type == falcon::TEST) {
        this->mlp_builder.setter_testing_accuracy((double) correct_num / dataset_size);
        log_info("[mlp_builder.eval] Dataset size = " + std::to_string(dataset_size)
                     + ", correct predicted num = " + std::to_string(correct_num)
                     + ", testing accuracy = " + std::to_string(this->mlp_builder.getter_testing_accuracy()));
      }
    } else {
      if (eval_type == falcon::TRAIN) {
        this->mlp_builder.setter_training_accuracy(mean_squared_error(predictions, cur_test_dataset_labels));
        log_info("[mlp_builder.eval] Training accuracy = " + std::to_string(this->mlp_builder.getter_training_accuracy()));
      }
      if (eval_type == falcon::TEST) {
        this->mlp_builder.setter_testing_accuracy(mean_squared_error(predictions, cur_test_dataset_labels));
        log_info("[mlp_builder.eval] Testing accuracy = " + std::to_string(this->mlp_builder.getter_testing_accuracy()));
      }
    }
  }

  delete [] decrypted_labels;
}

void MlpParameterServer::update_encrypted_weights(const std::vector<string> &encoded_messages) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int idx = 0;
  // deserialize encrypted message, and add to encrypted
  for (const std::string& message: encoded_messages){
    log_info("[MlpParameterServer::update_encrypted_weights] begin to deserialize encoded_message " + std::to_string(idx));
    MlpModel dec_mlp_model;
    deserialize_mlp_model(dec_mlp_model, message);
    log_info("[MlpParameterServer::update_encrypted_weights] after deserialization");
    if (idx == 0) {
      // assign dec_mlp_model to this->mlp_builder.mlp_model
      for (int i =0; i < this->mlp_builder.mlp_model.m_layers.size(); i++) {
        log_info("[MlpParameterServer::update_encrypted_weights] enter layer " + std::to_string(i));
        int num_inputs = this->mlp_builder.mlp_model.m_layers[i].m_num_inputs;
        int num_outputs = this->mlp_builder.mlp_model.m_layers[i].m_num_outputs;
        for (int j = 0; j < num_inputs; j++) {
          for (int k = 0; k < num_outputs; k++) {
            this->mlp_builder.mlp_model.m_layers[i].m_weight_mat[j][k] = dec_mlp_model.m_layers[i].m_weight_mat[j][k];
          }
        }
        for (int k = 0; k < num_outputs; k++) {
          this->mlp_builder.mlp_model.m_layers[i].m_bias[k] = dec_mlp_model.m_layers[i].m_bias[k];
        }
        log_info("[MlpParameterServer::update_encrypted_weights] exit layer " + std::to_string(i));
      }
    } else {
      // aggregate the dec_mlp_model into agg_mlp_model
      for (int i = 0; i < this->mlp_builder.mlp_model.m_layers.size(); i++) {
        log_info("[MlpParameterServer::update_encrypted_weights] enter layer " + std::to_string(i));

        // aggregate layer's m_weight_mat
        djcs_t_aux_matrix_ele_wise_ee_add_ext(
            phe_pub_key,
            this->mlp_builder.mlp_model.m_layers[i].m_weight_mat,
            this->mlp_builder.mlp_model.m_layers[i].m_weight_mat,
            dec_mlp_model.m_layers[i].m_weight_mat,
            this->mlp_builder.mlp_model.m_layers[i].m_num_inputs,
            this->mlp_builder.mlp_model.m_layers[i].m_num_outputs);
        // aggregate layer's m_bias
        djcs_t_aux_vec_ele_wise_ee_add_ext(
            phe_pub_key,
            this->mlp_builder.mlp_model.m_layers[i].m_bias,
            this->mlp_builder.mlp_model.m_layers[i].m_bias,
            dec_mlp_model.m_layers[i].m_bias,
            this->mlp_builder.mlp_model.m_layers[i].m_num_outputs);

        log_info("[MlpParameterServer::update_encrypted_weights] exit layer " + std::to_string(i));
      }
    }
    log_info("[MlpParameterServer::update_encrypted_weights] free the object assigned to encoded_message " + std::to_string(idx));
    idx += 1;
  }

  djcs_t_free_public_key(phe_pub_key);
}

void MlpParameterServer::save_model(const std::string &model_save_file) {
  std::string  pb_mlp_model_str;
  serialize_mlp_model(this->mlp_builder.mlp_model, pb_mlp_model_str);
  save_pb_model_string(pb_mlp_model_str, model_save_file);
}