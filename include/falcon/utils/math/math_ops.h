#ifndef FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_
#define FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_

#include <vector>

/**
 * given two vectors, compute the mean squared error
 * (i.e., accuracy) in regression
 * https://peltarion.com/knowledge-center/documentation/glossary
 *
 * @param a
 * @param b
 * @param weights: if weights vector is given, compute weighted mse
 * @return
 */
double mean_squared_error(std::vector<double> a, std::vector<double> b,
                          const std::vector<double>& weights = std::vector<double>());

/**
 * given two vectors, compute the mean squared log error
 * (i.e., accuracy) in regression
 *  https://peltarion.com/knowledge-center/documentation/glossary
 *
 * @param a
 * @param b
 * @param weights: if weights vector is given, compute weighted mse
 * @return
 */
double mean_squared_log_error(std::vector<double> a, std::vector<double> b,
                              const std::vector<double>& weights = std::vector<double>());

/**
 * given two vectors, compute the median absolute error
 * (i.e., accuracy) in regression
 * https://peltarion.com/knowledge-center/documentation/glossary
 *
 * @param a
 * @param b
 * @return
 */
double median_absolute_error(std::vector<double> a, std::vector<double> b);

/**
 * given two vectors, compute the max error
 * (i.e., accuracy) in regression
 * https://peltarion.com/knowledge-center/documentation/glossary
 *
 * @param a
 * @param b
 * @return
 */
double max_error(std::vector<double> a, std::vector<double> b);

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
double mode(const std::vector<double>& inputs);

/**
 * find the median value of a vector.
 *  Given a vector ``V`` of length ``N``, the median of ``V`` is the
    middle value of a sorted copy of ``V``, ``V_sorted`` - i
    e., ``V_sorted[(N-1)/2]``, when ``N`` is odd, and the average of the
    two middle values of ``V_sorted`` when ``N`` is even.
 *
 * @param vec
 * @return
 */
double median(std::vector<double>& inputs);

/**
 * compute the average value of a vector
 *
 * @param inputs
 * @return
 */
double average(std::vector<double> inputs);

/**
 * compute the square sum of the two vectors
 *
 * @param a
 * @param b
 * @return
 */
double square_sum(std::vector<double> a, std::vector<double> b);

/**
 * find top k indexes
 *
 * @param a
 * @param k
 * @return
 */
std::vector<int> find_top_k_indexes(const std::vector<double>& a, int k);

#endif  // FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_
