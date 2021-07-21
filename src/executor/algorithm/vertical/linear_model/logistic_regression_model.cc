//
// Created by wuyuncheng on 7/7/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>

#include <cmath>
#include <glog/logging.h>
#include <iostream>
#include <stack>
#include <Networking/ssl_sockets.h>

LogisticRegressionModel::LogisticRegressionModel() {}

LogisticRegressionModel::LogisticRegressionModel(int m_weight_size) {
  weight_size = m_weight_size;
  local_weights = new EncodedNumber[weight_size];
}

LogisticRegressionModel::~LogisticRegressionModel() {
  delete [] local_weights;
}

LogisticRegressionModel::LogisticRegressionModel(const LogisticRegressionModel& log_reg_model) {
  weight_size = log_reg_model.weight_size;
  local_weights = new EncodedNumber[weight_size];
  for (int i = 0; i < weight_size; i++) {
    local_weights[i] = log_reg_model.local_weights[i];
  }
}

LogisticRegressionModel& LogisticRegressionModel::operator=(const LogisticRegressionModel &log_reg_model) {
  weight_size = log_reg_model.weight_size;
  local_weights = new EncodedNumber[weight_size];
  for (int i = 0; i < weight_size; i++) {
    local_weights[i] = log_reg_model.local_weights[i];
  }
}

void LogisticRegressionModel::predict(Party &party,
    std::vector<std::vector<double> > predicted_samples,
    int predicted_sample_size,
    EncodedNumber *predicted_labels) {
  /// the prediction workflow is as follows:
  ///     step 1: every party computes partial phe summation and sends to active party
  ///     step 2: active party aggregates and send the predicted_labels to other parties
  ///     step 3: convert to secret shares and send to mpc parties to compute logistic function
  ///     step 4: convert the secret shared results back to ciphertexts

  // local compute aggregation and receive from passive parties
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // retrieve batch samples and encode (notice to use cur_batch_size
  // instead of default batch size to avoid unexpected batch)
  EncodedNumber* batch_phe_aggregation = new EncodedNumber[predicted_sample_size];
  EncodedNumber** encoded_batch_samples = new EncodedNumber*[predicted_sample_size];
  for (int i = 0; i < predicted_sample_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[weight_size];
  }
  for (int i = 0; i < predicted_sample_size; i++) {
    for (int j = 0; j < weight_size; j++) {
      encoded_batch_samples[i][j].set_double(phe_pub_key->n[0],
          predicted_samples[i][j], PHE_FIXED_POINT_PRECISION);
    }
  }

  // compute local homomorphic aggregation
  EncodedNumber* local_batch_phe_aggregation = new EncodedNumber[predicted_sample_size];
  djcs_t_aux_matrix_mult(phe_pub_key, party.phe_random, local_batch_phe_aggregation,
      local_weights, encoded_batch_samples, predicted_sample_size, weight_size);

  // every party sends the local aggregation to the active party
  // copy self local aggregation results
  for (int i = 0; i < predicted_sample_size; i++) {
    batch_phe_aggregation[i] = local_batch_phe_aggregation[i];
  }

  std::cout << "Local phe aggregation finished" << std::endl;
  LOG(INFO) << "Local phe aggregation finished";

  // receive serialized string from other parties
  // deserialize and sum to batch_phe_aggregation
  if (party.party_id == ACTIVE_PARTY_ID) {
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        EncodedNumber* recv_batch_phe_aggregation = new EncodedNumber[predicted_sample_size];
        std::string recv_local_aggregation_str;
        party.recv_long_message(id, recv_local_aggregation_str);
        deserialize_encoded_number_array(
            recv_batch_phe_aggregation,
            predicted_sample_size,
            recv_local_aggregation_str);
        // homomorphic addition of the received aggregations
        for (int i = 0; i < predicted_sample_size; i++) {
          djcs_t_aux_ee_add(phe_pub_key,batch_phe_aggregation[i],
                            batch_phe_aggregation[i], recv_batch_phe_aggregation[i]);
        }
        delete [] recv_batch_phe_aggregation;
      }
    }
    std::cout << "Global phe aggregation finished" << std::endl;
    LOG(INFO) << "Global phe aggregation finished";
  } else {
    // serialize local batch aggregation and send to active party
    std::string local_aggregation_str;
    serialize_encoded_number_array(
        local_batch_phe_aggregation,
        predicted_sample_size,
        local_aggregation_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);
  }

  // serialize and send the batch_phe_aggregation to other parties
  std::string global_aggregation_str;
  if (party.party_id == ACTIVE_PARTY_ID) {
    serialize_encoded_number_array(
        batch_phe_aggregation,
        predicted_sample_size,
        global_aggregation_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, global_aggregation_str);
      }
    }
    std::cout << "Broad global phe aggregation result" << std::endl;
    LOG(INFO) << "Broad global phe aggregation result";
  } else {
    party.recv_long_message(ACTIVE_PARTY_ID, global_aggregation_str);
    deserialize_encoded_number_array(
        batch_phe_aggregation,
        predicted_sample_size,
        global_aggregation_str);
  }

  // convert the batch_phe_aggregation to secret shares
  int ciphertext_precision = 0 - local_weights[0].getter_exponent();
  int plaintext_precision = 0 - encoded_batch_samples[0][0].getter_exponent();
  int batch_aggregation_precision = ciphertext_precision + plaintext_precision;
  std::vector<double> batch_aggregation_shares;
  party.ciphers_to_secret_shares(batch_phe_aggregation,
                                 batch_aggregation_shares,
                                 predicted_sample_size,
                                 ACTIVE_PARTY_ID,
                                 batch_aggregation_precision);

  // communicate with spdz parties and receive results
  // the spdz_logistic_function_computation will do the 1/(1+e^(wx)) operation
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  std::thread spdz_thread(spdz_logistic_function_computation,
                          party.party_num,
                          party.party_id,
                          SPDZ_PORT_BASE,
                          SPDZ_PLAYER_PATH,
                          party.host_names,
                          batch_aggregation_shares,
                          predicted_sample_size,
                          &promise_values);
  std::vector<double> batch_logistic_shares = future_values.get();
  // main thread wait spdz_thread to finish
  spdz_thread.join();

  // convert the secret shares to ciphertext, which is the predicted labels
  party.secret_shares_to_ciphers(predicted_labels,
      batch_logistic_shares,
      predicted_sample_size,
      ACTIVE_PARTY_ID,
      batch_aggregation_precision);

  djcs_t_free_public_key(phe_pub_key);
  for (int i = 0; i < predicted_sample_size; i++) {
    delete [] encoded_batch_samples[i];
  }
  delete [] encoded_batch_samples;
  delete [] local_batch_phe_aggregation;
  delete [] batch_phe_aggregation;
}

