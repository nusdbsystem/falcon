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
 * @param port_arrays: port arrays of each party
 * @param output_message: serialized message
 */
void serialize_network_configs(std::vector<std::string> ips,
    std::vector< std::vector<int> > port_arrays,
    std::string& output_message);

/**
 * deserialize the network configs
 *
 * @param ips: deserialized ip vector of parties
 * @param port_arrays: deserialized port arrays of each party
 * @param input_message: serialized message
 */
void deserialize_network_configs(std::vector<std::string>& ips,
    std::vector< std::vector<int> >& port_arrays,
    const std::string& input_message);

#endif //FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_NETWORK_CONVERTER_H_
