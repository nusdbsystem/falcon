//
// Created by wuyuncheng on 15/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_
#define FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_

#include <vector>
#include <string>
#include <falcon/operator/phe/fixed_point_encoder.h>

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
void deserialize_int_array(std::vector<int>& vec, const std::string& input_message);

/**
 * serialize encoded number
 *
 * @param number: an EncodedNumber
 * @param output_message: serialized string
 */
void serialize_encoded_number(EncodedNumber number, std::string& output_message);

/**
 * deserialize encoded number
 *
 * @param number: an EncodedNumber
 * @param input_message: serialized string
 */
void deserialize_encoded_number(EncodedNumber& number, const std::string& input_message);

/**
 * serialize encoded number array
 *
 * @param number_array: an EncodedNumber array
 * @param size: size of array
 * @param output_message: serialized string
 */
void serialize_encoded_number_array(EncodedNumber* number_array, int size, std::string& output_message);

/**
 * deserialize encoded number array
 *
 * @param number_array: an EncodedNumber array
 * @param size: size of array (need to specify before deserialization)
 * @param input_message: serialized string
 */
void deserialize_encoded_number_array(EncodedNumber* number_array, int size, const std::string& input_message);

#endif //FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_
