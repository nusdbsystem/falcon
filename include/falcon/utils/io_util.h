//
// Created by wuyuncheng on 12/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_
#define FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glog/logging.h>

/**
 * read dataset as 2d-float vectors
 *
 * @param data_file
 * @param delimiter
 * @return
 */
std::vector< std::vector<double> > read_dataset(const std::string& data_file, char delimiter);

/**
 * write dataset to file
 *
 * @param data: data to write
 * @param delimiter
 * @param data_file: file to write
 */
void write_dataset_to_file(std::vector< std::vector<double> > data, char delimiter, const std::string& data_file);

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

#endif //FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_
