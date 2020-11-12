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

 private:
  // sample number in the local dataset
  int sample_num;
  // feature number in the local dataset
  int feature_num;
  // 2-dimensional dataset (currently only support relational data)
  std::vector< std::vector <float> > local_data;
  // 1-dimensional label if the party has
  std::vector<float> labels;
  // public key of PHE
  djcs_t_public_key* phe_pub_key;
  // private key share (auth server) of PHE
  djcs_t_auth_server* phe_auth_server;
  // random generator of PHE
  hcs_random* phe_random;

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
      std::string m_key_file);

  ~Party();


  /** set party's local sample number */
  void setter_sample_num(int s_sample_num) { sample_num = s_sample_num; }

  /** set party's local feature number */
  void setter_feature_num(int s_feature_num) { feature_num = s_feature_num; }

  /** set party's local dataset (support only relational data) */
  void setter_local_data(std::vector< std::vector<float> > s_local_data) {
    local_data = std::move(s_local_data);
  }

  /** set party's local labels if the party is an active party */
  void setter_labels(std::vector<float> s_labels) { labels = std::move(s_labels); }

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
  std::vector< std::vector<float> > getter_local_data() const { return local_data; }

  /** get party's local labels if the party is an active party */
  std::vector<float> getter_labels() const { return labels; }

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
