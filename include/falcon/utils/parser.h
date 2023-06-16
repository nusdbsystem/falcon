//
// Created by root on 12/22/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PARSER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PARSER_H_

#include <falcon/common.h>
#include <iostream>

falcon::AlgorithmName parse_algorithm_name(const std::string &name);

falcon::SpdzMlpActivationFunc parse_mlp_act_func(const std::string &name);

#endif // FALCON_INCLUDE_FALCON_UTILS_PARSER_H_
