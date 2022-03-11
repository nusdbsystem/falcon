//
// Created by root on 3/11/22.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_ALG_TREE_UTIL_H_
#define FALCON_INCLUDE_FALCON_UTILS_ALG_TREE_UTIL_H_

#include "falcon/common.h"
#include <vector>

/**
 * compute the initial impurity for the root tree node
 * @param labels: the label vector
 * @param tree_type: whether classification or regression
 * @param class_num: the number of classes
 * @return
 */
double root_impurity(const std::vector<double>& labels, falcon::TreeType tree_type, int class_num);

#endif //FALCON_INCLUDE_FALCON_UTILS_ALG_TREE_UTIL_H_
