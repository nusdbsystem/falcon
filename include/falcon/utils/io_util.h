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

std::vector< std::vector<float> > read_dataset(const std::string& data_file);

#endif //FALCON_SRC_EXECUTOR_UTILS_IO_UTIL_H_
