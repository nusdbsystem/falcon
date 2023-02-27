/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by wuyuncheng on 13/12/20.
//

#include "../../include/message/network.pb.h"
#include "../../include/message/ps_network.pb.h"
#include <falcon/utils/pb_converter/network_converter.h>

#include <falcon/utils/logger/logger.h>
#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>
#include <iostream>

void serialize_network_configs(
    std::vector<std::string> ips,
    const std::vector<std::vector<int>> &executor_executor_port_arrays,
    const std::vector<int> &executor_mpc_port_array,
    std::string &output_message) {
  if ((ips.size() != executor_executor_port_arrays.size()) ||
      (ips.size() != executor_mpc_port_array.size())) {
    log_error("Serialize network config failed:"
              " the number of ip addresses and port arrays do not match");
    exit(EXIT_FAILURE);
  }
  com::nus::dbsytem::falcon::v0::NetworkConfig network_config;
  for (int i = 0; i < ips.size(); i++) {
    network_config.add_ips(ips[i]);
    com::nus::dbsytem::falcon::v0::PortArray *executors_port_array =
        network_config.add_executor_executor_port_arrays();
    for (int j = 0; j < executor_executor_port_arrays[0].size(); j++) {
      executors_port_array->add_ports(executor_executor_port_arrays[i][j]);
    }
  }
  auto *mpc_port_array = new com::nus::dbsytem::falcon::v0::PortArray;
  for (int i = 0; i < executor_executor_port_arrays.size(); i++) {
    mpc_port_array->add_ports(executor_mpc_port_array[i]);
  }
  network_config.set_allocated_executor_mpc_port_array(mpc_port_array);
  network_config.SerializeToString(&output_message);
  network_config.Clear();
}

void deserialize_network_configs(
    std::vector<std::string> &ips,
    std::vector<std::vector<int>> &executor_executor_port_arrays,
    std::vector<int> &executor_mpc_port_array,
    const std::string &input_message) {
  com::nus::dbsytem::falcon::v0::NetworkConfig network_config;
  if (!network_config.ParseFromString(input_message)) {
    log_error("Deserialize NetworkConfig message failed.");
    exit(EXIT_FAILURE);
  }
  int ips_size = network_config.ips_size();
  int executors_port_array_size =
      network_config.executor_executor_port_arrays_size();
  const com::nus::dbsytem::falcon::v0::PortArray &mpc_port_array =
      network_config.executor_mpc_port_array();
  int mpc_port_array_size = mpc_port_array.ports_size();

  if ((ips_size != executors_port_array_size) ||
      (ips_size != mpc_port_array_size)) {
    log_error("Deserialize network config failed: "
              "the number of ip address and port arrays do not match");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < ips_size; i++) {
    ips.push_back(network_config.ips(i));
    std::vector<int> port_vec_i;
    const com::nus::dbsytem::falcon::v0::PortArray &port_array =
        network_config.executor_executor_port_arrays(i);
    int port_array_i_size = port_array.ports_size();
    port_vec_i.reserve(port_array_i_size);
    for (int j = 0; j < port_array_i_size; j++) {
      port_vec_i.push_back(port_array.ports(j));
    }
    executor_executor_port_arrays.push_back(port_vec_i);
    executor_mpc_port_array.push_back(mpc_port_array.ports(i));
  }
}

void serialize_ps_network_configs(const std::vector<std::string> &worker_ips,
                                  const std::vector<int> &worker_ports,
                                  const std::vector<std::string> &ps_ips,
                                  const std::vector<int> &ps_ports,
                                  std::string &output_message) {
  if ((worker_ips.size() != worker_ports.size()) ||
      (ps_ips.size() != ps_ports.size()) ||
      (worker_ips.size() != ps_ips.size())) {
    log_error("Serialize ps network config failed:"
              " the ip or port arrays do not match");
    exit(EXIT_FAILURE);
  }
  com::nus::dbsytem::falcon::v0::PSNetworkConfig ps_network_config;
  for (int i = 0; i < worker_ips.size(); i++) {
    com::nus::dbsytem::falcon::v0::Worker *worker =
        ps_network_config.add_workers();
    com::nus::dbsytem::falcon::v0::PS *ps = ps_network_config.add_ps();
    worker->set_worker_ip(worker_ips[i]);
    worker->set_worker_port(worker_ports[i]);
    ps->set_ps_ip(ps_ips[i]);
    ps->set_ps_port(ps_ports[i]);
  }
  ps_network_config.SerializeToString(&output_message);
}

void deserialize_ps_network_configs(std::vector<std::string> &worker_ips,
                                    std::vector<int> &worker_ports,
                                    std::vector<std::string> &ps_ips,
                                    std::vector<int> &ps_ports,
                                    const std::string &input_message) {

  com::nus::dbsytem::falcon::v0::PSNetworkConfig ps_network_config;
  if (!ps_network_config.ParseFromString(input_message)) {
    log_error("Deserialize NetworkConfig message failed.");
    exit(EXIT_FAILURE);
  }
  int workers_size = ps_network_config.workers_size();
  int ps_size = ps_network_config.ps_size();

  // worker size should equal to ps size
  if (workers_size != ps_size) {
    log_error("PS NetworkConfig worker size does not match ps size.");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < workers_size; i++) {
    // get follower ip
    worker_ips.push_back(ps_network_config.workers(i).worker_ip());
    // get follower ports, index0:send port, index1:receive port
    worker_ports.push_back(ps_network_config.workers(i).worker_port());
    // get PS ip
    ps_ips.push_back(ps_network_config.ps(i).ps_ip());
    // get PS ports
    ps_ports.push_back(ps_network_config.ps(i).ps_port());
  }
}

int retrieve_worker_size_from_ps_network_configs(
    const std::string &input_message) {
  com::nus::dbsytem::falcon::v0::PSNetworkConfig ps_network_config;
  if (!ps_network_config.ParseFromString(input_message)) {
    log_error("Deserialize NetworkConfig message failed.");
    exit(EXIT_FAILURE);
  }
  int workers_size = ps_network_config.workers_size();
  return workers_size;
}
