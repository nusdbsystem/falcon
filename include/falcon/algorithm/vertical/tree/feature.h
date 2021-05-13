//
// Created by wuyuncheng on 12/5/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FEATURE_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FEATURE_H_

#include <vector>
#include <iostream>

class Feature {
  // index of party's local feature
  int id;
  // the number of splits of the current feature, should <= max_bins - 1
  int num_splits;
  // the maximum number of bins (read from the config file)
  int max_bins;
  // 0: not used, 1: used, -1: not decided
  int is_used;
  // 0: not categorical, 1: categorical, -1: not decided
  int is_categorical;
  // the split threshold values of the features
  std::vector<double> split_values;
  // the original data of this feature in the training dataset
  std::vector<double> origin_feature_values;
  // maximum value of this feature
  double maximum_value;
  // minimum value of this feature
  double minimum_value;
  // the values of this feature are sorted, the re-sorted indexes are recorded
  std::vector<int> sorted_indexes;
  // pre-compute 0 1 ivs for each split in the left branch, before re-sorting
  std::vector< std::vector<int> > split_ivs_left;
  // pre-compute 0 1 ivs for each split in the right branch, before re-sorting
  std::vector< std::vector<int> > split_ivs_right;

 public:
 public:
  /**
   * default constructor
   */
  Feature();

  /**
   * default destructor
   */
  ~Feature();

  /**
   * copy constructor
   * @param feature
   */
  Feature(const Feature & feature);

  /**
   * copy assignment constructor
   * @param feature
   * @return
   */
  Feature &operator = (Feature *feature);

  /**
   * set the feature data given a column in the training dataset
   * @param values
   * @param size
   */
  void set_feature_data(std::vector<double> values, int size);

  /**
   * sort the feature values to accelerate the computation
   * when computing the encrypted statistics,
   * store the sorted indexes in the sorted_indexes vector
   */
  void sort_feature();

  /**
   * compute distinct values to find if it is categorical or continuous
   * according to the distinct values, update split_num
   * @return
   */
  std::vector<double> compute_distinct_values();

  /**
   * find the split values given the data and max_bins
   * currently assume the categorical values could also be sorted,
   * i.e., using label encoder instead of one hot encoder
   */
  void find_splits();

  /**
   * pre-compute and store the split indicator vectors
   * once a split is found, directly using the split iv
   * to update the encrypted sample iv
   */
  void compute_split_ivs();
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FEATURE_H_
