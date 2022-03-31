//
// Created by wuyuncheng on 13/8/20.
//

#include <falcon/common.h>
#include <falcon/operator/phe/djcs_t_aux.h>
#include <falcon/party/party.h>
#include <falcon/utils/io_util.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/network_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/utils/base64.h>
#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>
#include <cstdlib>
#include <ctime>
#include <falcon/network/ConfigFile.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stack>
#include <string>
#include "omp.h"

Party::Party() {}

Party::Party(int m_party_id, int m_party_num, falcon::PartyType m_party_type,
             falcon::FLSetting m_fl_setting, const std::string& m_data_file) {
  // copy params
  party_id = m_party_id;
  party_num = m_party_num;
  party_type = m_party_type;
  fl_setting = m_fl_setting;

  // read dataset
  char delimiter = ',';
  local_data = read_dataset(m_data_file, delimiter);
  sample_num = local_data.size();
  feature_num = local_data[0].size();

  LOG(INFO) << "local_data is reading dataset from: " << m_data_file;
  LOG(INFO) << "sample_num = " << sample_num;

  // if this is active party, slice the last column as labels
  if (party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < sample_num; i++) {
      labels.push_back(local_data[i][feature_num - 1]);
      local_data[i].pop_back();
    }
    LOG(INFO) << "Active party last column is label, so feature_num-1\n";
    --feature_num;
  }
  LOG(INFO) << "feature_num = " << feature_num;
}

Party::Party(const Party& party) {
  // copy public variables
  party_id = party.party_id;
  party_num = party.party_num;
  party_type = party.party_type;
  fl_setting = party.fl_setting;
  channels = party.channels;
  host_names = party.host_names;
  executor_mpc_ports = party.executor_mpc_ports;

  // copy private variables
  feature_num = party.getter_feature_num();
  sample_num = party.getter_sample_num();
  local_data = party.getter_local_data();
  labels = party.getter_labels();

  // init phe keys and copy from party
  phe_random = hcs_init_random();
  phe_pub_key = djcs_t_init_public_key();
  phe_auth_server = djcs_t_init_auth_server();
  djcs_t_public_key* tmp_pub_key = djcs_t_init_public_key();
  djcs_t_auth_server* tmp_auth_server = djcs_t_init_auth_server();
  party.getter_phe_pub_key(tmp_pub_key);
  party.getter_phe_auth_server(tmp_auth_server);
  djcs_t_public_key_copy(tmp_pub_key, phe_pub_key);
  djcs_t_auth_server_copy(tmp_auth_server, phe_auth_server);

  // free tmp phe key objects
  djcs_t_free_public_key(tmp_pub_key);
  djcs_t_free_auth_server(tmp_auth_server);
}

Party& Party::operator=(const Party& party) {
  // copy public variables
  party_id = party.party_id;
  party_num = party.party_num;
  party_type = party.party_type;
  fl_setting = party.fl_setting;
  channels = party.channels;
  host_names = party.host_names;
  executor_mpc_ports = party.executor_mpc_ports;

  // copy private variables
  feature_num = party.getter_feature_num();
  sample_num = party.getter_sample_num();
  local_data = party.getter_local_data();
  labels = party.getter_labels();

  // init phe keys and copy from party
  phe_random = hcs_init_random();
  phe_pub_key = djcs_t_init_public_key();
  phe_auth_server = djcs_t_init_auth_server();
  djcs_t_public_key* tmp_pub_key = djcs_t_init_public_key();
  djcs_t_auth_server* tmp_auth_server = djcs_t_init_auth_server();
  party.getter_phe_pub_key(tmp_pub_key);
  party.getter_phe_auth_server(tmp_auth_server);
  djcs_t_public_key_copy(tmp_pub_key, phe_pub_key);
  djcs_t_auth_server_copy(tmp_auth_server, phe_auth_server);

  // free tmp phe key objects
  djcs_t_free_public_key(tmp_pub_key);
  djcs_t_free_auth_server(tmp_auth_server);
  return *this;
}