void spdz_logistic_function_computation(int party_num,
    int party_id,
    int mpc_port_base,
    std::string mpc_player_path,
    std::vector<std::string> party_host_names,
    std::vector<double> batch_aggregation_shares,
    int cur_batch_size,
    std::promise<std::vector<double>> *batch_loss_shares) {
  // Here put the whole setup socket code together, as using a function call
  // would result in a problem when deleting the created sockets
  // setup connections from this party to each spdz party socket
  std::vector<ssl_socket*> mpc_sockets(party_num);
  vector<int> plain_sockets(party_num);
  // ssl_ctx ctx(mpc_player_path, "C" + to_string(party_id));
  ssl_ctx ctx("C" + to_string(party_id));
  // std::cout << "correct init ctx" << std::endl;
  ssl_service io_service;
  octetStream specification;
  std::cout << "begin connect to spdz parties" << std::endl;
  std::cout << "party_num = " << party_num << std::endl;
  for (int i = 0; i < party_num; i++)
  {
    set_up_client_socket(plain_sockets[i], party_host_names[i].c_str(), mpc_port_base + i);
    send(plain_sockets[i], (octet*) &party_id, sizeof(int));
    mpc_sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                    "P" + to_string(i), "C" + to_string(party_id), true);
    if (i == 0){
      // receive gfp prime
      specification.Receive(mpc_sockets[0]);
    }
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                                                          " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_port_base + i << ".";
  }
  LOG(INFO) << "Finish setup socket connections to spdz engines.";
  std::cout << "Finish setup socket connections to spdz engines." << std::endl;
  int type = specification.get<int>();
  switch (type)
  {
    case 'p':
    {
      gfp::init_field(specification.get<bigint>());
      LOG(INFO) << "Using prime " << gfp::pr();
      break;
    }
    default:
      LOG(ERROR) << "Type " << type << " not implemented";
      exit(1);
  }
  LOG(INFO) << "Finish initializing gfp field.";
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size() << std::endl;
  google::FlushLogFiles(google::INFO);

  // send data to spdz parties
  if (party_id == ACTIVE_PARTY_ID) {
    // send the current array size to the mpc parties
    std::vector<int> array_size_vec;
    array_size_vec.push_back(cur_batch_size);
    send_public_values(array_size_vec, mpc_sockets, party_num);
  }

  // all the parties send private shares
  for (int i = 0; i < batch_aggregation_shares.size(); i++) {
    vector<double> x;
    x.push_back(batch_aggregation_shares[i]);
    send_private_inputs(x, mpc_sockets, party_num);
  }
  // send_private_inputs(batch_aggregation_shares,mpc_sockets, party_num);
  std::vector<double> return_values = receive_result(mpc_sockets, party_num, cur_batch_size);
  batch_loss_shares->set_value(return_values);

  for (int i = 0; i < party_num; i++) {
    close_client_socket(plain_sockets[i]);
  }

  // free memory and close mpc_sockets
  for (int i = 0; i < party_num; i++) {
    delete mpc_sockets[i];
    mpc_sockets[i] = nullptr;
  }
}