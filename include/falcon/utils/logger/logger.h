//
// Created by root on 10/13/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_LOGGER_LOGGER_H_
#define FALCON_INCLUDE_FALCON_UTILS_LOGGER_LOGGER_H_

#include <glog/logging.h>

/**
 * print the log info string
 *
 * @param str: the message to be logged
 */
void log_info(const std::string& str);

/**
 * print the log error string
 *
 * @param str: the message to be logged
 */
void log_error(const std::string& str);

#endif //FALCON_INCLUDE_FALCON_UTILS_LOGGER_LOGGER_H_