void Party::init_network_channels(const std::string &m_network_file) {
  std::vector<std::string> ips;
  std::vector<std::vector<int> > port_arrays;

  if (NETWORK_CONFIG_PROTO == 0) {
    // read network config file
    ConfigFile config_file(m_network_file);
    std::string port_string, ip_string;
    std::vector<int> ports(party_num);
    for (int i = 0; i < party_num; i++) {
      port_string = "party_" + to_string(i) + "_port";
      ip_string = "party_" + to_string(i) + "_ip";
      // get parties ips and ports
      ports[i] = stoi(config_file.Value("", port_string));
      ips.push_back(config_file.Value("", ip_string));
    }
    // generate port_arrays for communication between parties
    for (int i = 0; i < party_num; ++i) {
      std::vector<int> tmp_ports;
      if (i <= party_id) {
        tmp_ports.push_back(ports[party_id] + i);
        tmp_ports.push_back(ports[i] + party_id - 1);
      }else if (i > party_id){
        tmp_ports.push_back(ports[party_id] + i - 1);
        tmp_ports.push_back(ports[i] + party_id);
      }
      port_arrays.push_back(tmp_ports);
    }
  } else {
    // read network config proto from coordinator
//    // decode the network base64 string to pb string
//    vector<BYTE> network_file_byte = base64_decode(m_network_file);
//    std::string network_pb_string(network_file_byte.begin(), network_file_byte.end());
    deserialize_network_configs(ips, port_arrays, executor_mpc_ports, m_network_file);

    LOG(INFO) << "[deserialize_network_configs]: mpc_port_array_size after: " << executor_mpc_ports.size() <<" --------";
    std::cout << "[deserialize_network_configs]: mpc_port_array_size after: " << executor_mpc_ports.size() << ", and ips are " << ips.size()  << std::endl;

    LOG(INFO) << "[deserialize_network_configs]: size of ip address is: " << ips.size() <<" --------";

    for (int i = 0; i < ips.size(); i++) {
      std::cout << "ips[" << i << "] = " << ips[i] << std::endl;
      LOG(INFO) << "ips[" << i << "] = " << ips[i];
      for (int j = 0; j < port_arrays[i].size(); j++) {
        LOG(INFO) << "port_array[" << i << "][" << j
                  << "] = " << port_arrays[i][j];
        std::cout << "port_array[" << i << "][" << j
                  << "] = " << port_arrays[i][j] << std::endl;
      }
    }

    for (int i = 0; i < executor_mpc_ports.size(); i++) {
      log_info("executor_mpc_ports[" + std::to_string(i) + "] = " + std::to_string(executor_mpc_ports[i]));
    }
  }

  host_names = ips;

  // establish communication connections
  std::cout << "Establish network communications with other parties" << std::endl;
  LOG(INFO) << "Establish network communications with other parties";
  google::FlushLogFiles(google::INFO);
  SocketPartyData me, other;
  for (int i = 0; i < party_num; ++i) {
    if (i != party_id) {
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]),
                           port_arrays[party_id][i]);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]),
                              port_arrays[i][party_id]);

      std::cout << "My party id is: " << party_id << ", I am listening party " << i
                << " on port " << port_arrays[party_id][i] << std::endl;
      LOG(INFO) << "My party id is: " << party_id << ", I am listening party " << i
                << " on port " << port_arrays[party_id][i];
      google::FlushLogFiles(google::INFO);

      shared_ptr<CommParty> channel =
          make_shared<CommPartyTCPSynced>(io_service, me, other);

      std::cout << "The other party's port I will send data to is: " <<
                port_arrays[i][party_id] << std::endl;
      LOG(INFO) << "The other party's port I will send data to is: " <<
                port_arrays[i][party_id];
      google::FlushLogFiles(google::INFO);

      // connect to the other party and add channel
      channel->join(500, 5000);
      LOG(INFO) << "Communication channel established with party " << i
                << ", port is " << port_arrays[party_id][i] << ".";
      std::cout << "Communication channel established with party " << i
                << ", port is " << port_arrays[party_id][i] << "." << std::endl;
      channels.push_back(std::move(channel));
      google::FlushLogFiles(google::INFO);
    } else {
      // self communication
      shared_ptr<CommParty> channel = nullptr;
      channels.push_back(std::move(channel));
    }
  }
}

