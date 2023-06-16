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
double
mean_squared_error(std::vector<double> a, std::vector<double> b,
                   const std::vector<double> &weights = std::vector<double>());

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
double mean_squared_log_error(
    std::vector<double> a, std::vector<double> b,
    const std::vector<double> &weights = std::vector<double>());

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
double mode(const std::vector<double> &inputs);

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
double median(std::vector<double> &inputs);

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
 * @param a vector of a plain test array
 * @param k find top k from a
 * @return
 */
std::vector<int> find_top_k_indexes(const std::vector<double> &a, int k);

/**
 * revert the global index given a 2-dimensional index (id, j)
 *
 * @param a: the number of elements in each id vector
 * @param id: the id of local vector
 * @param idx: the index in local vector
 * @return
 */
int global_idx(const std::vector<int> &a, int id, int idx);

/**
 * Compare which one is bigger
 * @param a
 * @param b
 * @return
 */
bool MyComp(std::pair<double, int> a, std::pair<double, int> b);

/**
 *  Return the top K value's index in a list
 *  Create a pair of vectors, one for the values and one for the indices.
 *  Iterate through the original vector and for each element, add the value and
 * its index to the pair of vectors. Sort the pair of vectors in descending
 * order based on the values. Take the first K elements from the sorted pair of
 * vectors. These will be the K largest values and their indices.
 * @param a: the number of elements in each id vector
 * @param id: the id of local vector
 * @param idx: the index in local vector
 * @return
 */
std::vector<int> index_of_top_k_in_vector(std::vector<double> vec, int K);

/**
 * Partition numbers into multiple vector
 * @param numbers
 * @param partition_size how many ele in each partition .
 * @return
 */
std::vector<std::vector<int>>
partition_vec_evenly(const std::vector<int> &numbers, int partition_size);

/**
 * partition numbers into multiple vector in a balanced manner
 * so that each vector contains a specific proportion of each party
 * @param numbers: size is party num, each value the number of elements in each
 * party
 * @param num_partition
 * @return
 */
std::vector<std::vector<int>>
partition_vec_balanced(const std::vector<int> &numbers, int num_partition);

/**
 * find the index of a value in a vector
 * @param vec
 * @param ele
 * @return
 */
int find_idx_in_vec(const std::vector<int> &vec, int ele);

/**
 * compute the combination c_n^r
 * @param n
 * @param r
 * @return
 */
long long combination(long long n, long long r);

#endif // FALCON_SRC_EXECUTOR_UTILS_MATH_MATH_OPS_H_
