//
// Created by wuyuncheng on 13/8/20.
//

#ifndef FALCON_SRC_EXECUTOR_PARTY_PARTY_H_
#define FALCON_SRC_EXECUTOR_PARTY_PARTY_H_

#include "falcon/network/Comm.hpp"
#include <falcon/operator/phe/djcs_t_aux.h>
#include <falcon/common.h>

#include <string>
#include <utility>
#include <vector>
#include <libhcs.h>


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
  std::vector< shared_ptr<CommParty> > channels;
  // boost i/o functionality
  boost::asio::io_service io_service;
  // host names of other parties
  std::vector<std::string> host_names;
  // random generator of PHE
  hcs_random* phe_random;

 private:
  // sample number in the local dataset
  int sample_num;
  // feature number in the local dataset
  int feature_num;
  // 2-dimensional dataset (currently only support relational data)
  std::vector< std::vector <double> > local_data;
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
   * @param m_network_file: parties network configuration
   * @param m_data_file: local dataset file
   * @param m_use_existing_key: whether use existing phe key
   * @param m_key_file: if true above, provide phe key file
   */
  Party(int m_party_id,
      int m_party_num,
      falcon::PartyType m_party_type,
      falcon::FLSetting m_fl_setting,
      const std::string& m_network_file,
      const std::string& m_data_file,
      bool m_use_existing_key,
      const std::string& m_key_file);

  /**
   * copy constructor
   * @param party
   */
  Party(const Party& party);

  /**
   * assignment constructor
   * @param party
   */
  Party& operator = (const Party &party);

  /**
   * destructor
   */
  ~Party();

  /**
   * when not use existing key file, generate and init with new keys
   *
   * @param epsilon: djcs_t cryptosystem level, default 1
   * @param phe_key_size: djcs_t key size
   * @param required_party_num: i.e., party num (additive secret sharing)
   */
  void init_with_new_phe_keys(int epsilon, int phe_key_size, int required_party_num);

  /**
   * when use existing key file, deserialize and init phe keys
   *
   * @param key_file: file that stores the pb serialized phe keys
   */
  void init_with_key_file(const std::string& key_file);

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
  void send_long_message (int id, string message) const;

  /**
   * receive message from channel comm_party
   *
   * @param id: other party id
   * @param message: received message
   * @param buffer: received buffer
   * @param expected_size: buffer size
   */
  void recv_message(int id, std::string message, byte* buffer, int expected_size) const;

  /**
   * receive message from channel comm_party
   *
   * @param id: other party id
   * @param message: received message
   */
  void recv_long_message (int id, std::string & message) const;

  /**
   * split local data and labels into train-test
   *
   * @param split_percentage: percentage of training data
   * @param training_data: split training dataset
   * @param testing_data: split testing dataset
   * @param training_labels: split training labels for active party
   * @param testing_labels: split testing labels for active party
   */
  void split_train_test_data(double split_percentage,
      std::vector< std::vector<double> >& training_data,
      std::vector< std::vector<double> >& testing_data,
      std::vector<double>& training_labels,
      std::vector<double>& testing_labels) const;

  /**
   * parties jointly decrypt a ciphertext vector,
   * assume that the parties have already have the same src_ciphers.
   * The request party obtains the decrypted plaintext while
   * the other parties obtain nothing plaintext.
   *
   * @param src_ciphers: ciphertext vector to be decrypted
   * @param dest_plains: decrypted plaintext vector
   * @param size: size of the vector
   * @param req_party_id: party that initiate decryption
   */
  void collaborative_decrypt(EncodedNumber* src_ciphers,
      EncodedNumber* dest_plains,
      int size, int req_party_id) const;

  /**
   * convert ciphertext vector to secret shares securely,
   * Algorithm 1: Conversion to secretly shared value in paper
   * <Privacy Preserving Vertical Federated Learning for Tree-based Models>
   *
   * @param src_ciphers: ciphertext vector to be decrypted
   * @param secret_shares: decrypted and decoded secret shares
   * @param size: size of the vector
   * @param req_party_id: party that initiate decryption
   * @param phe_precision: fixed point precision when encoding
   */
  void ciphers_to_secret_shares(EncodedNumber* src_ciphers,
      std::vector<double>& secret_shares,
      int size, int req_party_id,
      int phe_precision);

  /**
   * convert secret shares back to ciphertext vector
   *
   * @param dest_ciphers: ciphertext vector to be recovered
   * @param secret_shares: secret shares received from spdz parties
   * @param size: size of the vector
   * @param req_party_id: party that initiate conversion
   * @param phe_precision: ciphertext vector precision, need careful design
   */
  void secret_shares_to_ciphers(EncodedNumber* dest_ciphers,
      std::vector<double> secret_shares,
      int size, int req_party_id,
      int phe_precision);

  /** set party's local sample number */
  void setter_sample_num(int s_sample_num) { sample_num = s_sample_num; }

  /** set party's local feature number */
  void setter_feature_num(int s_feature_num) { feature_num = s_feature_num; }

  /** set party's local dataset (support only relational data) */
  void setter_local_data(std::vector< std::vector<double> > s_local_data) {
    local_data = std::move(s_local_data);
  }

  /** set party's local labels if the party is an active party */
  void setter_labels(std::vector<double> s_labels) { labels = std::move(s_labels); }

  /** set party's phe public key */
  void setter_phe_pub_key(djcs_t_public_key* s_phe_pub_key) {
    djcs_t_public_key_copy(s_phe_pub_key, phe_pub_key);
  }

  /** set party's phe authenticate server (i.e., private key share) */
  void setter_phe_auth_server(djcs_t_auth_server* s_phe_auth_server) {
    djcs_t_auth_server_copy(s_phe_auth_server, phe_auth_server);
  }

  /** set party's phe hcs random generator */
  void setter_phe_random(hcs_random* s_phe_random) {
    djcs_t_hcs_random_copy(s_phe_random, phe_random);
  }

  /** get party's local sample number */
  int getter_sample_num() const { return sample_num; }

  /** get party's local feature number */
  int getter_feature_num() const { return feature_num; }

  /** get party's local dataset (support only relational data) */
  std::vector< std::vector<double> > getter_local_data() const { return local_data; }

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

  /** get party's phe hcs random generator */
  void getter_phe_random(hcs_random* g_phe_random) const {
    djcs_t_hcs_random_copy(phe_random, g_phe_random);
  }
};

#endif //FALCON_SRC_EXECUTOR_PARTY_PARTY_H_