void Party::init_phe_keys(bool m_use_existing_key, const std::string &m_key_file) {
  LOG(INFO) << "Init threshold partially homomorphic encryption keys";
  std::cout << "Init threshold partially homomorphic encryption keys" << std::endl;
  google::FlushLogFiles(google::INFO);
  // init phe keys: if use existing key, read key file
  // otherwise, generate keys and broadcast to others
  phe_random = hcs_init_random();
  phe_pub_key = djcs_t_init_public_key();
  phe_auth_server = djcs_t_init_auth_server();
  std::cout << "Initialize phe keys success" << std::endl;
  std::cout << "m_use_existing_key = " << m_use_existing_key << std::endl;
  LOG(INFO) << "Initialize phe keys success";
  LOG(INFO) << "m_use_existing_key = " << m_use_existing_key;
  google::FlushLogFiles(google::INFO);
  if (m_use_existing_key) {
    init_with_key_file(m_key_file);
  } else {
    // the active party generates new phe keys and send to passive parties
    std::cout << "init with new phe keys begin" << std::endl;
    LOG(INFO) << "init with new phe keys begin";
    google::FlushLogFiles(google::INFO);
    init_with_new_phe_keys(PHE_EPSILON, PHE_KEY_SIZE, party_num);
    std::cout << "init with new phe keys success" << std::endl;
    LOG(INFO) << "init with new phe keys success";
    google::FlushLogFiles(google::INFO);
    std::string phe_keys_str;
    // LOG(INFO) << "create keys succeed";
    serialize_phe_keys(phe_pub_key, phe_auth_server, phe_keys_str);
    std::cout << "serialize phe keys success" << std::endl;
    LOG(INFO) << "serialize phe keys success";
    google::FlushLogFiles(google::INFO);
    // LOG(INFO) << "serialize keys succeed";
    write_key_to_file(phe_keys_str, m_key_file);
    std::cout << "write phe keys success" << std::endl;
    LOG(INFO) << "write phe keys success";
    // LOG(INFO) << "write keys succeed";
    google::FlushLogFiles(google::INFO);
  }
}

void Party::init_with_new_phe_keys(int epsilon, int phe_key_size,
                                   int required_party_num) {
  LOG(INFO) << "party type = " << party_type;
  google::FlushLogFiles(google::INFO);
  std::cout << "party type = " << party_type << std::endl;
  if (party_type == falcon::ACTIVE_PARTY) {
    LOG(INFO) << "active party begins to generate phe keys";
    google::FlushLogFiles(google::INFO);
    std::cout << "active party begins to generate phe keys" << std::endl;

    // generate phe keys
    hcs_random* random = hcs_init_random();
    djcs_t_public_key* pub_key = djcs_t_init_public_key();
    djcs_t_private_key* priv_key = djcs_t_init_private_key();

    LOG(INFO) << "step 1";
    google::FlushLogFiles(google::INFO);
    std::cout << "step 1" << std::endl;

    djcs_t_auth_server** auth_server =
        (djcs_t_auth_server**)malloc(party_num * sizeof(djcs_t_auth_server*));
    mpz_t* si = (mpz_t*)malloc(party_num * sizeof(mpz_t));
    djcs_t_generate_key_pair(pub_key, priv_key, random, epsilon, phe_key_size,
                             party_num, required_party_num);

    LOG(INFO) << "step 2";
    google::FlushLogFiles(google::INFO);
    std::cout << "step 2" << std::endl;

    mpz_t* coeff = djcs_t_init_polynomial(priv_key, random);
    for (int i = 0; i < party_num; i++) {
      mpz_init(si[i]);
      djcs_t_compute_polynomial(priv_key, coeff, si[i], i);
      auth_server[i] = djcs_t_init_auth_server();
      djcs_t_set_auth_server(auth_server[i], si[i], i);
    }

    // serialize phe keys and send to passive parties
    LOG(INFO) << "party id = " << party_id;
    google::FlushLogFiles(google::INFO);
    std::cout << "party id = " << party_id << std::endl;
    for (int i = 0; i < party_num; i++) {
      if (i == party_id) {
        djcs_t_public_key_copy(pub_key, phe_pub_key);
        djcs_t_auth_server_copy(auth_server[i], phe_auth_server);
        // LOG(INFO) << "i = " << i << ", party id = " << party_id << ", copy
        // succeed";
      } else {
        std::string phe_keys_message_i;
        serialize_phe_keys(pub_key, auth_server[i], phe_keys_message_i);
        send_long_message(i, phe_keys_message_i);
      }
    }

    // free memory
    hcs_free_random(random);
    djcs_t_free_public_key(pub_key);
    djcs_t_free_polynomial(priv_key, coeff);
    for (int i = 0; i < party_num; i++) {
      mpz_clear(si[i]);
      djcs_t_free_auth_server(auth_server[i]);
    }
    free(si);
    free(auth_server);

    djcs_t_free_private_key(priv_key);

    LOG(INFO) << "free succeed";
    google::FlushLogFiles(google::INFO);
    std::cout << "free succeed" << std::endl;
    // LOG(INFO) << "free succeed";
    // google::FlushLogFiles(google::INFO);
  } else {
    // for passive parties, receive the phe keys message from the active party
    // and set its own keys with the received message
    // TODO: now active party id is 0 by design, could abstract as a variable
    LOG(INFO) << "receive serialized keys from the active party";
    google::FlushLogFiles(google::INFO);
    std::cout << "receive serialized keys from the active party" << std::endl;

    std::string recv_phe_keys_message;
    recv_long_message(0, recv_phe_keys_message);
    deserialize_phe_keys(phe_pub_key, phe_auth_server, recv_phe_keys_message);

    LOG(INFO) << "deserialize the phe keys";
    google::FlushLogFiles(google::INFO);
    std::cout << "deserialize the phe keys" << std::endl;
  }
}

