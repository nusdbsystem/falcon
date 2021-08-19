//
// Created by wuyuncheng on 13/12/20.
//

#include "../../include/message/network.pb.h"
#include <falcon/utils/pb_converter/network_converter.h>

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

void serialize_network_configs(std::vector<std::string> ips,
    std::vector< std::vector<int> > port_arrays,
    std::string& output_message) {
  if (ips.size() != port_arrays.size()) {
    LOG(ERROR) << "Serialize network config failed:"
                  " the number of ip addresses and port arrays do not match";
    return;
  }
  com::nus::dbsytem::falcon::v0::NetworkConfig network_config;
  for (int i = 0; i < ips.size(); i++) {
    network_config.add_ips(ips[i]);
    com::nus::dbsytem::falcon::v0::PortArray *port_array = network_config.add_port_arrays();
    for (int j = 0; j < port_arrays[0].size(); j++) {
      port_array->add_ports(port_arrays[i][j]);
    }
  }
  network_config.SerializeToString(&output_message);
  network_config.Clear();
}

void deserialize_network_configs(std::vector<std::string>& ips,
    std::vector< std::vector<int> >& port_arrays,
    const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::NetworkConfig network_config;
  if (!network_config.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize NetworkConfig message failed.";
    return;
  }
  int ips_size = network_config.ips_size();
  int port_array_size = network_config.port_arrays_size();
  if (ips_size != port_array_size) {
    LOG(ERROR) << "Deserialize network config failed: "
                  "the number of ip address and port arrays do not match";
    return;
  }
  for (int i = 0; i < ips_size; i++) {
    ips.push_back(network_config.ips(i));
    std::vector<int> port_vec_i;
    const com::nus::dbsytem::falcon::v0::PortArray& port_array = network_config.port_arrays(i);
    int port_array_i_size = port_array.ports_size();
    port_vec_i.reserve(port_array_i_size);
    for (int j = 0; j < port_array_i_size; j++) {
      port_vec_i.push_back(port_array.ports(j));
    }
    port_arrays.push_back(port_vec_i);
  }
}
