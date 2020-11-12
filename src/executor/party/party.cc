//
// Created by wuyuncheng on 13/8/20.
//

#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <stack>

#include <glog/logging.h>

#include <falcon/operator/phe/djcs_t_aux.h>
#include <falcon/network/ConfigFile.hpp>
#include <falcon/party/party.h>
#include <falcon/utils/io_util.h>

Party::Party(){}

Party::Party(int m_party_id,
             int m_party_num,
             falcon::PartyType m_party_type,
             falcon::FLSetting m_fl_setting,
             const std::string& m_network_file,
             const std::string& m_data_file,
             bool m_use_existing_key,
             std::string m_key_file) {
  // copy params
  party_id = m_party_id;
  party_num = m_party_num;
  party_type = m_party_type;
  fl_setting = m_fl_setting;

  // read dataset
  local_data = read_dataset(m_data_file);
  sample_num = local_data.size();
  feature_num = local_data[0].size();

  // if this is active party, slice the last column as labels
  if (party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < sample_num; i++) {
      labels.push_back(local_data[i][feature_num-1]);
      local_data[i].pop_back();
    }
    feature_num = feature_num - 1;
  }

  // read network config file
  ConfigFile config_file(m_network_file);
  std::string port_string, ip_string;
  std::vector<int> ports(party_num);
  std::vector<std::string> ips(party_num);
  for (int i = 0; i < party_num; i++) {
    port_string = "party_" + to_string(i) + "_port";
    ip_string = "party_" + to_string(i) + "_ip";
    //get parties ips and ports
    ports[i] = stoi(config_file.Value("", port_string));
    ips[i] = config_file.Value("", ip_string);
  }

  // establish communication connections
  SocketPartyData me, other;
  for (int  i = 0;  i < party_num; ++ i) {
    if (i < party_id) {
      // this party will be the receiver in the protocol
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]), ports[party_id] + i);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + party_id - 1);
      shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

      // connect to the other party and add channel
      channel->join(500,5000);
      LOG(INFO) << "Communication channel established with party " << i << ", port is " << ports[party_id] + i << ".";
      channels.push_back(std::move(channel));
    } else if (i > party_id) {
      // this party will be the sender in the protocol
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]), ports[party_id] + i - 1);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + party_id);
      shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

      // connect to the other party and add channel
      channel->join(500,5000);
      LOG(INFO) << "Communication channel established with party " << i << ", port is " << ports[party_id] + i << ".";
      channels.push_back(std::move(channel));
    } else {
      // self communication
      shared_ptr<CommParty> channel = NULL;
      channels.push_back(std::move(channel));
    }
  }

  host_names = ips;

  /// init phe keys
  /// if use existing key, read key file
  /// otherwise, generate keys and broadcast to others
  if (m_use_existing_key) {

  } else {

  }
}

Party::~Party() {
  io_service.stop();
}

