//
// Created by wuyuncheng on 15/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_
#define FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_

#include <vector>
#include <string>

/**
 * serialize int array proto message
 *
 * @param vec: input int vector
 * @param output_message: serialized string
 */
void serialize_int_array(std::vector<int> vec, std::string& output_message);

/**
 * deserialize int array proto message
 *
 * @param vec: deserialized int vector
 * @param input_message: serialized string
 */
void deserialize_int_array(std::vector<int>& vec, std::string input_message);

#endif //FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_
