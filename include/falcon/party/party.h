//
// Created by wuyuncheng on 13/8/20.
//

#ifndef FALCON_SRC_EXECUTOR_PARTY_PARTY_H_
#define FALCON_SRC_EXECUTOR_PARTY_PARTY_H_

#include <falcon/common.h>
#include <falcon/operator/phe/djcs_t_aux.h>
#include <libhcs.h>

#include <string>
#include <utility>
#include <vector>

#include "falcon/network/Comm.hpp"

class Party {
 public:
  // current party id
  int party_id;
  // total number of parties
  int party_num;
  // party type: active or passive
  falcon::PartyType party_type;
  // federated learning setting
  falcon::FLSetting fl_setting;
  // communication channel with other parties
  std::vector<shared_ptr<CommParty> > channels;
  // boost i/o functionality
  boost::asio::io_service io_service;
  // host names of other parties
  std::vector<std::string> host_names;
  // port array of mpc engines
  std::vector<int> executor_mpc_ports;
  // random generator of PHE
  hcs_random* phe_random;

 private:
  // sample number in the local dataset
  int sample_num;
  // feature number in the local dataset
  int feature_num;
  // 2-dimensional dataset (currently only support relational data)
  std::vector<std::vector<double> > local_data;
  // 1-dimensional label if the party has
  std::vector<double> labels;
  // public key of PHE
  djcs_t_public_key* phe_pub_key;
  // private key share (auth server) of PHE
  djcs_t_auth_server* phe_auth_server;

 public:
  /**
   * default constructor
   */
  Party();

  /**
   * party constructor
   *
   * @param m_party_id: current party id
   * @param m_party_num: total party number
   * @param m_party_type: party type (active or passive)
   * @param m_fl_setting: federated learning setting
   * @param ps_network_file: network configuration for distributed training
   * @param m_data_file: local dataset file
   */
  Party(int m_party_id, int m_party_num, falcon::PartyType m_party_type,
        falcon::FLSetting m_fl_setting, const std::string& m_data_file);

  /**
   * copy constructor
   * @param party
   */
  Party(const Party& party);

  /**
   * assignment constructor
   * @param party
   */
  Party& operator=(const Party& party);

  /**
   * destructor
   */
  ~Party();

  /**
   * initialize the network communication channels among parties
   *
   * @param m_network_file: parties network configuration
   */
  void init_network_channels(const std::string& m_network_file);

  /**
   * initialize the phe keys
   *
   * @param m_use_existing_key: whether use existing phe key
   * @param m_key_file: if true above, provide phe key file
   */
  void init_phe_keys(bool m_use_existing_key, const std::string& m_key_file);

  /**
   * when not use existing key file, generate and init with new keys
   *
   * @param epsilon: djcs_t cryptosystem level, default 1
   * @param phe_key_size: djcs_t key size
   * @param required_party_num: i.e., party num (additive secret sharing)
   */
  void init_with_new_phe_keys(int epsilon, int phe_key_size,
                              int required_party_num);

  /**
   * when use existing key file, deserialize and init phe keys
   *
   * @param key_file: file that stores the pb serialized phe keys
   */
  void init_with_key_file(const std::string& key_file);

  /**
   * export the phe key string for transmission
   *
   * @return serialized phe key string
   */
  std::string export_phe_key_string();

  /**
   * given the serialized phe key string, load and init party's keys
   *
   * @param phe_keys_str: serialized phe key string
   */
  void load_phe_key_string(const std::string& phe_keys_str);

  /**
   * send message via channel commParty
   *
   * @param id: other party id
   * @param message: sent message
   */
  void send_message(int id, std::string message) const;

  /**
   * send long message via channel commParty
   *
   * @param id: other party id
   * @param message: sent message
   */
  void send_long_message(int id, string message) const;

  /**
   * receive message from channel comm_party
   *
   * @param id: other party id
   * @param message: received message
   * @param buffer: received buffer
   * @param expected_size: buffer size
   */
  void recv_message(int id, std::string message, byte* buffer,
                    int expected_size) const;

  /**
   * receive message from channel comm_party
   *
   * @param id: other party id
   * @param message: received message
   */
  void recv_long_message(int id, std::string& message) const;

  /**
   * split the dataset into training and testing dataset
   *
   * @param split_percentage: #training_data ratio
   * @param training_data: returned training data
   * @param testing_data: returned testing data
   * @param training_labels: returned training labels
   * @param testing_labels: returned testing labels
   */
  void split_train_test_data(double split_percentage,
                             std::vector<std::vector<double> >& training_data,
                             std::vector<std::vector<double> >& testing_data,
                             std::vector<double>& training_labels,
                             std::vector<double>& testing_labels) const;

  /**
   * broadcast an encoded vector to other parties
   *
   * @param vec: the encoded vector to be broadcast
   * @param size: the size of the vector
   * @param req_party_id: the party who has the vector to be broadcast
   */
  void broadcast_encoded_number_array(EncodedNumber *vec,
                                      int size, int req_party_id) const;

  /**
   * each party has a int value, sync up to obtain a int array
   *
   * @param v: the value to be sync up
   * @return
   */
  std::vector<int> sync_up_int_arr(int v) const;

  /** set party's local sample number */
  void setter_sample_num(int s_sample_num) { sample_num = s_sample_num; }

  /** set party's local feature number */
  void setter_feature_num(int s_feature_num) { feature_num = s_feature_num; }

  /** set party's local dataset (support only relational data) */
  void setter_local_data(std::vector<std::vector<double> > s_local_data) {
    local_data = std::move(s_local_data);
  }

  /** set party's local labels if the party is an active party */
  void setter_labels(std::vector<double> s_labels) {
    labels = std::move(s_labels);
  }

  /** set party's phe public key */
  void setter_phe_pub_key(djcs_t_public_key* s_phe_pub_key) {
    djcs_t_public_key_copy(s_phe_pub_key, phe_pub_key);
  }

  /** set party's phe authenticate server (i.e., private key share) */
  void setter_phe_auth_server(djcs_t_auth_server* s_phe_auth_server) {
    djcs_t_auth_server_copy(s_phe_auth_server, phe_auth_server);
  }

  /** get party's local sample number */
  int getter_sample_num() const { return sample_num; }

  /** get party's local feature number */
  int getter_feature_num() const { return feature_num; }

  /** get party's local dataset (support only relational data) */
  std::vector<std::vector<double> > getter_local_data() const {
    return local_data;
  }

  /** get party's local labels if the party is an active party */
  std::vector<double> getter_labels() const { return labels; }

  /** get party's phe public key */
  void getter_phe_pub_key(djcs_t_public_key* g_phe_pub_key) const {
    djcs_t_public_key_copy(phe_pub_key, g_phe_pub_key);
  }

  /** get party's phe authenticate server (i.e., private key share) */
  void getter_phe_auth_server(djcs_t_auth_server* g_phe_auth_server) const {
    djcs_t_auth_server_copy(phe_auth_server, g_phe_auth_server);
  }
};

#endif  // FALCON_SRC_EXECUTOR_PARTY_PARTY_H_
