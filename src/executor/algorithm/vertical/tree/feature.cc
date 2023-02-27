/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by wuyuncheng on 12/5/21.
//

#include <algorithm> // std::sort
#include <falcon/algorithm/vertical/tree/feature.h>
#include <falcon/utils/logger/logger.h>
#include <glog/logging.h>
#include <numeric> // std::iota

FeatureHelper::FeatureHelper() = default;

FeatureHelper::~FeatureHelper() = default;

FeatureHelper::FeatureHelper(const FeatureHelper &feature_helper) {
  id = feature_helper.id;
  num_splits = feature_helper.num_splits;
  max_bins = feature_helper.max_bins;
  is_used = feature_helper.is_used;
  feature_type = feature_helper.feature_type;
  split_values = feature_helper.split_values;
  origin_feature_values = feature_helper.origin_feature_values;
  maximum_value = feature_helper.maximum_value;
  minimum_value = feature_helper.minimum_value;
  sorted_indexes = feature_helper.sorted_indexes;
  split_ivs_left = feature_helper.split_ivs_left;
  split_ivs_right = feature_helper.split_ivs_right;
}

FeatureHelper &FeatureHelper::operator=(FeatureHelper *feature_helper) {
  id = feature_helper->id;
  num_splits = feature_helper->num_splits;
  max_bins = feature_helper->max_bins;
  is_used = feature_helper->is_used;
  feature_type = feature_helper->feature_type;
  split_values = feature_helper->split_values;
  origin_feature_values = feature_helper->origin_feature_values;
  maximum_value = feature_helper->maximum_value;
  minimum_value = feature_helper->minimum_value;
  sorted_indexes = feature_helper->sorted_indexes;
  split_ivs_left = feature_helper->split_ivs_left;
  split_ivs_right = feature_helper->split_ivs_right;
}

void FeatureHelper::set_feature_data(std::vector<double> values, int size) {
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

std::vector<double> FeatureHelper::compute_distinct_values() {
  // now the feature values are sorted, the sorted indexes are stored in
  // sorted_indexes
  int sample_num = (int)origin_feature_values.size();
  int distinct_value_num = 0;
  std::vector<double> distinct_values;
  for (int i = 0; i < sample_num; i++) {
    // if no value has been added, directly add a value
    if (distinct_value_num == 0) {
      distinct_values.push_back(origin_feature_values[sorted_indexes[i]]);
      distinct_value_num++;
    } else {
      // if this value has been already added, skip
      if (distinct_values[distinct_value_num - 1] ==
          origin_feature_values[sorted_indexes[i]]) {
        continue;
      } else {
        // if his value is not seen , add directly
        distinct_values.push_back(origin_feature_values[sorted_indexes[i]]);
        distinct_value_num++;
      }
    }
  }
  return distinct_values;
}

void FeatureHelper::sort_feature() {
  std::vector<double> v = origin_feature_values;
  // initialize original index locations
  std::vector<int> idx(v.size());
  iota(idx.begin(), idx.end(), 0);
  // sort indexes based on comparing values in v
  // sort 100000 running time 30ms, sort 10000 running time 3ms
  sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });
  sorted_indexes = idx;
}

void FeatureHelper::find_splits() {
  /// use quantile sketch method, that computes k split values such that
  /// there are k + 1 bins, and each bin has almost same number samples

  /// basically, after sorting the feature values, we compute the size of
  /// each bin, n is the sample number
  /// i.e., n_sample_per_bin = n/(k+1), and samples[0:n_sample_per_bin]
  /// is the first bin, while (value[n_sample_per_bin] +
  /// value[n_sample_per_bin+1])/2 is the first split value, etc.

  /// note: currently assume that the feature values is sorted, treat
  /// categorical feature as label encoder sortable values

  int n_samples = (int)origin_feature_values.size();
  std::vector<double> distinct_values = compute_distinct_values();

  log_info("The number of distinct values of this feature is " +
           std::to_string(distinct_values.size()));

  // if distinct values is larger than max_bins + 1, treat as continuous feature
  // otherwise, treat as categorical feature
  if (distinct_values.size() >= max_bins) {
    // treat as continuous feature, find splits using quantile method
    // (might not accurate when the values are imbalanced)
    int n_sample_per_bin = n_samples / (num_splits + 1);
    for (int i = 0; i < num_splits; i++) {
      double split_value_i =
          (origin_feature_values[sorted_indexes[(i + 1) * n_sample_per_bin]] +
           origin_feature_values[sorted_indexes[(i + 1) * n_sample_per_bin +
                                                1]]) /
          2;
      split_values.push_back(split_value_i);
    }
  } else if (distinct_values.size() > 1) {
    // the split values are same as the distinct values
    num_splits = (int)distinct_values.size() - 1;
    for (int i = 0; i < num_splits; i++) {
      split_values.push_back(distinct_values[i]);
    }
  } else {
    // the distinct values is equal to 1, which is suspicious for the input
    // dataset
    log_info("This feature has only one distinct value, please check it again");
    num_splits = (int)distinct_values.size();
    split_values.push_back(distinct_values[0]);
  }
}

void FeatureHelper::compute_split_ivs() {
  split_ivs_left.reserve(num_splits);
  split_ivs_right.reserve(num_splits);
  for (int i = 0; i < num_splits; i++) {
    // read split value i
    double split_value_i = split_values[i];
    int n_samples = (int)origin_feature_values.size();
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