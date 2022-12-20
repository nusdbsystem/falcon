//
// Created by wuyuncheng on 13/12/20.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_NETWORK_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_NETWORK_CONVERTER_H_

#include <string>
#include <vector>

/**
 * serialize the network configs
 *
 * @param ips: ip vector of parties
 * @param executor_executor_port_arrays: port arrays of executors to executors
 * @param executor_mpc_port_array: ports of executor to mpc
 * @param output_message: serialized message
 */
void serialize_network_configs(std::vector<std::string> ips,
                               const std::vector<std::vector<int> > &executor_executor_port_arrays,
                               const std::vector<int> &executor_mpc_port_array,
                               std::string &output_message);

/**
 * deserialize the network configs
 *
 * @param ips: deserialized ip vector of parties
 * @param executor_executor_port_arrays: deserialized port arrays of executors
 * @param executor_mpc_port_array: deserialized ports for executors to mpc
 * @param input_message: serialized message
 */
void deserialize_network_configs(std::vector<std::string> &ips,
                                 std::vector<std::vector<int> > &executor_executor_port_arrays,
                                 std::vector<int> &executor_mpc_port_array,
                                 const std::string &input_message);

/**
 * serialize network configs between ps and workers
 *
 * @param worker_ips: ip vector of workers
 * @param worker_ports: port vector of workers
 * @param ps_ips: ip vector of ps, should be the same
 * @param ps_ports: port vector of ps
 * @param output_message: serialized message
 */
void serialize_ps_network_configs(
    const std::vector<std::string> &worker_ips,
    const std::vector<int> &worker_ports,
    const std::vector<std::string> &ps_ips,
    const std::vector<int> &ps_ports,
    std::string &output_message);

/**
 * deserialize the network configs between ps and workers
 *
 * @param worker_ips: deserialized ip vector of workers
 * @param worker_ports: deserialized port vector of workers
 * @param ps_ips: deserialized ip vector of ps, should be the same
 * @param ps_ports: deserialized port vector of ps
 * @param input_message: serialized message
 */
void deserialize_ps_network_configs(
    std::vector<std::string> &worker_ips,
    std::vector<int> &worker_ports,
    std::vector<std::string> &ps_ips,
    std::vector<int> &ps_ports,
    const std::string &input_message);

/**
 * Get number of worker from ps
 * @param input_message
 * @return
 */
int retrieve_worker_size_from_ps_network_configs(const std::string &input_message);

#endif //FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_NETWORK_CONVERTER_H_