void Party::init_with_key_file(const std::string& key_file) {
  // read key file as string
  std::string phe_keys_str = read_key_file(key_file);
  deserialize_phe_keys(phe_pub_key, phe_auth_server, phe_keys_str);
}

std::string Party::export_phe_key_string() {
  std::string phe_keys_str;
  serialize_phe_keys(phe_pub_key, phe_auth_server, phe_keys_str);
  return phe_keys_str;
}

void Party::load_phe_key_string(const std::string &phe_keys_str) {
  phe_random = hcs_init_random();
  phe_pub_key = djcs_t_init_public_key();
  phe_auth_server = djcs_t_init_auth_server();
  deserialize_phe_keys(phe_pub_key, phe_auth_server, phe_keys_str);
}

void Party::send_message(int id, std::string message) const {
  channels[id]->write((const byte*)message.c_str(), message.size());
}

void Party::send_long_message(int id, string message) const {
  channels[id]->writeWithSize(message);
}

void Party::recv_message(int id, std::string message, byte* buffer,
                         int expected_size) const {
  channels[id]->read(buffer, expected_size);
  // the size of all strings is 2. Parse the message to get the original strings
  auto s = string(reinterpret_cast<char const*>(buffer), expected_size);
  message = s;
}

void Party::recv_long_message(int id, std::string& message) const {
  vector<byte> recv_message;
  channels[id]->readWithSizeIntoVector(recv_message);
  const byte* uc = &(recv_message[0]);
  string recv_message_str(reinterpret_cast<char const*>(uc),
                          recv_message.size());
  message = recv_message_str;
}

void Party::split_train_test_data(
    double split_percentage, std::vector<std::vector<double> >& training_data,
    std::vector<std::vector<double> >& testing_data,
    std::vector<double>& training_labels,
    std::vector<double>& testing_labels) const {
  LOG(INFO) << "Split local data and labels into training and testing dataset.";
  int training_data_size = (int) (sample_num * split_percentage);
  LOG(INFO) << "Split percentage for train-test = " << split_percentage;
  LOG(INFO) << "training_data_size = " << training_data_size;

  std::vector<int> data_indexes;

  // if the party is active party, shuffle the data and labels,
  // and send the shuffled indexes to passive parties for splitting accordingly
  if (party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < sample_num; i++) {
      data_indexes.push_back(i);
    }
    // without seed:
    // std::random_device rd;
    // std::default_random_engine rng(rd());
    // using seed, for development:
    std::default_random_engine rng(RANDOM_SEED);
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);
//    // print the shuffled data indexes for debug
//    for (int i = 0; i < sample_num; i++) {
//      LOG(INFO) << "shuffled data indexes[" << i << "] = " << data_indexes[i];
//    }
    // select the former training data size as training data,
    // and the latter as testing data
    for (int i = 0; i < sample_num; i++) {
      if (i < training_data_size) {
        // add to training dataset and labels
        training_data.push_back(local_data[data_indexes[i]]);
        training_labels.push_back(labels[data_indexes[i]]);
      } else {
        // add to testing dataset and labels
        testing_data.push_back(local_data[data_indexes[i]]);
        testing_labels.push_back(labels[data_indexes[i]]);
      }
    }

    // send the data_indexes to passive parties
    for (int i = 0; i < party_num; i++) {
      if (i != party_id) {
        std::string shuffled_indexes_str;
        serialize_int_array(data_indexes, shuffled_indexes_str);
        // send the shuffled_indexes_str protobuf over the network
        send_long_message(i, shuffled_indexes_str);
      }
    }

