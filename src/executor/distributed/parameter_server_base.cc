//
// Created by 04/07/21.
//

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/utils/pb_converter/network_converter.h>
#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>


ParameterServer::ParameterServer(const std::string& ps_network_config_pb_str){
    std::vector<std::string> worker_ips;
    std::vector< int > worker_ports;
    std::vector<std::string> ps_ips;
    std::vector<int> ps_ports;

    // 1. read network config proto from coordinator
    deserialize_ps_network_configs(
            worker_ips,worker_ports,
            ps_ips, ps_ports,
            ps_network_config_pb_str);
    log_info("Establish network communications with workers");
    // establish communication connections between workers and parameter server
    // current party instance is created at parameter server, build channel with workers
    for (int wk_index = 0; wk_index < worker_ips.size(); wk_index++){
      SocketPartyData me_ps, other_worker;
      me_ps = SocketPartyData(boost_ip::address::from_string(ps_ips[wk_index]),
                                ps_ports[wk_index]);
      other_worker = SocketPartyData(boost_ip::address::from_string(worker_ips[wk_index]),
                                       worker_ports[wk_index]);
      shared_ptr<CommParty> tmp_channel = make_shared<CommPartyTCPSynced>(io_service, me_ps, other_worker);

      // connect to the other party and add channel
      tmp_channel->join(500, 5000);
      worker_channels.push_back(tmp_channel);

      log_info("Communication channel established with worker index "
        + std::to_string(wk_index) + ", ip is " + worker_ips[wk_index]
        + ", port is " + std::to_string(worker_ports[wk_index]));
    }
}

ParameterServer::~ParameterServer()= default;

void ParameterServer::send_long_message_to_worker(int id,std::string message) const {
    worker_channels[id]->writeWithSize(std::move(message));
}

void ParameterServer::recv_long_message_from_worker(int id, std::string& message) const {
    vector<byte> recv_message;
    worker_channels[id]->readWithSizeIntoVector(recv_message);
    const byte* uc = &(recv_message[0]);
    string recv_message_str(reinterpret_cast<char const*>(uc),
                            recv_message.size());
    message = recv_message_str;
}