//
// Created by wuyuncheng on 15/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_
#define FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <string>
#include <vector>

/**
 * serialize int array proto message
 *
 * @param vec: input int vector
 * @param output_message: serialized string
 */
void serialize_int_array(std::vector<int> vec, std::string &output_message);

/**
 * deserialize int array proto message
 *
 * @param vec: deserialized int vector
 * @param input_message: serialized string
 */
void deserialize_int_array(std::vector<int> &vec,
                           const std::string &input_message);

/**
 * serialize double array proto message
 *
 * @param vec: input double vector
 * @param output_message: serialized string
 */
void serialize_double_array(std::vector<double> vec,
                            std::string &output_message);

/**
 * deserialize double array proto message
 *
 * @param vec: deserialized double vector
 * @param input_message: serialized string
 */
void deserialize_double_array(std::vector<double> &vec,
                              const std::string &input_message);

/**
 * serialize double matrix proto message
 *
 * @param mat: input double matrix
 * @param output_message: serialized string
 */
void serialize_double_matrix(std::vector<std::vector<double>> mat,
                             std::string &output_message);

/**
 * deserialize double matrix proto message
 *
 * @param mat: deserialized double matrix
 * @param input_message: serialized string
 */
void deserialize_double_matrix(std::vector<std::vector<double>> &mat,
                               const std::string &input_message);

/**
 * serialize encoded number
 *
 * @param number: an EncodedNumber
 * @param output_message: serialized string
 */
void serialize_encoded_number(const EncodedNumber &number,
                              std::string &output_message);

/**
 * deserialize encoded number
 *
 * @param number: an EncodedNumber
 * @param input_message: serialized string
 */
void deserialize_encoded_number(EncodedNumber &number,
                                const std::string &input_message);

/**
 * serialize encoded number array
 *
 * @param number_array: an EncodedNumber array
 * @param size: size of array
 * @param output_message: serialized string
 */
void serialize_encoded_number_array(EncodedNumber *number_array, int size,
                                    std::string &output_message);

/**
 * deserialize encoded number array
 *
 * @param number_array: an EncodedNumber array
 * @param size: size of array (need to specify before deserialization)
 * @param input_message: serialized string
 */
void deserialize_encoded_number_array(EncodedNumber *number_array, int size,
                                      const std::string &input_message);

/**
 * serialize encoded number matrix
 *
 * @param number_matrix: an EncodedNumber matrix
 * @param row_size: row number
 * @param column_size: column number
 * @param output_message: serialized string
 */
void serialize_encoded_number_matrix(EncodedNumber **number_matrix,
                                     int row_size, int column_size,
                                     std::string &output_message);

/**
 * deserialize encoded number array
 *
 * @param number_array: an EncodedNumber array
 * @param size: size of array (need to specify before deserialization)
 * @param input_message: serialized string
 */
void deserialize_encoded_number_matrix(EncodedNumber **number_matrix,
                                       int row_size, int column_size,
                                       const std::string &input_message);

#endif // FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_COMMON_CONVERTER_H_
