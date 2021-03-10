//
// Created by wuyuncheng on 11/12/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_METRIC_ACCURACY_H_
#define FALCON_SRC_EXECUTOR_UTILS_METRIC_ACCURACY_H_

#include <vector>


/**
 * compute the fraction of the matched elements
 *
 * @param predictions: first vector
 * @param labels: second vector
 * @return
 */
float accuracy_computation(std::vector<int> predictions, std::vector<float> labels);


#endif //FALCON_SRC_EXECUTOR_UTILS_METRIC_ACCURACY_H_
