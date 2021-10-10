//
// Created by wuyuncheng on 7/7/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>

#include <cmath>
#include <glog/logging.h>
#include <stack>
#include <Networking/ssl_sockets.h>
#include <iostream>


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

void LogisticRegressionModel::predict(
    const Party &party,
    const std::vector<std::vector<double> >& predicted_samples,
    EncodedNumber *predicted_labels) const {

  /// the prediction workflow is as follows:
  ///     step 1: every party computes partial phe summation and sends to active party
  ///     step 2: active party aggregates and send the predicted_labels to other parties
  ///     step 3: convert to secret shares and send to mpc parties to compute logistic function
  ///     step 4: convert the secret shared results back to ciphertexts

  EncodedNumber** encoded_batch_samples;
  int cur_sample_size = predicted_samples.size();
  encoded_batch_samples = new EncodedNumber*[cur_sample_size];
  for (int i = 0; i < cur_sample_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[cur_sample_size];
  }
  encode_samples( party, predicted_samples, encoded_batch_samples);

  int ciphertext_precision = 0 - local_weights[0].getter_exponent();
  int plaintext_precision = 0 - encoded_batch_samples[0][0].getter_exponent();
  int encrypted_batch_aggregation_precision = ciphertext_precision + plaintext_precision;

  forward_computation(party,
      predicted_samples.size(),
      encoded_batch_samples,
      encrypted_batch_aggregation_precision,
      predicted_labels);

  int cur_batch_size = predicted_samples.size();
  for (int i = 0; i < cur_batch_size; i++) {
    delete[] encoded_batch_samples[i];
  }
  delete[] encoded_batch_samples;

}

void LogisticRegressionModel::forward_computation(
    const Party& party,
    int cur_batch_size,
    EncodedNumber** encoded_batch_samples,
    int& encrypted_batch_aggregation_precision,
    EncodedNumber* predicted_labels) const{

  // step 2.1: homomorphic batch aggregation
  // every party computes partial phe summation and sends to active party
  auto encrypted_batch_aggregation = new EncodedNumber[cur_batch_size];

  LOG(INFO) << "[forward_computation]: mpc_port_array_size after: " << party.executor_mpc_ports.size() <<" --------";
  std::cout << "[forward_computation]: mpc_port_array_size after: " << party.executor_mpc_ports.size() << std::endl;

  compute_batch_phe_aggregation(party,
      cur_batch_size,
      encoded_batch_samples,
      PHE_FIXED_POINT_PRECISION,
      encrypted_batch_aggregation);

  LOG(INFO) << "[forward_computation]: batch phe aggregation success" << " --------";
  std::cout << "[forward_computation]: batch phe aggregation success" << std::endl;

  // std::cout << "step 2.1 success" << std::endl;

  // step 2.2: convert the encrypted batch aggregation into secret shares
  // in order to do the exponential calculation
  // use 4 * fixed precision, encrypted_weights_precision + plaintext_samples_precision
  std::vector<double> batch_aggregation_shares;
  party.ciphers_to_secret_shares(
      encrypted_batch_aggregation,
      batch_aggregation_shares,
      cur_batch_size,
      ACTIVE_PARTY_ID,
      encrypted_batch_aggregation_precision);

  LOG(INFO) << "[forward_computation]: ciphers_to_secret_shares success" << " --------";
  std::cout << "[forward_computation]: ciphers_to_secret_shares success" << std::endl;

  // std::cout << "step 2.2 success" << std::endl;
  LOG(INFO) << "[spdz_logistic_function_computation]: length:" << party.executor_mpc_ports.size();
  std::cout <<  "[spdz_logistic_function_computation]: length:" << party.executor_mpc_ports.size() << std::endl;
  for (int i = 0; i < party.party_num; i++) {
    LOG(INFO) << "[spdz_logistic_function_computation]: base:" << party.executor_mpc_ports[i]  << "mpc_port_bases[i] + i: "<< party.executor_mpc_ports[i] + i;
    std::cout <<  "[spdz_logistic_function_computation]: base:" << party.executor_mpc_ports[i]  << "mpc_port_bases[i] + i: "<< party.executor_mpc_ports[i] + i << std::endl;
  }

  // step 2.3: communicate with spdz parties and receive results
  // the spdz_logistic_function_computation will do the 1/(1+e^(wx)) operation
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values =
      promise_values.get_future();
  std::thread spdz_thread(spdz_logistic_function_computation,
                          party.party_num,
                          party.party_id,
                          party.executor_mpc_ports,
                          SPDZ_PLAYER_PATH,
                          party.host_names,
                          batch_aggregation_shares,
                          cur_batch_size,
                          &promise_values);

  vector< double> batch_logistic_shares = future_values.get();
  // main thread wait spdz_thread to finish
  spdz_thread.join();

  LOG(INFO) << "[forward_computation]: call spdz_logistic_function_computation to do  1/(1+e^(wx)) operation success" << " --------";
  std::cout << "[forward_computation]: call spdz_logistic_function_computation to do  1/(1+e^(wx)) operation success" << std::endl;

  // active party receive all shares from other party and do the aggregation,
  // and board-cast predicted labels to other party.
  // use 1.5 * fixed precision
  int update_precision = (encrypted_batch_aggregation_precision - PHE_FIXED_POINT_PRECISION) / 2;
  party.secret_shares_to_ciphers(predicted_labels,
                                 batch_logistic_shares,
                                 cur_batch_size,
                                 ACTIVE_PARTY_ID,
                                 update_precision);
  LOG(INFO) << "[forward_computation]: secret_shares_to_ciphers success" << " --------";
  std::cout << "[forward_computation]: secret_shares_to_ciphers success" << std::endl;

  // std::cout << "step 2.3 success" << std::endl;
  std::cout << "step 2:forward_computation success" << std::endl;

  delete[] encrypted_batch_aggregation;
}