#ifdef DEBUG
    // save shuffled data_indexes vector<int> local debugging
    std::ostringstream oss;
    oss << TEST_IO_OUTDIR << "/falcon_client0_data_indexes_seed" << RANDOM_SEED
        << ".txt";
    std::string shuffled_data_indexes_file_name = oss.str();

    write_shuffled_data_indexes_to_file(data_indexes, shuffled_data_indexes_file_name);

    // check if the data_indexes match the saved txt in TEST_IO_OUTDIR
    LOG(INFO) << "data_indexes[0] = " << data_indexes[0] << std::endl;
    LOG(INFO) << "data_indexes[1] = " << data_indexes[1] << std::endl;
    LOG(INFO) << "data_indexes[2] = " << data_indexes[2] << std::endl;

    char delimiter = ',';
    // save active party's training_data and testing_data
    // write vector<vector<double>> to file in TEST_IO_OUTDIR
    oss.str(std::string());  // reset the ostringstream
    oss << TEST_IO_OUTDIR << "/falcon_client0_training_data_seed" << RANDOM_SEED
        << ".txt";
    std::string training_data_file_name = oss.str();
    write_dataset_to_file(training_data, delimiter, training_data_file_name);
    LOG(INFO) << "saved a copy of training_data in TEST_IO_OUTDIR\n";

    oss.str(std::string());  // reset the ostringstream
    oss << TEST_IO_OUTDIR << "/falcon_client0_testing_data_seed" << RANDOM_SEED
        << ".txt";
    std::string testing_data_file_name = oss.str();
    write_dataset_to_file(testing_data, delimiter, testing_data_file_name);
    LOG(INFO) << "saved a copy of testing_data in TEST_IO_OUTDIR\n";
#endif

  }

  // receive shuffled indexes and split in the same way
  else {
    std::string recv_shuffled_indexes_str;
    recv_long_message(0, recv_shuffled_indexes_str);
    deserialize_int_array(data_indexes, recv_shuffled_indexes_str);

    if (data_indexes.size() != sample_num) {
      LOG(ERROR)
          << "Received size of shuffled indexes does not equal to sample num.";
      return;
    }

    // split local data accordingly
    for (int i = 0; i < sample_num; i++) {
      if (i < training_data_size) {
        training_data.push_back(local_data[data_indexes[i]]);
      } else {
        testing_data.push_back(local_data[data_indexes[i]]);
      }
    }
  }
}

void Party::collaborative_decrypt(EncodedNumber* src_ciphers,
                                  EncodedNumber* dest_plains, int size,
                                  int req_party_id) const {
  // partially decrypt the ciphertext vector
  EncodedNumber* partial_decryption = new EncodedNumber[size];
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    djcs_t_aux_partial_decrypt(phe_pub_key, phe_auth_server,
                               partial_decryption[i], src_ciphers[i]);
  }

  if (party_id == req_party_id) {
    // create 2D-array m*n to store the decryption shares,
    // m = size, n = party_num, such that each row i represents
    // all the shares for the i-th ciphertext
    EncodedNumber** decryption_shares = new EncodedNumber*[size];
    for (int i = 0; i < size; i++) {
      decryption_shares[i] = new EncodedNumber[party_num];
    }

    // collect all the decryption shares
    for (int id = 0; id < party_num; id++) {
      if (id == party_id) {
        // copy self partially decrypted shares
        for (int i = 0; i < size; i++) {
          decryption_shares[i][id] = partial_decryption[i];
        }
      } else {
        std::string recv_partial_decryption_str;
        recv_long_message(id, recv_partial_decryption_str);
        EncodedNumber* recv_partial_decryption = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_partial_decryption, size,
                                         recv_partial_decryption_str);
        // copy other party's decrypted shares
        for (int i = 0; i < size; i++) {
          decryption_shares[i][id] = recv_partial_decryption[i];
        }
        delete[] recv_partial_decryption;
      }
    }

    // share combine for decryption
    omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
    for (int i = 0; i < size; i++) {
      djcs_t_aux_share_combine(phe_pub_key, dest_plains[i],
                               decryption_shares[i], party_num);
    }

    // free memory
    for (int i = 0; i < size; i++) {
      delete[] decryption_shares[i];
    }
    delete[] decryption_shares;
  } else {
    // send decrypted shares to the req_party_id
    std::string partial_decryption_str;
    serialize_encoded_number_array(partial_decryption, size,
                                   partial_decryption_str);
    send_long_message(req_party_id, partial_decryption_str);
  }

  delete[] partial_decryption;
}

