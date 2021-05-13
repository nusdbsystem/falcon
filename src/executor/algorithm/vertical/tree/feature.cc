//
// Created by wuyuncheng on 12/5/21.
//

#include <falcon/algorithm/vertical/tree/feature.h>
#include <numeric>      // std::iota
#include <algorithm>    // std::sort
#include <glog/logging.h>

Feature::Feature() {
  // empty constructor
}

Feature::~Feature() {
  // empty destructor
}

Feature::Feature(const Feature &feature) {
  id = feature.id;
  num_splits = feature.num_splits;
  max_bins = feature.max_bins;
  is_used = feature.is_used;
  is_categorical = feature.is_categorical;
  split_values = feature.split_values;
  origin_feature_values = feature.origin_feature_values;
  maximum_value = feature.maximum_value;
  minimum_value = feature.minimum_value;
  sorted_indexes = feature.sorted_indexes;
  split_ivs_left = feature.split_ivs_left;
  split_ivs_right = feature.split_ivs_right;
}

Feature& Feature::operator=(Feature *feature) {
  id = feature->id;
  num_splits = feature->num_splits;
  max_bins = feature->max_bins;
  is_used = feature->is_used;
  is_categorical = feature->is_categorical;
  split_values = feature->split_values;
  origin_feature_values = feature->origin_feature_values;
  maximum_value = feature->maximum_value;
  minimum_value = feature->minimum_value;
  sorted_indexes = feature->sorted_indexes;
  split_ivs_left = feature->split_ivs_left;
  split_ivs_right = feature->split_ivs_right;
}

void Feature::set_feature_data(std::vector<double> values, int size) {
  double max = std::numeric_limits<double>::max();
  double min = std::numeric_limits<double>::min();
  origin_feature_values.reserve(size);
  for (int i = 0; i < size; i++) {
    origin_feature_values.push_back(values[i]);
    if (values[i] > max) {
      max = values[i];
    }
    if (values[i] < min) {
      min = values[i];
    }
  }
  maximum_value = max;
  minimum_value = min;
}

std::vector<double> Feature::compute_distinct_values() {
  // now the feature values are sorted, the sorted indexes are stored in sorted_indexes
  int sample_num = origin_feature_values.size();
  int distinct_value_num = 0;
  std::vector<double> distinct_values;
  for (int i = 0; i < sample_num; i++) {
    // if no value has been added, directly add a value
    if (distinct_value_num == 0) {
      distinct_values.push_back(origin_feature_values[sorted_indexes[i]]);
      distinct_value_num++;
    } else {
      if (distinct_values[distinct_value_num-1] == origin_feature_values[sorted_indexes[i]]) {
        continue;
      } else {
        distinct_values.push_back(origin_feature_values[sorted_indexes[i]]);
        distinct_value_num++;
      }
    }
  }
  return distinct_values;
}

void Feature::sort_feature() {
  std::vector<double> v = origin_feature_values;
  // initialize original index locations
  std::vector<int> idx(v.size());
  iota(idx.begin(), idx.end(), 0);
  // sort indexes based on comparing values in v
  // sort 100000 running time 30ms, sort 10000 running time 3ms
  sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
  sorted_indexes = idx;
}

void Feature::find_splits() {
  /// use quantile sketch method, that computes k split values such that
  /// there are k + 1 bins, and each bin has almost same number samples

  /// basically, after sorting the feature values, we compute the size of
  /// each bin, i.e., n_sample_per_bin = n/(k+1), and samples[0:n_sample_per_bin]
  /// is the first bin, while (value[n_sample_per_bin] + value[n_sample_per_bin+1])/2
  /// is the first split value, etc.

  /// note: currently assume that the feature values is sorted, treat categorical
  /// feature as label encoder sortable values

  int n_samples = origin_feature_values.size();
  std::vector<double> distinct_values = compute_distinct_values();

  // if distinct values is larger than max_bins + 1, treat as continuous feature
  // otherwise, treat as categorical feature
  if (distinct_values.size() >= max_bins) {
    // treat as continuous feature, find splits using quantile method
    // (might not accurate when the values are imbalanced)
    int n_sample_per_bin = n_samples / (num_splits + 1);
    for (int i = 0; i < num_splits; i++) {
      double split_value_i = (origin_feature_values[sorted_indexes[(i + 1) * n_sample_per_bin]]
          + origin_feature_values[sorted_indexes[(i + 1) * n_sample_per_bin + 1]])/2;
      split_values.push_back(split_value_i);
    }
  }
  else if (distinct_values.size() > 1) {
    // the split values are same as the distinct values
    num_splits = (int) distinct_values.size() - 1;
    for (int i = 0; i < num_splits; i++) {
      split_values.push_back(distinct_values[i]);
    }
  }
  else {
    // the distinct values is equal to 1, which is suspicious for the input dataset
    LOG(INFO) << "This feature has only one distinct value, please check it again";
    num_splits = distinct_values.size();
    split_values.push_back(distinct_values[0]);
  }
}

void Feature::compute_split_ivs() {
  split_ivs_left.reserve(num_splits);
  split_ivs_right.reserve(num_splits);
  for (int i = 0; i < num_splits; i++) {
    // read split value i
    double split_value_i = split_values[i];
    int n_samples = origin_feature_values.size();
    std::vector<int> indicator_vec_left;
    indicator_vec_left.reserve(n_samples);
    std::vector<int> indicator_vec_right;
    indicator_vec_right.reserve(n_samples);
    for (int j = 0; j < n_samples; j++) {
      if (origin_feature_values[j] <= split_value_i) {
        indicator_vec_left.push_back(1);
        indicator_vec_right.push_back(0);
      } else {
        indicator_vec_left.push_back(0);
        indicator_vec_right.push_back(1);
      }
    }
    split_ivs_left.push_back(indicator_vec_left);
    split_ivs_right.push_back(indicator_vec_right);
  }
}