//
// Created by wuyuncheng on 7/7/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/logger/logger.h>

#include <cmath>
#include <glog/logging.h>
#include <stack>
#include <Networking/ssl_sockets.h>
#include <iostream>


LogisticRegressionModel::LogisticRegressionModel() = default;

LogisticRegressionModel::LogisticRegressionModel(int m_weight_size) : LinearModel(m_weight_size) {}

LogisticRegressionModel::~LogisticRegressionModel() = default;

LogisticRegressionModel::LogisticRegressionModel(const LogisticRegressionModel& log_reg_model)
  : LinearModel(log_reg_model) {}

LogisticRegressionModel& LogisticRegressionModel::operator=(const LogisticRegressionModel &log_reg_model) {
  LinearModel::operator=(log_reg_model);
  return *this;
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
    encoded_batch_samples[i] = new EncodedNumber[weight_size];
  }
  encode_samples(party, predicted_samples, encoded_batch_samples);

  int ciphertext_precision = 0 - local_weights[0].getter_exponent();
  int plaintext_precision = 0 - encoded_batch_samples[0][0].getter_exponent();
  int encrypted_batch_aggregation_precision = ciphertext_precision + plaintext_precision;

  forward_computation(party,
      predicted_samples.size(),
      encoded_batch_samples,
      encrypted_batch_aggregation_precision,
      predicted_labels);

  for (int i = 0; i < cur_sample_size; i++) {
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

  compute_batch_phe_aggregation(party,
      cur_batch_size,
      encoded_batch_samples,
      PHE_FIXED_POINT_PRECISION,
      encrypted_batch_aggregation);

  log_info("[forward_computation]: batch phe aggregation success");

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

  log_info("[forward_computation]: ciphers_to_secret_shares success");

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

  log_info("[forward_computation]: call spdz_logistic_function_computation to do 1/(1+e^(wx)) operation success");

  // active party receive all shares from other party and do the aggregation,
  // and board-cast predicted labels to other party.
  // use 1.5 * fixed precision
  int update_precision = (encrypted_batch_aggregation_precision - PHE_FIXED_POINT_PRECISION) / 2;
  party.secret_shares_to_ciphers(predicted_labels,
                                 batch_logistic_shares,
                                 cur_batch_size,
                                 ACTIVE_PARTY_ID,
                                 update_precision);
  log_info("[forward_computation]: secret_shares_to_ciphers success");
  log_info("step 2: forward_computation success");

  delete[] encrypted_batch_aggregation;
}


void retrieve_prediction_result(
    int sample_size,
    EncodedNumber* decrypted_labels,
    std::vector<double>* labels,
    std::vector< std::vector<double> >* probabilities) {
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
  log_info("Compute prediction finished");
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
  log_info("begin connect to spdz parties");
  log_info("party_num = " + std::to_string(party_num));
  for (int i = 0; i < party_num; i++)
  {
    log_info("[spdz_logistic_function_computation]: "
             "base:" + std::to_string(mpc_port_bases[i])
             + ", mpc_port_bases[i] + i: " + std::to_string(mpc_port_bases[i] + i));

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
  log_info("Finish setup socket connections to spdz engines.");
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
  log_info("Finish initializing gfp field.");
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size() << std::endl;

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