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

  log_info("local_data is reading dataset from: " + m_data_file);
  log_info("sample_num = " + std::to_string(sample_num));
  // if this is active party, slice the last column as labels
  if (party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < sample_num; i++) {
      labels.push_back(local_data[i][feature_num - 1]);
      local_data[i].pop_back();
    }
    log_info("Active party last column is label, so feature_num - 1");
    --feature_num;
  }
  log_info("feature_num = " + std::to_string(feature_num));
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

#ifndef NETWORK_CONFIG_PROTO
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
    } else if (i > party_id){
      tmp_ports.push_back(ports[party_id] + i - 1);
      tmp_ports.push_back(ports[i] + party_id);
    }
    port_arrays.push_back(tmp_ports);
  }
#else
  // read network config proto from coordinator
  //    // decode the network base64 string to pb string
  //    vector<BYTE> network_file_byte = base64_decode(m_network_file);
  //    std::string network_pb_string(network_file_byte.begin(), network_file_byte.end());
  deserialize_network_configs(ips, port_arrays, executor_mpc_ports, m_network_file);
  log_info("mpc_port_array_size = " + std::to_string(executor_mpc_ports.size()));
  log_info("size of ip address = " + std::to_string(ips.size()));

  for (int i = 0; i < ips.size(); i++) {
    log_info("ips[" + std::to_string(i) + "] = " + ips[i]);
    for (int j = 0; j < port_arrays[i].size(); j++) {
      log_info("port_array[" + std::to_string(i) + "][" + std::to_string(j) + "] = " + std::to_string(port_arrays[i][j]));
    }
  }

  for (int i = 0; i < executor_mpc_ports.size(); i++) {
    log_info("executor_mpc_ports[" + std::to_string(i) + "] = " + std::to_string(executor_mpc_ports[i]));
  }
#endif

  host_names = ips;

  // establish communication connections
  log_info("Establish network communications with other parties");
  SocketPartyData me, other;
  for (int i = 0; i < party_num; ++i) {
    if (i != party_id) {
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]),
                           port_arrays[party_id][i]);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]),
                              port_arrays[i][party_id]);
#ifdef DEBUG
      log_info("My party id is: " + std::to_string(party_id) + ", I am listening party "
        + std::to_string(i) + " on port " + std::to_string(port_arrays[party_id][i]));
#endif
      shared_ptr<CommParty> channel =
          make_shared<CommPartyTCPSynced>(io_service, me, other);
#ifdef DEBUG
      log_info("The other party's port I will send data to is: " + std::to_string(port_arrays[i][party_id]));
#endif
      // connect to the other party and add channel
      channel->join(500, 5000);
#ifdef DEBUG
      log_info("Communication channel established with party " + std::to_string(i)
        + ", port is " + std::to_string(port_arrays[party_id][i]));
#endif
      channels.push_back(std::move(channel));
    } else {
      // self communication
      shared_ptr<CommParty> channel = nullptr;
      channels.push_back(std::move(channel));
    }
  }
}

void Party::init_phe_keys(bool m_use_existing_key, const std::string &m_key_file) {
  log_info("Init threshold partially homomorphic encryption keys");
  // init phe keys: if use existing key, read key file
  // otherwise, generate keys and broadcast to others
  phe_random = hcs_init_random();
  phe_pub_key = djcs_t_init_public_key();
  phe_auth_server = djcs_t_init_auth_server();
  log_info("m_use_existing_key = " + std::to_string(m_use_existing_key));
  if (m_use_existing_key) {
    init_with_key_file(m_key_file);
  } else {
    // the active party generates new phe keys and send to passive parties
    init_with_new_phe_keys(PHE_EPSILON, PHE_KEY_SIZE, party_num);
    log_info("Init with new phe keys finished");
    std::string phe_keys_str;
    serialize_phe_keys(phe_pub_key, phe_auth_server, phe_keys_str);
    write_key_to_file(phe_keys_str, m_key_file);
    log_info("Write phe keys finished");
  }
}

void Party::init_with_new_phe_keys(int epsilon, int phe_key_size,
                                   int required_party_num) {
  log_info("party type = " + std::to_string(party_type));
  if (party_type == falcon::ACTIVE_PARTY) {
    log_info("active party begins to generate phe keys...");
    // generate phe keys
    hcs_random* random = hcs_init_random();
    djcs_t_public_key* pub_key = djcs_t_init_public_key();
    djcs_t_private_key* priv_key = djcs_t_init_private_key();

    djcs_t_auth_server** auth_server =
        (djcs_t_auth_server**)malloc(party_num * sizeof(djcs_t_auth_server*));
    mpz_t* si = (mpz_t*)malloc(party_num * sizeof(mpz_t));
    djcs_t_generate_key_pair(pub_key, priv_key, random, epsilon, phe_key_size,
                             party_num, required_party_num);

    mpz_t* coeff = djcs_t_init_polynomial(priv_key, random);
    for (int i = 0; i < party_num; i++) {
      mpz_init(si[i]);
      djcs_t_compute_polynomial(priv_key, coeff, si[i], i);
      auth_server[i] = djcs_t_init_auth_server();
      djcs_t_set_auth_server(auth_server[i], si[i], i);
    }

    // serialize phe keys and send to passive parties
    for (int i = 0; i < party_num; i++) {
      if (i == party_id) {
        djcs_t_public_key_copy(pub_key, phe_pub_key);
        djcs_t_auth_server_copy(auth_server[i], phe_auth_server);
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
  } else {
    // for passive parties, receive the phe keys message from the active party
    // and set its own keys with the received message
    // TODO: now active party id is 0 by design, could abstract as a variable
    log_info("receive serialized keys from the active party and deserializing...");
    std::string recv_phe_keys_message;
    recv_long_message(0, recv_phe_keys_message);
    deserialize_phe_keys(phe_pub_key, phe_auth_server, recv_phe_keys_message);
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
  int training_data_size = (int) (sample_num * split_percentage);
  log_info("Split local data and labels into training and testing dataset.");
  log_info("Split percentage for train and test datasets = " + std::to_string(split_percentage));
  log_info("Training_data_size = " + std::to_string(training_data_size));

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
#ifdef DEBUG
    // print the shuffled data indexes for debug
    for (int i = 0; i < sample_num; i++) {
      log_info("shuffled data indexes[" + std::to_string(i) + "] = " + std::to_string(data_indexes[i]));
    }
#endif
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
  }

  // receive shuffled indexes and split in the same way
  else {
    std::string recv_shuffled_indexes_str;
    recv_long_message(0, recv_shuffled_indexes_str);
    deserialize_int_array(data_indexes, recv_shuffled_indexes_str);

    if (data_indexes.size() != sample_num) {
      log_error("Received size of shuffled indexes does not equal to sample num.");
      exit(EXIT_FAILURE);
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
