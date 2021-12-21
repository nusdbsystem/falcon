//
// Created by wuyuncheng on 12/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_
#define FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_

#include <falcon/operator/phe/fixed_point_encoder.h>

#include <glog/logging.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

/**
 * read dataset as 2d-float vectors
 *
 * @param data_file
 * @param delimiter
 * @return
 */
std::vector<std::vector<double> > read_dataset(const std::string& data_file,
                                               char delimiter);

/**
 * write dataset to file
 *
 * @param data: data to write
 * @param delimiter
 * @param data_file: file to write
 */
void write_dataset_to_file(std::vector<std::vector<double> > data,
                           char delimiter, const std::string& data_file);

// for Party::split_train_test_data
// save a copy of shuffled data_indexes vector<int> to file for local debugging
void write_shuffled_data_indexes_to_file(std::vector<int> data_indexes,
                                         const std::string& data_file);

/**
 * read pb serialized phe keys
 *
 * @param key_file
 * @return
 */
std::string read_key_file(const std::string& key_file);

/**
 * write pb serialized phe keys
 *
 * @param phe_keys_str: serialized key string
 * @param key_file: file to write
 */
void write_key_to_file(std::string phe_keys_str, const std::string& key_file);

/**
 * read an encoded number data matrix from the file
 *
 * @param data_matrix: the returned encoded numbers
 * @param row_num: the number of rows
 * @param column_num: the number of columns
 * @param encoded_number_file: the file to be read
 */
void read_encoded_number_matrix_file(
    EncodedNumber** data_matrix,
    int row_num,
    int column_num,
    const std::string& encoded_number_file
);

/**
 * write an encoded number data matrix to the file
 *
 * @param data_matrix: the encoded numbers
 * @param row_num: the number of rows
 * @param column_num: the number of columns
 * @param encoded_number_file: the file to be written
 */
void write_encoded_number_matrix_to_file(
    EncodedNumber** data_matrix,
    int row_num,
    int column_num,
    const std::string& encoded_number_file
);

/**
 * read an encoded number data array from the file
 *
 * @param data_matrix: the returned encoded numbers
 * @param row_num: the number of rows
 * @param encoded_number_file: the file to be read
 */
void read_encoded_number_array_file(
    EncodedNumber* data_arr,
    int row_num,
    const std::string& encoded_number_file
);

/**
 * write an encoded number data array to the file
 *
 * @param data_arr: the encoded numbers
 * @param row_num: the number of rows
 * @param encoded_number_file: the file to be written
 */
void write_encoded_number_array_to_file(
    EncodedNumber* data_arr,
    int row_num,
    const std::string& encoded_number_file
);

/**
 * convert a number to string for output
 * @tparam T
 * @param Number
 * @return
 */
template <typename T>
std::string NumberToString ( T Number )
{
  std::ostringstream ss;
  ss << Number;
  return ss.str();
}

/**
 * print a vector
 * @tparam T
 * @return
 */
template <typename T>
inline void print_vector(const std::vector<T>& vec){
  std::cout << "[ ";
  for( const auto& ele: vec){
    std::cout << ele <<" ";
  }
  std::cout << "]" << std::endl;
}

#endif //FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_
