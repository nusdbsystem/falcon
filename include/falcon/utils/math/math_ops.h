#ifndef FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_
#define FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_

#include <vector>

/**
 * given two vectors, compute the mean squared error (i.e., accuracy) in regression
 *
 * @param a
 * @param b
 * @return
 */
float mean_squared_error(std::vector<float> a, std::vector<float> b);

/**
 * given two values, compare whether they are same within some accuracy
 * TO compensate the accuracy loss of float values from SPDZ
 *
 * @param a
 * @param b
 * @return
 */
bool rounded_comparison(float a, float b);

/**
 * given an input float vector, compute the softmax probability distribution
 * FOR classification in GBDT
 *
 * @param inputs
 * @return
 */
std::vector<float> softmax(std::vector<float> inputs);


/**
 * return the argmax index of an input vector
 *
 * @param inputs
 * @return
 */
float argmax(std::vector<float> inputs);


/**
 * compute the logistic function for logit
 * logistic function (sigmoid) for estimated probability
 *
 * @param logit: logit score
 * @return
 */
float logistic_function(float logit);

#endif //FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_
