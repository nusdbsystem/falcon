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
 * @return
 */
std::vector< std::vector<float> > read_dataset(const std::string& data_file);

/**
 * read pb serialized phe keys
 *
 * @param key_file
 * @return
 */
std::string read_key_file(const std::string& key_file);

#endif //FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_
