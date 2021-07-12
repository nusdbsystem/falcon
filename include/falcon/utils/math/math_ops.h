#ifndef FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_
#define FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_

#include <vector>

/**
 * given two vectors, compute the mean squared error
 * (i.e., accuracy) in regression
 *
 * @param a
 * @param b
 * @return
 */
double mean_squared_error(std::vector<double> a, std::vector<double> b);

/**
 * given two values, compare whether they are same within some accuracy
 * TO compensate the accuracy loss of double values from SPDZ
 *
 * @param a
 * @param b
 * @return
 */
bool rounded_comparison(double a, double b);

/**
 * given an input double vector, compute the softmax probability distribution
 * FOR classification in GBDT
 *
 * @param inputs
 * @return
 */
std::vector<double> softmax(std::vector<double> inputs);

/**
 * return the argmax index of an input vector
 *
 * @param inputs
 * @return
 */
double argmax(std::vector<double> inputs);

/**
 * compute the logistic function for logit
 * logistic function (sigmoid) for estimated probability
 *
 * @param logit: logit score
 * @return
 */
double logistic_function(double logit);

/**
 * compute the loss function of the binary logistic regression model
 *
 * @param pred_probs: predicted probabilities
 * @param labels: ground truth labels
 * @return
 */
double logistic_regression_loss(std::vector<double> pred_probs,
                                std::vector<double> labels);

/**
 * compute the mode value of a vector
 *
 * @param inputs
 * @return
 */
double mode(std::vector<double> inputs);

/**
 * compute the average value of a vector
 *
 * @param inputs
 * @return
 */
double average(std::vector<double> inputs);

#endif  // FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_