void Party::ciphers_to_secret_shares(EncodedNumber* src_ciphers,
                                     std::vector<double>& secret_shares,
                                     int size, int req_party_id,
                                     int phe_precision) const {
  // each party generates a random vector with size values
  // (the request party will add the summation to the share)
  // ui randomly chooses ri belongs to Zq and encrypts it as [ri]
  auto* encrypted_shares = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    // TODO: check how to replace with spdz random values
    if (phe_precision != 0) {
      double s = static_cast<double>(rand() % MAXIMUM_RAND_VALUE);
      encrypted_shares[i].set_double(phe_pub_key->n[0], s, phe_precision);
      djcs_t_aux_encrypt(phe_pub_key, phe_random, encrypted_shares[i],
                         encrypted_shares[i]);
      secret_shares.push_back(0 - s);
    } else {
      int s = rand() % MAXIMUM_RAND_VALUE;
      encrypted_shares[i].set_integer(phe_pub_key->n[0], s);
      djcs_t_aux_encrypt(phe_pub_key, phe_random, encrypted_shares[i],
                         encrypted_shares[i]);
      secret_shares.push_back(0 - s);
    }
  }

  // request party aggregate the shares and invoke collaborative decryption
  auto* aggregated_shares = new EncodedNumber[size];
  if (party_id == req_party_id) {
    for (int i = 0; i < size; i++) {
      aggregated_shares[i] = encrypted_shares[i];
      djcs_t_aux_ee_add(phe_pub_key, aggregated_shares[i], aggregated_shares[i],
                        src_ciphers[i]);
    }
    // recv message and add to aggregated shares,
    // u1 computes [e] = [x]+[r1]+..+[rm]
    for (int id = 0; id < party_num; id++) {
      if (id != party_id) {
        std::string recv_encrypted_shares_str;
        recv_long_message(id, recv_encrypted_shares_str);
        auto* recv_encrypted_shares = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_encrypted_shares, size,
                                         recv_encrypted_shares_str);
        omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, aggregated_shares[i],
                            aggregated_shares[i], recv_encrypted_shares[i]);
        }
        delete[] recv_encrypted_shares;
      }
    }
    // serialize and send to other parties
    std::string aggregated_shares_str;
    serialize_encoded_number_array(aggregated_shares, size,
                                   aggregated_shares_str);
    for (int id = 0; id < party_num; id++) {
      if (id != party_id) {
        send_long_message(id, aggregated_shares_str);
      }
    }
  } else {
    // send encrypted shares to the request party
    // ui sends [ri] to u1
    std::string encrypted_shares_str;
    serialize_encoded_number_array(encrypted_shares, size,
                                   encrypted_shares_str);
    send_long_message(req_party_id, encrypted_shares_str);

    // receive aggregated shares from the request party
    std::string recv_aggregated_shares_str;
    recv_long_message(req_party_id, recv_aggregated_shares_str);
    deserialize_encoded_number_array(aggregated_shares, size,
                                     recv_aggregated_shares_str);
  }
  // collaborative decrypt the aggregated shares, clients jointly decrypt [e]
  auto* decrypted_sum = new EncodedNumber[size];
  collaborative_decrypt(aggregated_shares, decrypted_sum, size, req_party_id);
  // if request party, add the decoded results to the secret shares
  // u1 sets [x]1 = e − r1 mod q
  if (party_id == req_party_id) {
    omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
    for (int i = 0; i < size; i++) {
      if (phe_precision != 0) {
        double decoded_sum_i;
        decrypted_sum[i].decode(decoded_sum_i);
        secret_shares[i] += decoded_sum_i;
      } else {
        long decoded_sum_i;
        decrypted_sum[i].decode(decoded_sum_i);
        secret_shares[i] += (double)decoded_sum_i;
      }
    }
  }

  delete[] encrypted_shares;
  delete[] aggregated_shares;
  delete[] decrypted_sum;
}

void Party::secret_shares_to_ciphers(EncodedNumber* dest_ciphers,
                                     std::vector<double> secret_shares,
                                     int size, int req_party_id,
                                     int phe_precision) const {
//  LOG(INFO) << "[secret_shares_to_ciphers]: enter secret shares to ciphers func" << " --------";
//  std::cout << "[secret_shares_to_ciphers]: enter secret shares to ciphers func" << std::endl;

  // encode and encrypt the secret shares
  // and send to req_party, who aggregates
  // and send back to the other parties
  auto* encrypted_shares = new EncodedNumber[size];
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    encrypted_shares[i].set_double(phe_pub_key->n[0], secret_shares[i],
                                   phe_precision);
    djcs_t_aux_encrypt(phe_pub_key, phe_random, encrypted_shares[i],
                       encrypted_shares[i]);
  }