void LogisticRegressionModel::compute_batch_phe_aggregation(
    const Party &party,
    int cur_batch_size,
    EncodedNumber** encoded_batch_samples,
    int precision,
    EncodedNumber *encrypted_batch_aggregation) const {

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  LOG(INFO) << "[LogisticRegressionModel.compute_batch_phe_aggregation]: getter_phe_pub_key success" << " --------";
  std::cout << "[LogisticRegressionModel.compute_batch_phe_aggregation]: getter_phe_pub_key success" << std::endl;

  // each party compute local homomorphic aggregation
  auto* local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
  djcs_t_aux_matrix_mult(phe_pub_key, party.phe_random,
                         local_batch_phe_aggregation, local_weights,
                         encoded_batch_samples, cur_batch_size, weight_size);

  LOG(INFO) << "[LogisticRegressionModel.compute_batch_phe_aggregation]: djcs_t_aux_matrix_mult success" << " --------";
  std::cout << "[LogisticRegressionModel.compute_batch_phe_aggregation]: djcs_t_aux_matrix_mult success" << std::endl;
  google::FlushLogFiles(google::INFO);
  // every party sends the local aggregation to the active party
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // copy self local aggregation results
    for (int i = 0; i < cur_batch_size; i++) {
      // each element (i) in local_batch_phe_aggregation is [W].Xi,
      // Xi is a part of feature vector in example i
      encrypted_batch_aggregation[i] = local_batch_phe_aggregation[i];
    }

    // 1. active party receive serialized string from other parties
    // deserialize and sum to batch_phe_aggregation
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        auto* recv_batch_phe_aggregation =
            new EncodedNumber[cur_batch_size];
        std::string recv_local_aggregation_str;
        try {
          party.recv_long_message(id, recv_local_aggregation_str);
        } catch (const boost::system::system_error& ex)
        {
          cout << "Failed to receive. " << ex.what() << endl;
        } catch (const std::string& exs) {
          cout << "String error. " << exs << endl;
        } catch (...) {
          cout << "Other errors. " << endl;
        }

        LOG(INFO) << "[LogisticRegressionModel.compute_batch_phe_aggregation]: active party recv_long_message from " << id <<" success" << " --------";
        std::cout << "[LogisticRegressionModel.compute_batch_phe_aggregation]: active party recv_long_message from " << id <<" success" << std::endl;

        deserialize_encoded_number_array(recv_batch_phe_aggregation,
                                         cur_batch_size,
                                         recv_local_aggregation_str);
        // homomorphic addition of the received aggregations
        // After addition, each element (i) in batch_phe_aggregation is
        // [W1].Xi1+[W2].Xi2+...+[Wn].Xin Xin is example i's n-th feature,
        // W1 is first element in weigh vector
        for (int i = 0; i < cur_batch_size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, encrypted_batch_aggregation[i],
                            encrypted_batch_aggregation[i],
                            recv_batch_phe_aggregation[i]);
        }
        delete[] recv_batch_phe_aggregation;
      }
    }

    LOG(INFO) << "[LogisticRegressionModel.compute_batch_phe_aggregation]: active party do djcs_t_aux_ee_add success" << " --------";
    std::cout << "[LogisticRegressionModel.compute_batch_phe_aggregation]: active party do djcs_t_aux_ee_add success" << std::endl;

    // 2. active party serialize and send the aggregated
    // batch_phe_aggregation to other parties
    std::string global_aggregation_str;
    serialize_encoded_number_array(encrypted_batch_aggregation, cur_batch_size,
                                   global_aggregation_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, global_aggregation_str);
      }
    }
    LOG(INFO) << "[LogisticRegressionModel.compute_batch_phe_aggregation]: active party send_long_message success" << " --------";
    std::cout << "[LogisticRegressionModel.compute_batch_phe_aggregation]: active party send_long_message success" << std::endl;
  } else {
    // if it's passive party, it will not do the aggregation,
    // 1. it just serializes local batch aggregation and send to active party
    std::string local_aggregation_str;
    serialize_encoded_number_array(local_batch_phe_aggregation, cur_batch_size,
                                   local_aggregation_str);
    try {
      party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);
    } catch (const boost::system::system_error& ex)
    {
      cout << "Failed to receive. " << ex.what() << endl;
    } catch (const std::string& exs) {
      cout << "String error. " << exs << endl;
    } catch (...) {
      cout << "Other errors. " << endl;
    }

    LOG(INFO) << "[LogisticRegressionModel.compute_batch_phe_aggregation]: passive party send_long_message success" << " --------";
    std::cout << "[LogisticRegressionModel.compute_batch_phe_aggregation]: passive party send_long_message success" << std::endl;
    // 2. passive party receive the global batch aggregation
    // from the active party
    std::string recv_global_aggregation_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_aggregation_str);
    deserialize_encoded_number_array(encrypted_batch_aggregation, cur_batch_size,
                                     recv_global_aggregation_str);
    LOG(INFO) << "[LogisticRegressionModel.compute_batch_phe_aggregation]: passive party deserialize_encoded_number_array success" << " --------";
    std::cout << "[LogisticRegressionModel.compute_batch_phe_aggregation]: passive party deserialize_encoded_number_array success" << std::endl;
  }

  djcs_t_free_public_key(phe_pub_key);

  delete[] local_batch_phe_aggregation;
}


