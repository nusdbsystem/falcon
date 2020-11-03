//
// Created by wuyuncheng on 13/8/20.
//

#include "party.h"
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <stack>

#include "../network/ConfigFile.hpp"

#include <glog/logging.h>

Party::Party(){}

Party::Party(int m_party_id, int m_party_num, std::string network_file) {
  // copy params
  party_id = m_party_id;
  party_num = m_party_num;

  // read network config file
  ConfigFile config_file(network_file);
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

  // establish connections
  SocketPartyData me, other;

  for (int  i = 0;  i < party_num; ++ i) {
    if (i < party_id) {
      // this party will be the receiver in the protocol
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]), ports[party_id] + i);
      //logger(logger_out, "my port = %d\n", ports[party_id] + i);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + party_id - 1);
      //logger(logger_out, "other port = %d\n", ports[i] + party_id - 1);
      shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

      // connect to the other party
      channel->join(500,5000);
      //logger(logger_out, "channel established\n");
      LOG(INFO) << "channel established.";

      // add channel to the other client
      channels.push_back(std::move(channel));
    } else if (i > party_id) {
      // this party will be the sender in the protocol
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]), ports[party_id] + i - 1);
      //logger(logger_out, "my port = %d\n", ports[party_id] + i - 1);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + party_id);
      //logger(logger_out, "other port = %d\n", ports[i] + party_id);
      shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

      // connect to the other party
      channel->join(500,5000);
      //logger(logger_out, "channel established\n");
      LOG(INFO) << "channel established.";

      // add channel to the other client
      channels.push_back(std::move(channel));
    } else {
      // self communication
      shared_ptr<CommParty> channel = NULL;
      channels.push_back(std::move(channel));
    }
  }
}

Party::~Party() {
  io_service.stop();
}