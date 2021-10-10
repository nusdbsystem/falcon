//
// Created by nai li xing on 11/08/21.
//

#include <falcon/network/Comm.hpp>
#include "falcon/distributed/worker.h"
#include <falcon/utils/pb_converter/network_converter.h>
#include <glog/logging.h>
#include <falcon/utils/base64.h>

Worker::Worker(const std::string& ps_network_config_pb_str, int worker_id){
    std::vector<std::string> worker_ips;
    std::vector< int > worker_ports;
    std::vector<std::string> ps_ips;
    std::vector<int> ps_ports;

    LOG(INFO) << "begin to deserialize the ps network configs";
    std::cout << "begin to deserialize the ps network configs" << std::endl;
    google::FlushLogFiles(google::INFO);

//    // 0. decode the network base64 string to pb string
//    vector<BYTE> ps_network_file_byte = base64_decode(ps_network_file);
//    std::string ps_network_pb_string(ps_network_file_byte.begin(), ps_network_file_byte.end());

    // 1. read network config proto from coordinator
    deserialize_ps_network_configs(
            worker_ips,worker_ports,
            ps_ips, ps_ports,
            ps_network_config_pb_str);
    LOG(INFO) << "Establish network communications with parameter server";

    for (int i = 0; i < worker_ips.size(); i++) {
      std::cout << "worker_ips[" << i << "] = " << worker_ips[i] << std::endl;
      LOG(INFO) << "worker_ips[" << i << "] = " << worker_ips[i];
      std::cout << "worker_ports[" << i << "] = " << worker_ports[i] << std::endl;
      LOG(INFO) << "worker_ports[" << i << "] = " << worker_ports[i];
      std::cout << "ps_ips[" << i << "] = " << ps_ips[i] << std::endl;
      LOG(INFO) << "ps_ips[" << i << "] = " << ps_ips[i];
      std::cout << "ps_ports[" << i << "] = " << ps_ports[i] << std::endl;
      LOG(INFO) << "ps_ports[" << i << "] = " << ps_ports[i];
    }
    google::FlushLogFiles(google::INFO);

    // 2. establish communication connections between worker and parameter server
    // current party instance is created at worker, only build channel with ps
    SocketPartyData me_worker, other_ps;
    me_worker = SocketPartyData(boost_ip::address::from_string(worker_ips[worker_id - 1]),
                                worker_ports[worker_id - 1]);
    other_ps = SocketPartyData(boost_ip::address::from_string(ps_ips[worker_id - 1]),
                               ps_ports[worker_id - 1]);
    ps_channel = make_shared<CommPartyTCPSynced>(io_service, me_worker, other_ps);

    // connect to the other party and add channel
    ps_channel->join(500, 5000);
    LOG(INFO) << "Communication channel established with ps " << ps_ips[worker_id - 1]
              << ", port is " << ps_ports[worker_id - 1] << ".";
}


Worker::~Worker() {
  io_service.stop();
  ps_channel = nullptr;
};

void Worker::send_long_message_to_ps(std::string message) const {
    ps_channel->writeWithSize(std::move(message));
}

void Worker::recv_long_message_from_ps(std::string& message) const {
    vector<byte> recv_message;
    ps_channel->readWithSizeIntoVector(recv_message);
    const byte* uc = &(recv_message[0]);
    string recv_message_str(reinterpret_cast<char const*>(uc),
                            recv_message.size());
    message = recv_message_str;
}