void LogisticRegressionModel::encode_samples(
    const Party &party,
    const std::vector<std::vector<double>>& used_samples,
    EncodedNumber** encoded_samples) const {

  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // retrieve batch samples and encode (notice to use cur_batch_size
  // instead of default batch size to avoid unexpected batch)
  int cur_sample_size = used_samples.size();

  for (int i = 0; i < cur_sample_size; i++) {
    for (int j = 0; j < weight_size; j++) {
      encoded_samples[i][j].set_double(
          phe_pub_key->n[0],
          used_samples[i][j],
          PHE_FIXED_POINT_PRECISION);
    }
  }

  djcs_t_free_public_key(phe_pub_key);
}


void retrieve_prediction_result(
    int sample_size,
    EncodedNumber* decrypted_labels,
    std::vector<double>* labels,
    std::vector< std::vector<double> >* probabilities){


  for (int i = 0; i < sample_size; i++) {
    double t;
    decrypted_labels[i].decode(t);
    std::vector<double> prob;
    prob.push_back(t);
    prob.push_back(1 - t);
    t = t >= LOGREG_THRES ? CERTAIN_PROBABILITY : ZERO_PROBABILITY;
    // std::cout << "label = " << t << std::endl;
    (*labels).push_back(t);
    (*probabilities).push_back(prob);
  }
  std::cout << "Compute prediction finished" << std::endl;
  LOG(INFO) << "Compute prediction finished";
}

void spdz_logistic_function_computation(int party_num,
                                        int party_id,
                                        std::vector<int> mpc_port_bases,
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
    LOG(INFO) << "[spdz_logistic_function_computation]: base:" << mpc_port_bases[i]  << "mpc_port_bases[i] + i: "<< mpc_port_bases[i] + i;
    std::cout <<  "[spdz_logistic_function_computation]: base:" << mpc_port_bases[i]  << "mpc_port_bases[i] + i: "<< mpc_port_bases[i] + i << std::endl;

    set_up_client_socket(plain_sockets[i], party_host_names[i].c_str(), mpc_port_bases[i] + i);
    send(plain_sockets[i], (octet*) &party_id, sizeof(int));
    mpc_sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                    "P" + to_string(i), "C" + to_string(party_id), true);
    if (i == 0){
      // receive gfp prime
      specification.Receive(mpc_sockets[0]);
    }
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                                                          " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_port_bases[i] + i << ".";
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