//  LOG(INFO) << "[secret_shares_to_ciphers]: step 1" << " --------";
//  std::cout << "[secret_shares_to_ciphers]: step 1" << std::endl;
//
//  LOG(INFO) << "[secret_shares_to_ciphers]: party_id = " << party_id << ", req_party_id = " << req_party_id;
//  std::cout << "[secret_shares_to_ciphers]: party_id = " << party_id << ", req_party_id = " << req_party_id << std::endl;
//
//  LOG(INFO) << "[secret_shares_to_ciphers]: size = " << size << " --------";
//  std::cout << "[secret_shares_to_ciphers]: size = " << size << std::endl;

  if (party_id == req_party_id) {
    // copy local encrypted_shares to dest_ciphers
    for (int i = 0; i < size; i++) {
      dest_ciphers[i] = encrypted_shares[i];
    }

//    LOG(INFO) << "[secret_shares_to_ciphers]: step 2" << " --------";
//    std::cout << "[secret_shares_to_ciphers]: step 2" << std::endl;

    // receive from other parties and aggregate encrypted shares
    for (int id = 0; id < party_num; id++) {
      if (id != party_id) {
        std::string recv_encrypted_shares_str;
        recv_long_message(id, recv_encrypted_shares_str);
        auto* recv_encrypted_shares = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_encrypted_shares, size,
                                         recv_encrypted_shares_str);
        // homomorphic aggregation
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, dest_ciphers[i], dest_ciphers[i],
                            recv_encrypted_shares[i]);
        }
        delete[] recv_encrypted_shares;
      }
    }

//    LOG(INFO) << "[secret_shares_to_ciphers]: step 3" << " --------";
//    std::cout << "[secret_shares_to_ciphers]: step 3" << std::endl;

    // serialize dest_ciphers and broadcast
    std::string dest_ciphers_str;
    serialize_encoded_number_array(dest_ciphers, size, dest_ciphers_str);
    for (int id = 0; id < party_num; id++) {
      if (id != party_id) {
        send_long_message(id, dest_ciphers_str);
      }
    }

//    LOG(INFO) << "[secret_shares_to_ciphers]: step 4" << " --------";
//    std::cout << "[secret_shares_to_ciphers]: step 4" << std::endl;
  } else {
    // serialize and send to req_party
    std::string encrypted_shares_str;
    serialize_encoded_number_array(encrypted_shares, size,
                                   encrypted_shares_str);
    send_long_message(req_party_id, encrypted_shares_str);

    // receive and set dest_ciphers
    std::string recv_dest_ciphers_str;
    recv_long_message(req_party_id, recv_dest_ciphers_str);
    deserialize_encoded_number_array(dest_ciphers, size, recv_dest_ciphers_str);
  }

  delete[] encrypted_shares;
}

void Party::broadcast_encoded_number_array(EncodedNumber *vec,
                                           int size, int req_party_id) const {
  if (party_id == req_party_id) {
    // serialize the encoded number vector and send to other parties
    std::string vec_str;
    serialize_encoded_number_array(vec, size, vec_str);
    for (int id = 0; id < party_num; id++) {
      if (id != party_id) {
        send_long_message(id, vec_str);
      }
    }
  } else {
    // receive the message from req_party_id and set to vec
    std::string recv_vec_str;
    recv_long_message(req_party_id, recv_vec_str);
    deserialize_encoded_number_array(vec, size, recv_vec_str);
  }
}

void Party::truncate_ciphers_precision(EncodedNumber *ciphers, int size,
                                       int req_party_id, int dest_precision) const {
  // check if the cipher precision is higher than the dest_precision
  // otherwise, no need to truncate the precision
  int src_precision = abs(ciphers[0].getter_exponent());
  if (src_precision == dest_precision) {
    LOG(INFO) << "src precision is the same as dest_precision, do nothing";
    return;
  }
  if (src_precision < dest_precision) {
    LOG(ERROR) << "cannot increase ciphertext precision";
    exit(1);
  }
  // step 1. broadcast the ciphers from req_party_id
  // broadcast_encoded_number_array(ciphers, size, req_party_id);
  // step 2. convert the ciphers into secret shares given src_precision
  std::vector<double> ciphers_shares;
  ciphers_to_secret_shares(ciphers,
                           ciphers_shares,
                           size,
                           req_party_id,
                           src_precision);
  google::FlushLogFiles(google::INFO);
  // step 3. convert the secret shares into ciphers given dest_precision
  auto *dest_ciphers = new EncodedNumber[size];
  secret_shares_to_ciphers(dest_ciphers,
                           ciphers_shares,
                           size,
                           req_party_id,
                           dest_precision);
  google::FlushLogFiles(google::INFO);
  // step 4. inplace ciphers by dest_ciphers
  for (int i = 0; i < size; i++) {
    ciphers[i] = dest_ciphers[i];
  }
  google::FlushLogFiles(google::INFO);
  LOG(INFO) << "truncate the ciphers precision finished";
  delete [] dest_ciphers;
}

void Party::ciphers_multi(EncodedNumber *res,
                          EncodedNumber *ciphers1,
                          EncodedNumber *ciphers2,
                          int size,
                          int req_party_id) const {
  // step 1: convert ciphers 1 to secret shares
  int cipher1_precision = std::abs(ciphers1[0].getter_exponent());
  int cipher2_precision = std::abs(ciphers2[0].getter_exponent());
  std::vector<double> ciphers1_shares;
  ciphers_to_secret_shares(ciphers1, ciphers1_shares,
                           size, req_party_id, cipher1_precision);
  auto* encoded_ciphers1_shares = new EncodedNumber[size];

  // step 2: aggregate the plaintext and ciphers2 multiplication
  auto* global_aggregation = new EncodedNumber[size];
  auto* local_aggregation = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    encoded_ciphers1_shares[i].set_double(phe_pub_key->n[0],
                                        ciphers1_shares[i]);
    djcs_t_aux_ep_mul(phe_pub_key,
                      local_aggregation[i],
                      ciphers2[i],
                      encoded_ciphers1_shares[i]);
  }
  if (party_id == req_party_id) {
    for (int i = 0; i < size; i++) {
      global_aggregation[i] = local_aggregation[i];
    }
    for (int id = 0; id < party_num; id++) {
      if (id != party_id) {
        auto* recv_local_aggregation = new EncodedNumber[size];
        std::string recv_local_aggregation_str;
        recv_long_message(id, recv_local_aggregation_str);
        deserialize_encoded_number_array(recv_local_aggregation,
                                         size, recv_local_aggregation_str);
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, global_aggregation[i],
                            global_aggregation[i], recv_local_aggregation[i]);
        }
        delete [] recv_local_aggregation;
      }
    }
  } else {
    // serialize and send to req_party_id
    std::string local_aggregation_str;
    serialize_encoded_number_array(local_aggregation, size, local_aggregation_str);
    send_long_message(req_party_id, local_aggregation_str);
  }
  broadcast_encoded_number_array(global_aggregation, size, req_party_id);

  // step 3: write to result vector
  for (int i = 0; i < size; i++) {
    res[i] = global_aggregation[i];
  }

  delete [] encoded_ciphers1_shares;
  delete [] global_aggregation;
  delete [] local_aggregation;
}

std::vector<int> Party::sync_up_int_arr(int v) const {
  std::vector<int> sync_arr;
  if (party_type == falcon::ACTIVE_PARTY) {
    // first set its own weight size and receive other parties' weight sizes
    sync_arr.push_back(v);
    for (int i = 0; i < party_num; i++) {
      if (i != party_id) {
        std::string recv_weight_size;
        recv_long_message(i, recv_weight_size);
        sync_arr.push_back(std::stoi(recv_weight_size));
      }
    }
    // then broadcast the vector
    std::string party_weight_sizes_str;
    serialize_int_array(sync_arr, party_weight_sizes_str);
    for (int i = 0; i < party_num; i++) {
      if (i != party_id) {
        send_long_message(i, party_weight_sizes_str);
      }
    }
  } else {
    // first send the weight size to active party
    send_long_message(ACTIVE_PARTY_ID, std::to_string(v));
    // then receive and deserialize the party_weight_sizes array
    std::string recv_party_weight_sizes_str;
    recv_long_message(ACTIVE_PARTY_ID, recv_party_weight_sizes_str);
    deserialize_int_array(sync_arr, recv_party_weight_sizes_str);
  }
  return sync_arr;
}

Party::~Party() {
  // if does not reset channels[i] to nullptr,
  // there will be a heap-use-after-free crash
  io_service.stop();
  for (int i = 0; i < party_num; i++) {
    channels[i] = nullptr;
  }
  hcs_free_random(phe_random);
  djcs_t_free_public_key(phe_pub_key);
  djcs_t_free_auth_server(phe_auth_server);
}
