//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/logger/log_alg_params.h>
#include <falcon/utils/logger/logger.h>

#include <map>
#include <stack>
#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>
#include <falcon/utils/math/math_ops.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/message_lite.h>

DecisionTreeBuilder::DecisionTreeBuilder() {}

DecisionTreeBuilder::~DecisionTreeBuilder() {
  delete [] feature_helpers;
}

DecisionTreeBuilder::DecisionTreeBuilder(DecisionTreeParams params,
    std::vector<std::vector<double> > m_training_data,
    std::vector<std::vector<double> > m_testing_data,
    std::vector<double> m_training_labels,
    std::vector<double> m_testing_labels,
    double m_training_accuracy,
    double m_testing_accuracy) : ModelBuilder(std::move(m_training_data),
          std::move(m_testing_data),
          std::move(m_training_labels),
          std::move(m_testing_labels),
          m_training_accuracy,
          m_testing_accuracy) {

  train_data_size = training_data.size();
  test_data_size = testing_data.size();

  // copy builder parameters
  if (params.tree_type == "classification") {
    tree_type = falcon::CLASSIFICATION;
    class_num = params.class_num;
  } else {
    tree_type = falcon::REGRESSION;
    // for regression, the class num is set to 2 for y and y^2
    class_num = REGRESSION_TREE_CLASS_NUM;
  }
  criterion = params.criterion;
  split_strategy = params.split_strategy;
  max_depth = params.max_depth;
  max_bins = params.max_bins;
  min_samples_split = params.min_samples_split;
  min_samples_leaf = params.min_samples_leaf;
  max_leaf_nodes = params.max_leaf_nodes;
  min_impurity_decrease = params.min_impurity_decrease;
  min_impurity_split = params.min_impurity_split;
  dp_budget = params.dp_budget;
  // init feature types to continuous by default
  // NOTE: here should use the training_data instead of m_training_data
  //        because the m_training_data is already moved
  local_feature_num = (int) training_data[0].size();
  for (int i = 0; i < local_feature_num; i++) {
    feature_types.push_back(falcon::CONTINUOUS);
  }
  feature_helpers = new FeatureHelper[local_feature_num];
  // init tree object
  tree = TreeModel(tree_type, class_num, max_depth);
}

void DecisionTreeBuilder::precompute_label_helper(falcon::PartyType party_type) {
  if (party_type == falcon::PASSIVE_PARTY) {
    return;
  }
  // only the active party has label information
  // pre-compute indicator vectors or variance vectors for labels
  // here already assume that it is computed on active party
  LOG(INFO) << "Pre-compute label assist info for training";
  if (tree_type == falcon::CLASSIFICATION) {
    // classification, compute binary vectors and store
    int * sample_num_classes = new int[class_num];
    for (int i = 0; i < class_num; i++) {
      sample_num_classes[i] = 0;
    }
    for (int i = 0; i < class_num; i++) {
      // r1 and r2 in the paper, Figure 2
      std::vector<int> indicator_vec;
      for (int j = 0; j < training_labels.size(); j++) {
        if (training_labels[j] == (double) i) {
          indicator_vec.push_back(1);
          sample_num_classes[i] += 1;
        } else {
          indicator_vec.push_back(0);
        }
      }
      indicator_class_vecs.push_back(indicator_vec);
    }
    for (int i = 0; i < class_num; i++) {
      LOG(INFO) << "Class " << i << "'s sample num = " << sample_num_classes[i];
    }
    delete [] sample_num_classes;
  } else {
    // regression, compute variance necessary stats
    std::vector<double> label_square_vec;
    for (int i = 0; i < training_labels.size(); i++) {
      label_square_vec.push_back(training_labels[i] * training_labels[i]);
    }
    variance_stat_vecs.push_back(training_labels); // the first vector is the actual label vector
    variance_stat_vecs.push_back(label_square_vec);     // the second vector is the squared label vector
  }
}

void DecisionTreeBuilder::precompute_feature_helpers() {
  LOG(INFO) << "Pre-compute feature helpers for training";
  for (int i = 0; i < local_feature_num; i++) {
    // 1. extract feature values of the i-th feature, compute samples_num
    // 2. check if distinct values number <= max_bins, if so, update splits_num as distinct number
    // 3. init feature, and assign to features[i]
    std::vector<double> feature_values;
    for (int j = 0; j < training_data.size(); j++) {
      feature_values.push_back(training_data[j][i]);
    }
    feature_helpers[i].id = i;
    feature_helpers[i].is_used = 0;
    feature_helpers[i].feature_type = feature_types[i];
    feature_helpers[i].num_splits = max_bins - 1;
    feature_helpers[i].max_bins = max_bins;
    feature_helpers[i].set_feature_data(feature_values, training_data.size());
    feature_helpers[i].sort_feature();
    feature_helpers[i].find_splits();
    feature_helpers[i].compute_split_ivs();

    // for debug
    for (int x = 0; x < feature_helpers[i].split_values.size(); x++) {
      log_info("Feature " + std::to_string(i) + "'s split values[" + std::to_string(x) + "] = " + std::to_string(feature_helpers[i].split_values[x]));
    }
  }
}

void DecisionTreeBuilder::train(Party party) {
  /// To avoid each tree node disclose which samples are on available,
  /// we use an encrypted mask vector to protect the information
  /// while ensure that the training can still be processed.
  /// Note that for the root node, all the samples are available and every
  /// party can initialize an encrypted mask vector with [1], but for the
  /// labels, only the active party has the info, then it initializes
  /// and sends the encrypted label info to other parties.

  std::cout << "************* Training Start *************" << std::endl;
  LOG(INFO) << "************* Training Start *************";
  const clock_t training_start_time = clock();

  // pre-compute label helper and feature helper
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();

  int sample_num = training_data.size();
  // label_size = len(indicator_class_vecs), len([r1, r2])
  int label_size = class_num * sample_num;
  std::vector<int> available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    available_feature_ids.push_back(i);
  }
  EncodedNumber * sample_mask_iv = new EncodedNumber[sample_num];
  EncodedNumber * encrypted_labels = new EncodedNumber[class_num * sample_num];

  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // because all samples are available at the beginning of the training, init with 1
  EncodedNumber tmp;
  tmp.set_integer(phe_pub_key->n[0], 1);
  // init encrypted mask vector on the root node
  for (int i = 0; i < sample_num; i++) {
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, sample_mask_iv[i], tmp);
  }

  // if active party, compute the encrypted label info and broadcast
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::string encrypted_labels_str;
    for (int i = 0; i < class_num; i++) {
      for (int j = 0; j < sample_num; j++) {
        EncodedNumber tmp_label;
        // classification use indicator_class_vecs, regression use variance_stat_vecs
        if (tree_type == falcon::CLASSIFICATION) {
          tmp_label.set_double(phe_pub_key->n[0], indicator_class_vecs[i][j]);
        } else {
          tmp_label.set_double(phe_pub_key->n[0], variance_stat_vecs[i][j]);
        }
        // encrypt the label
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           encrypted_labels[i * sample_num + j], tmp_label);
      }
    }

    // serialize and send to the other client
    serialize_encoded_number_array(encrypted_labels, label_size, encrypted_labels_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, encrypted_labels_str);
      }
    }
  } else {
    // if not active party, receive the encrypted label info
    std::string recv_result_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_result_str);
    deserialize_encoded_number_array(encrypted_labels, label_size, recv_result_str);
  }
  LOG(INFO) << "Finish broadcasting the encrypted label info";

  // init the root node info
  tree.nodes[0].depth = 0;
  EncodedNumber max_impurity;
  max_impurity.set_double(phe_pub_key->n[0], MAX_IMPURITY, PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, max_impurity);

  // required by spdz connector and mpc computation
  bigint::init_thread();
  // recursively build the tree
  build_node(party, 0, available_feature_ids, sample_mask_iv, encrypted_labels);
  LOG(INFO) << "tree capacity = " << tree.capacity;

  delete [] sample_mask_iv;
  delete [] encrypted_labels;
  djcs_t_free_public_key(phe_pub_key);

  const clock_t training_finish_time = clock();
  double training_consumed_time = double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Training time = " << training_consumed_time;
  LOG(INFO) << "************* Training Finished *************";
  google::FlushLogFiles(google::INFO);
}

void DecisionTreeBuilder::train(Party party, EncodedNumber *encrypted_labels) {
  /// To avoid each tree node disclose which samples are on available,
  /// we use an encrypted mask vector to protect the information
  /// while ensure that the training can still be processed.
  /// Note that for the root node, all the samples are available and every
  /// party can initialize an encrypted mask vector with [1]
  /// for the labels, the format should be encrypted (with size class_num * sample_num),
  /// and is known to other parties as well (should be sent before calling this train method)

  std::cout << "************* Training Start *************" << std::endl;
  LOG(INFO) << "************* Training Start *************";
  const clock_t training_start_time = clock();

  // pre-compute label helper and feature helper
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();
  log_info("[train]: step 1, finished init label and features");

  int sample_num = training_data.size();
  int label_size = class_num * sample_num;
  std::vector<int> available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    available_feature_ids.push_back(i);
  }
  auto * sample_mask_iv = new EncodedNumber[sample_num];

  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // as the samples are available at the beginning of the training, init with 1
  EncodedNumber tmp;
  tmp.set_integer(phe_pub_key->n[0], 1);
  // init encrypted mask vector on the root node
  for (int i = 0; i < sample_num; i++) {
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, sample_mask_iv[i], tmp);
  }

  // init the root node info
  tree.nodes[0].depth = 0;
  EncodedNumber max_impurity;
  max_impurity.set_double(phe_pub_key->n[0], MAX_IMPURITY, PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, max_impurity);

  // required by spdz connector and mpc computation
  bigint::init_thread();
  // recursively build the tree
  build_node(party, 0, available_feature_ids, sample_mask_iv, encrypted_labels);
  LOG(INFO) << "tree capacity = " << tree.capacity;

  delete [] sample_mask_iv;
  djcs_t_free_public_key(phe_pub_key);

  const clock_t training_finish_time = clock();
  double training_consumed_time = double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Training time = " << training_consumed_time;
  LOG(INFO) << "************* Training Finished *************";
  google::FlushLogFiles(google::INFO);
}

void DecisionTreeBuilder::lime_train(
    Party party,
    bool use_encrypted_labels,
    EncodedNumber *encrypted_true_labels,
    bool use_sample_weights,
    EncodedNumber *encrypted_weights) {
  /// To avoid each tree node disclose which samples are on available,
  /// we use an encrypted mask vector to protect the information
  /// while ensure that the training can still be processed.
  /// Note that for the root node, all the samples are available and every
  /// party can initialize an encrypted mask vector with [1]
  /// for the labels, the format should be encrypted (with size class_num * sample_num),
  /// and is known to other parties as well (should be sent before calling this train method)

  std::cout << "************* Training Start *************" << std::endl;
  LOG(INFO) << "************* Training Start *************";
  const clock_t training_start_time = clock();

  // pre-compute label helper and feature helper
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();
  log_info("[train]: step 1, finished init label and features");

  int sample_num = (int) training_data.size();
  int label_size = class_num * sample_num;
  log_info("[train]: sample_num = " + std::to_string(sample_num) + ", label_size = " + std::to_string(label_size));
  std::vector<int> available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    available_feature_ids.push_back(i);
  }
  auto * sample_mask_iv = new EncodedNumber[sample_num];

  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // as the samples are available at the beginning of the training, init with 1
  EncodedNumber tmp;
  tmp.set_integer(phe_pub_key->n[0], 1);
  // init encrypted mask vector on the root node
  for (int i = 0; i < sample_num; i++) {
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, sample_mask_iv[i], tmp);
  }

  // init the root node info
  tree.nodes[0].depth = 0;
  EncodedNumber max_impurity;
  max_impurity.set_double(phe_pub_key->n[0], MAX_IMPURITY, PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, max_impurity);

  // check whether use sample_weights, if so, compute cipher multi
  auto* weighted_encrypted_true_labels = new EncodedNumber[label_size];
  if (use_sample_weights) {
    auto* assist_encrypted_weights = new EncodedNumber[label_size];
    for (int i = 0; i < sample_num; i++) {
      assist_encrypted_weights[i] = encrypted_weights[i];
      assist_encrypted_weights[i+sample_num] = encrypted_weights[i];
    }
    party.ciphers_multi(weighted_encrypted_true_labels,
                        encrypted_true_labels,
                        assist_encrypted_weights,
                        label_size,
                        ACTIVE_PARTY_ID);
    delete [] assist_encrypted_weights;
  } else {
    for (int i = 0; i < label_size; i++) {
      weighted_encrypted_true_labels[i] = encrypted_true_labels[i];
    }
  }

  // truncate encrypted labels to PHE_FIXED_POINT_PRECISION
  // TODO: make the label precision automatically match
  party.truncate_ciphers_precision(weighted_encrypted_true_labels,
                                   label_size,
                                   ACTIVE_PARTY_ID,
                                   PHE_FIXED_POINT_PRECISION);

  // required by spdz connector and mpc computation
  bigint::init_thread();
  // recursively build the tree
  build_node(party,
             0,
             available_feature_ids,
             sample_mask_iv,
             weighted_encrypted_true_labels,
             use_sample_weights,
             encrypted_weights);
  LOG(INFO) << "tree capacity = " << tree.capacity;

  delete [] sample_mask_iv;
  delete [] weighted_encrypted_true_labels;
  djcs_t_free_public_key(phe_pub_key);

  const clock_t training_finish_time = clock();
  double training_consumed_time = double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Training time = " << training_consumed_time;
  LOG(INFO) << "************* Training Finished *************";
  google::FlushLogFiles(google::INFO);
}

void DecisionTreeBuilder::build_node(Party &party,
    int node_index,
    std::vector<int> available_feature_ids,
    EncodedNumber *sample_mask_iv,
    EncodedNumber *encrypted_labels,
    bool use_sample_weights,
    EncodedNumber *encrypted_weights) {
  std::cout << "****** Build tree node " << node_index
            << " (depth = " << tree.nodes[node_index].depth << ")******" << std::endl;
  LOG(INFO) << "****** Build tree node " << node_index
            << " (depth = " << tree.nodes[node_index].depth << ")******";
  const clock_t node_start_time = clock();

  /// 1. check if some pruning conditions are satisfied
  ///      1.1 feature set is empty;
  ///      1.2 the number of samples are less than a pre-defined threshold
  ///      1.3 if classification, labels are same; if regression,
  ///         label variance is less than a threshold
  /// 2. if satisfied, return a leaf node with majority class or mean label;
  ///      else, continue to step 3
  /// 3. super client computes some encrypted label information
  ///     and broadcast to the other clients
  /// 4. every client locally compute necessary encrypted statistics,
  ///     i.e., #samples per class for classification or variance info
  /// 5. the clients convert the encrypted statistics to shares
  ///     and send to SPDZ parties
  /// 6. wait for SPDZ parties return (i_*, j_*, s_*),
  ///     where i_* is client id, j_* is feature id, and s_* is split id
  /// 7. client who owns the best split feature do the splits
  ///     and update mask vector, and broadcast to the other clients
  /// 8. every client updates mask vector and local tree model
  /// 9. recursively build the next two tree nodes

  if (node_index >= tree.capacity) {
    log_info("Node index exceeds the maximum tree depth");
    exit(1);
  }

  /// step 1: check pruning condition via spdz computation
  bool is_satisfied = check_pruning_conditions(party, node_index, sample_mask_iv);

  /// step 2: if satisfied, compute label via spdz computation
  if (is_satisfied) {
    compute_leaf_statistics(party,
                            node_index,
                            sample_mask_iv,
                            encrypted_labels,
                            use_sample_weights,
                            encrypted_weights);
    return;
  }

  /// step 3: active party computes some encrypted label
  /// information and broadcast to the other clients

  /// step 4: every party locally computes necessary encrypted statistics,
  /// i.e., #samples per class for classification or variance info
  ///     specifically, for each feature, for each split, for each class,
  ///     compute necessary encrypted statistics,
  ///     store the encrypted statistics,
  ///     convert to secret shares,
  ///     and send to spdz parties for mpc computation
  ///     finally, receive the (i_*, j_*, s_*) and encrypted impurity
  /// for the left child and right child, update tree_nodes
  /// step 4.1: party inits a two-dimensional encrypted vector
  ///     with size \sum_{i=0}^{node[available_feature_ids.size()]} features[i].num_splits
  /// step 4.2: party computes encrypted statistics
  /// step 4.3: party sends encrypted statistics
  /// step 4.4: active party computes a large encrypted
  ///     statistics matrix, and broadcasts total splits num
  /// step 4.5: party converts the encrypted statistics matrix into secret shares
  /// step 4.6: parties jointly send shares to spdz parties

  int local_splits_num = 0, global_split_num = 0;
  int available_local_feature_num = (int) available_feature_ids.size();
  for (int i = 0; i < available_local_feature_num; i++) {
    int feature_id = available_feature_ids[i];
    local_splits_num = local_splits_num + feature_helpers[feature_id].num_splits;
  }

  // the global encrypted statistics for all possible splits
  EncodedNumber **global_encrypted_statistics;
  EncodedNumber *global_left_branch_sample_nums, *global_right_branch_sample_nums;
  EncodedNumber **encrypted_statistics;
  EncodedNumber *encrypted_left_branch_sample_nums, *encrypted_right_branch_sample_nums;
  std::vector<int> party_split_nums;

  global_encrypted_statistics = new EncodedNumber*[MAX_GLOBAL_SPLIT_NUM];
  for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
    global_encrypted_statistics[i] = new EncodedNumber[2 * class_num];
  }
  global_left_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];
  global_right_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];

  // compute local encrypted statistics
  if (local_splits_num != 0) {
    encrypted_statistics = new EncodedNumber*[local_splits_num];
    for (int i = 0; i < local_splits_num; i++) {
      // here 2 * class_num refers to left branch and right branch
      encrypted_statistics[i] = new EncodedNumber[2 * class_num];
    }
    encrypted_left_branch_sample_nums = new EncodedNumber[local_splits_num];
    encrypted_right_branch_sample_nums = new EncodedNumber[local_splits_num];
    // call compute function
    compute_encrypted_statistics(party, node_index,
                                 available_feature_ids,
                                 sample_mask_iv,
                                 encrypted_statistics,
                                 encrypted_labels,
                                 encrypted_left_branch_sample_nums,
                                 encrypted_right_branch_sample_nums,
                                 use_sample_weights,
                                 encrypted_weights);
  }
  log_info("Finish computing local statistics");

  if (party.party_type == falcon::ACTIVE_PARTY) {
    // pack self
    if (local_splits_num == 0) {
      party_split_nums.push_back(0);
    } else {
      party_split_nums.push_back(local_splits_num);
      for (int i = 0; i < local_splits_num; i++) {
        global_left_branch_sample_nums[i] = encrypted_left_branch_sample_nums[i];
        global_right_branch_sample_nums[i] = encrypted_right_branch_sample_nums[i];
        for (int j = 0; j < 2 * class_num; j++) {
          global_encrypted_statistics[i][j] = encrypted_statistics[i][j];
        }
      }
    }
    global_split_num += local_splits_num;

    // receive from the other clients of the encrypted statistics
    for (int i = 0; i < party.party_num; i++) {
      std::string recv_encrypted_statistics_str, recv_local_split_num_str;
      if (i != party.party_id) {
        party.recv_long_message(i, recv_local_split_num_str);
        party.recv_long_message(i, recv_encrypted_statistics_str);
        int recv_party_id, recv_node_index, recv_split_num, recv_classes_num;
        recv_split_num = std::stoi(recv_local_split_num_str);
        // pack the encrypted statistics
        if (recv_split_num == 0) {
          party_split_nums.push_back(0);
          continue;
        }

        if (recv_split_num != 0) {
          auto **recv_encrypted_statistics = new EncodedNumber*[recv_split_num];
          for (int s = 0; s < recv_split_num; s++) {
            recv_encrypted_statistics[s] = new EncodedNumber[2 * class_num];
          }
          auto *recv_left_sample_nums = new EncodedNumber[recv_split_num];
          auto *recv_right_sample_nums = new EncodedNumber[recv_split_num];
          deserialize_encrypted_statistics(recv_party_id, recv_node_index,
                                           recv_split_num, recv_classes_num,
                                           recv_left_sample_nums, recv_right_sample_nums,
                                           recv_encrypted_statistics,
                                           recv_encrypted_statistics_str);

          party_split_nums.push_back(recv_split_num);
          for (int j = 0; j < recv_split_num; j++) {
            global_left_branch_sample_nums[global_split_num + j] = recv_left_sample_nums[j];
            global_right_branch_sample_nums[global_split_num + j] = recv_right_sample_nums[j];
            for (int k = 0; k < 2 * class_num; k++) {
              global_encrypted_statistics[global_split_num + j][k] = recv_encrypted_statistics[j][k];
            }
          }
          global_split_num += recv_split_num;

          delete [] recv_left_sample_nums;
          delete [] recv_right_sample_nums;
          for (int xx = 0; xx < recv_split_num; xx++) {
            delete [] recv_encrypted_statistics[xx];
          }
          delete [] recv_encrypted_statistics;
        }
      }
    }
    log_info("The global_split_num = " + std::to_string(global_split_num));
    // send the total number of splits for the other clients to generate secret shares
    //logger(logger_out, "Send global split num to the other clients\n");
    std::string split_info_str;
    serialize_split_info(global_split_num, party_split_nums, split_info_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, split_info_str);
      }
    }
  }

  if (party.party_type == falcon::PASSIVE_PARTY) {
    // first send local split num
    log_info("Local split num = " + std::to_string(local_splits_num));
    std::string local_split_num_str = std::to_string(local_splits_num);
    party.send_long_message(ACTIVE_PARTY_ID, local_split_num_str);
    // then send the encrypted statistics
    std::string encrypted_statistics_str;
    serialize_encrypted_statistics(party.party_id, node_index,
                                   local_splits_num, class_num,
                                   encrypted_left_branch_sample_nums,
                                   encrypted_right_branch_sample_nums,
                                   encrypted_statistics, encrypted_statistics_str);
    party.send_long_message(ACTIVE_PARTY_ID, encrypted_statistics_str);

    // recv the split info from active party
    std::string recv_split_info_str;
    party.recv_long_message(0, recv_split_info_str);
    deserialize_split_info(global_split_num, party_split_nums, recv_split_info_str);
    log_info("The global_split_num = " + std::to_string(global_split_num));
  }

  /// step 5: encrypted statistics computed finished, convert to secret shares
  std::vector< std::vector<double> > stats_shares;
  std::vector<double> left_sample_nums_shares;
  std::vector<double> right_sample_nums_shares;
  std::vector<int> public_values;
  std::vector<double> private_values;
  int branch_sample_nums_prec;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    branch_sample_nums_prec = std::abs(global_left_branch_sample_nums[0].getter_exponent());
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, std::to_string(branch_sample_nums_prec));
      }
    }
  } else {
    std::string recv_prec;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_prec);
    branch_sample_nums_prec = std::stoi(recv_prec);
  }
  log_info("[build_node]: branch_sample_nums_prec = " + std::to_string(branch_sample_nums_prec));
  party.ciphers_to_secret_shares(global_left_branch_sample_nums,
      left_sample_nums_shares, global_split_num,
      ACTIVE_PARTY_ID, branch_sample_nums_prec);
  party.ciphers_to_secret_shares(global_right_branch_sample_nums,
      right_sample_nums_shares, global_split_num,
      ACTIVE_PARTY_ID, branch_sample_nums_prec);
  for (int i = 0; i < global_split_num; i++) {
    std::vector<double> tmp;
    party.ciphers_to_secret_shares(global_encrypted_statistics[i],
        tmp, 2 * class_num,
        ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
    stats_shares.push_back(tmp);
  }
  log_info("[build_node]: convert secret shares finished ");

  /// step 6: secret shares conversion finished, talk to spdz for computation
  // communicate with spdz parties and receive results to compute labels
  // first send computation type, tree type, class num
  // then send private values

  // arrange public values and private values
  public_values.push_back(tree_type);
  public_values.push_back(global_split_num);
  public_values.push_back(class_num);
  public_values.push_back(class_num);

  // according to the spdz program,
  // first send the encrypted_statistics: global_split_num * (2 * class_num)
  // then send the left shares, and finally the right shares: global_split_num
  for (int i = 0; i < global_split_num; i++) {
    for (int j = 0; j < 2 * class_num; j++) {
      private_values.push_back(stats_shares[i][j]);
    }
  }
  for (int i = 0; i < global_split_num; i++) {
    private_values.push_back(left_sample_nums_shares[i]);
  }
  for (int i = 0; i < global_split_num; i++) {
    private_values.push_back(right_sample_nums_shares[i]);
  }
  falcon::SpdzTreeCompType comp_type = falcon::FIND_BEST_SPLIT;
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  std::thread spdz_pruning_check_thread(spdz_tree_computation,
      party.party_num,
      party.party_id,
      party.executor_mpc_ports,
      party.host_names,
      public_values.size(),
      public_values,
      private_values.size(),
      private_values,
      comp_type,
      &promise_values);
  std::vector<double> res = future_values.get();
  spdz_pruning_check_thread.join();
  log_info("[build_node]: communicate with spdz program finished");

  // the result values are as follows (assume public in this version):
  // best_split_index (global), best_left_impurity, and best_right_impurity
  int best_split_index = (int) res[0];
  double left_impurity = res[1];
  double right_impurity = res[2];
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  EncodedNumber encrypted_left_impurity, encrypted_right_impurity;
  encrypted_left_impurity.set_double(phe_pub_key->n[0],
                                     left_impurity, PHE_FIXED_POINT_PRECISION);
  encrypted_right_impurity.set_double(phe_pub_key->n[0],
                                      right_impurity, PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                     encrypted_left_impurity, encrypted_left_impurity);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                     encrypted_right_impurity, encrypted_right_impurity);

  /// step 7: update tree nodes, including sample iv for iterative node computation
  std::vector<int> available_feature_ids_new;
  int sample_num = (int) training_data.size();
  EncodedNumber *sample_mask_iv_left = new EncodedNumber[sample_num];
  EncodedNumber *sample_mask_iv_right = new EncodedNumber[sample_num];
  EncodedNumber *encrypted_labels_left = new EncodedNumber[sample_num * class_num];
  EncodedNumber *encrypted_labels_right = new EncodedNumber[sample_num * class_num];

  int left_child_index = 2 * node_index + 1;
  int right_child_index = 2 * node_index + 2;

  // LOG(INFO) << "Create sample iv for " << left_child_index << " and " << right_child_index;

  // convert the index_in_global_split_num to (i_*, index_*)
  int i_star = -1;
  int index_star = -1;
  int index_tmp = best_split_index;
  for (int i = 0; i < party_split_nums.size(); i++) {
    if (index_tmp < party_split_nums[i]) {
      i_star = i;
      index_star = index_tmp;
      break;
    } else {
      index_tmp = index_tmp - party_split_nums[i];
    }
  }
  log_info("Best split party: i_star = " + std::to_string(i_star));

  // if the party has the best split:
  // compute locally and broadcast, find the j_* feature and s_* split
  if (i_star == party.party_id) {
    int j_star = -1;
    int s_star = -1;
    int index_star_tmp = index_star;
    for (int i = 0; i < available_feature_ids.size(); i++) {
      int feature_id = available_feature_ids[i];
      if (index_star_tmp < feature_helpers[feature_id].num_splits) {
        j_star = feature_id;
        s_star = index_star_tmp;
        break;
      } else {
        index_star_tmp = index_star_tmp - feature_helpers[feature_id].num_splits;
      }
    }

    log_info("Best feature id: j_star = " + std::to_string(j_star));
    log_info("Best split id: s_star = " + std::to_string(s_star));

    // now we have (i_*, j_*, s_*), retrieve s_*-th split ivs and update sample_ivs of two branches
    // update current node index for prediction
    tree.nodes[node_index].node_type = falcon::INTERNAL;
    tree.nodes[node_index].is_self_feature = 1;
    tree.nodes[node_index].best_party_id = i_star;
    tree.nodes[node_index].best_feature_id = j_star;
    tree.nodes[node_index].best_split_id = s_star;
    tree.nodes[node_index].split_threshold = feature_helpers[j_star].split_values[s_star];

    tree.nodes[left_child_index].depth = tree.nodes[node_index].depth + 1;
    tree.nodes[left_child_index].impurity = encrypted_left_impurity;
    tree.nodes[right_child_index].depth = tree.nodes[node_index].depth + 1;
    tree.nodes[right_child_index].impurity = encrypted_right_impurity;

    for (int i = 0; i < available_feature_ids.size(); i++) {
      int feature_id = available_feature_ids[i];
      if (j_star != feature_id) {
        available_feature_ids_new.push_back(feature_id);
      }
    }

    // compute between split_iv and sample_iv and update
    std::vector<int> split_left_iv = feature_helpers[j_star].split_ivs_left[s_star];
    std::vector<int> split_right_iv = feature_helpers[j_star].split_ivs_right[s_star];
    for (int i = 0; i < sample_num; i++) {
      EncodedNumber left, right;
      left.set_integer(phe_pub_key->n[0], split_left_iv[i]);
      right.set_integer(phe_pub_key->n[0], split_right_iv[i]);
      djcs_t_aux_ep_mul(phe_pub_key, sample_mask_iv_left[i], sample_mask_iv[i], left);
      djcs_t_aux_ep_mul(phe_pub_key, sample_mask_iv_right[i], sample_mask_iv[i], right);
    }

    // serialize and send to the other clients
    std::string update_str_sample_iv;
    serialize_update_info(party.party_id, party.party_id, j_star, s_star,
        encrypted_left_impurity, encrypted_right_impurity,
        sample_mask_iv_left, sample_mask_iv_right,
        sample_num, update_str_sample_iv);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, update_str_sample_iv);
      }
    }

    // compute between split_iv and encrypted_labels and update
    for (int i = 0; i < class_num; i++) {
      for (int j = 0; j < sample_num; j++) {
        EncodedNumber left, right;
        left.set_integer(phe_pub_key->n[0], split_left_iv[j]);
        right.set_integer(phe_pub_key->n[0], split_right_iv[j]);
        djcs_t_aux_ep_mul(phe_pub_key, encrypted_labels_left[i * sample_num + j],
                          encrypted_labels[i * sample_num + j], left);
        djcs_t_aux_ep_mul(phe_pub_key, encrypted_labels_right[i * sample_num + j],
                          encrypted_labels[i * sample_num + j], right);
      }
    }
    // serialize and send to the other client
    std::string update_str_encrypted_labels_left, update_str_encrypted_labels_right;
    serialize_encoded_number_array(encrypted_labels_left, class_num * sample_num,
                                     update_str_encrypted_labels_left);
    serialize_encoded_number_array(encrypted_labels_right, class_num * sample_num,
                                     update_str_encrypted_labels_right);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, update_str_encrypted_labels_left);
        party.send_long_message(i, update_str_encrypted_labels_right);
      }
    }
  }

  /// step 8: every other party update the local tree model
  if (i_star != party.party_id) {
    // receive from i_star client and update
    std::string recv_update_str_sample_iv, recv_update_str_encrypted_labels_left, recv_update_str_encrypted_labels_right;
    party.recv_long_message(i_star, recv_update_str_sample_iv);
    party.recv_long_message(i_star, recv_update_str_encrypted_labels_left);
    party.recv_long_message(i_star, recv_update_str_encrypted_labels_right);

    // deserialize and update sample iv
    int recv_source_party_id, recv_best_party_id, recv_best_feature_id, recv_best_split_id;
    EncodedNumber recv_left_impurity, recv_right_impurity;
    deserialize_update_info(recv_source_party_id, recv_best_party_id,
        recv_best_feature_id, recv_best_split_id,
        recv_left_impurity, recv_right_impurity,
        sample_mask_iv_left, sample_mask_iv_right,
        recv_update_str_sample_iv);

    deserialize_encoded_number_array(encrypted_labels_left, class_num * sample_num, recv_update_str_encrypted_labels_left);
    deserialize_encoded_number_array(encrypted_labels_right, class_num * sample_num, recv_update_str_encrypted_labels_right);

    // update current node index for prediction
    tree.nodes[node_index].node_type = falcon::INTERNAL;
    tree.nodes[node_index].is_self_feature = 0;
    tree.nodes[node_index].best_party_id = recv_best_party_id;
    tree.nodes[node_index].best_feature_id = recv_best_feature_id;
    tree.nodes[node_index].best_split_id = recv_best_split_id;

    tree.nodes[left_child_index].depth = tree.nodes[node_index].depth + 1;
    tree.nodes[left_child_index].impurity = recv_left_impurity;
    tree.nodes[right_child_index].depth = tree.nodes[node_index].depth + 1;
    tree.nodes[right_child_index].impurity = recv_right_impurity;

    available_feature_ids_new = available_feature_ids;
  }

  tree.internal_node_num += 1;
  tree.total_node_num += 1;

  const clock_t node_finish_time = clock();
  double node_consumed_time = double (node_finish_time - node_start_time) / CLOCKS_PER_SEC;
  std::cout << "Node build time = " << node_consumed_time << std::endl;
  LOG(INFO) << "Node build time = " << node_consumed_time;
  google::FlushLogFiles(google::INFO);

  delete [] global_left_branch_sample_nums;
  delete [] global_right_branch_sample_nums;
  delete [] encrypted_left_branch_sample_nums;
  delete [] encrypted_right_branch_sample_nums;
  for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
    delete [] global_encrypted_statistics[i];
  }
  delete [] global_encrypted_statistics;
  for (int i = 0; i < local_splits_num; i++) {
    delete [] encrypted_statistics[i];
  }
  delete [] encrypted_statistics;
  djcs_t_free_public_key(phe_pub_key);

  /// step 9: recursively build the next child tree nodes
  build_node(party,
      left_child_index,
      available_feature_ids_new,
      sample_mask_iv_left,
      encrypted_labels_left);
  build_node(party,
      right_child_index,
      available_feature_ids_new,
      sample_mask_iv_right,
      encrypted_labels_right);

  delete [] sample_mask_iv_left;
  delete [] sample_mask_iv_right;
  delete [] encrypted_labels_left;
  delete [] encrypted_labels_right;
  // LOG(INFO) << "Freed sample iv " << left_child_index << " and " << right_child_index;
  LOG(INFO) << "End build tree node: " << node_index;
}

bool DecisionTreeBuilder::check_pruning_conditions(Party &party,
    int node_index,
    EncodedNumber *sample_mask_iv) {
  bool is_satisfied = false;

  // if current split depth is already equal to max_depth, return.
  if (tree.nodes[node_index].depth == max_depth) {
    is_satisfied = true;
    return is_satisfied;
  }

  int sample_num = training_data.size();
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // init temp values
  auto *encrypted_sample_count = new EncodedNumber[1];
  auto *encrypted_impurity = new EncodedNumber[1];
  std::vector<int> public_values;
  std::vector<double> private_values;
  std::vector<double> condition_shares1, condition_shares2;

  // parties check the pruning conditions by SPDZ
  // 1. the number of samples is less than a threshold
  // 2. the number of class is 1 (impurity == 0) or variance less than a threshold
  falcon::SpdzTreeCompType comp_type = falcon::PRUNING_CHECK;
  public_values.push_back(tree_type);
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // compute the available sample_num as well as the impurity of the current node
    encrypted_sample_count[0].set_integer(phe_pub_key->n[0], 0);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
        encrypted_sample_count[0], encrypted_sample_count[0]);
    for (int i = 0; i < sample_num; i++) {
      djcs_t_aux_ee_add(phe_pub_key, encrypted_sample_count[0],
          encrypted_sample_count[0], sample_mask_iv[i]);
    }
    encrypted_impurity[0] = tree.nodes[node_index].impurity;
    party.ciphers_to_secret_shares(encrypted_sample_count, condition_shares1, 1,
        ACTIVE_PARTY_ID, 0);
    party.ciphers_to_secret_shares(encrypted_impurity, condition_shares2, 1,
        ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  } else {
    party.ciphers_to_secret_shares(encrypted_sample_count, condition_shares1, 1,
        ACTIVE_PARTY_ID, 0);
    party.ciphers_to_secret_shares(encrypted_impurity, condition_shares2, 1,
        ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  }
  private_values.push_back(condition_shares1[0]);
  private_values.push_back(condition_shares2[0]);

  // print executor_mpc_ports for debug
  for (int i = 0; i < party.executor_mpc_ports.size(); i++) {
    log_info("[check_pruning_conditions]: party.executor_mpc_ports[" + std::to_string(i) + "] = " + std::to_string(party.executor_mpc_ports[i]));
  }

  // check if encrypted sample count is less than a threshold (prune_sample_num),
  // or if the impurity satisfies the pruning condition
  // communicate with spdz parties and receive results
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  std::thread spdz_pruning_check_thread(spdz_tree_computation,
      party.party_num,
      party.party_id,
      party.executor_mpc_ports,
      party.host_names,
      public_values.size(),
      public_values,
      private_values.size(),
      private_values,
      comp_type,
      &promise_values);
  std::vector<double> res = future_values.get();
  spdz_pruning_check_thread.join();
  // if returned value is 1, then it is a leaf node, need to compute label
  // otherwise, it is not a leaf node, need to find the best split
  if ((int) res[0] == 1) {
    is_satisfied = true;
  }
  delete [] encrypted_sample_count;
  delete [] encrypted_impurity;
  djcs_t_free_public_key(phe_pub_key);
  return is_satisfied;
}

void DecisionTreeBuilder::compute_leaf_statistics(Party &party,
    int node_index,
    EncodedNumber *sample_mask_iv,
    EncodedNumber *encrypted_labels,
    bool use_sample_weights,
    EncodedNumber *encrypted_weights) {
  int sample_num = (int) training_data.size();
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  falcon::SpdzTreeCompType comp_type = falcon::COMPUTE_LABEL;
  std::vector<int> public_values;
  std::vector<double> private_values, shares1, shares2;
  public_values.push_back(tree_type);
  public_values.push_back(class_num);
  // if with encrypted sample weights, can multiply encrypted_labels with
  // encrypted_weights before computing the label by spdz computation
  if (use_sample_weights) {
    // multiply encrypted_weights and set back to encrypted labels
    party.ciphers_multi(encrypted_labels,
                        encrypted_labels,
                        encrypted_weights,
                        sample_num,
                        ACTIVE_PARTY_ID);
    log_info("[compute_leaf_statistics]: encrypted_labels multiply encrypted weights finished.");
  }
  if (party.party_type == falcon::ACTIVE_PARTY) {
    if (tree_type == falcon::CLASSIFICATION) {
      // compute how many samples in each class, eg: [100, 90], 100 samples for c1, and 90 for c2
      auto *class_sample_nums = new EncodedNumber[class_num];
      for (int xx = 0; xx < class_num; xx++) {
        class_sample_nums[xx] = encrypted_labels[xx * sample_num + 0];
        for (int j = 1; j < sample_num; j++) {
          djcs_t_aux_ee_add(phe_pub_key, class_sample_nums[xx],
              class_sample_nums[xx], encrypted_labels[xx * sample_num + j]);
        }
      }
      party.ciphers_to_secret_shares(class_sample_nums, private_values,
          class_num, ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      delete [] class_sample_nums;
    } else {
      // need to compute average label
      auto *label_info = new EncodedNumber[1];
      auto *encrypted_sample_num_aux = new EncodedNumber[1];
      label_info[0] = encrypted_labels[0];
      for (int i = 1; i < sample_num; i++) {
        djcs_t_aux_ee_add(phe_pub_key, label_info[0],
            label_info[0], encrypted_labels[0 * sample_num + i]);
      }
      encrypted_sample_num_aux[0].set_integer(phe_pub_key->n[0], 0);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
          encrypted_sample_num_aux[0], encrypted_sample_num_aux[0]);
      for (int i = 0; i < sample_num; i++) {
        djcs_t_aux_ee_add(phe_pub_key, encrypted_sample_num_aux[0],
            encrypted_sample_num_aux[0], sample_mask_iv[i]);
      }
      party.ciphers_to_secret_shares(label_info, shares1, 1,
          ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      party.ciphers_to_secret_shares(encrypted_sample_num_aux, shares2, 1,
          ACTIVE_PARTY_ID, 0);
      private_values.push_back(shares1[0]);
      private_values.push_back(shares2[0]);
      delete [] label_info;
      delete [] encrypted_sample_num_aux;
    }
  } else { // PASSIVE PARTY
    if (tree_type == falcon::CLASSIFICATION) {
      EncodedNumber *class_sample_nums = new EncodedNumber[class_num];
      party.ciphers_to_secret_shares(class_sample_nums, private_values,
          class_num, ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      delete [] class_sample_nums;
    } else { // REGRESSION
      EncodedNumber *label_info = new EncodedNumber[1];
      EncodedNumber *encrypted_sample_num_aux = new EncodedNumber[1];
      party.ciphers_to_secret_shares(label_info, shares1, 1,
          ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      party.ciphers_to_secret_shares(encrypted_sample_num_aux, shares2, 1,
          ACTIVE_PARTY_ID, 0);
      private_values.push_back(shares1[0]);
      private_values.push_back(shares2[0]);
      delete [] label_info;
      delete [] encrypted_sample_num_aux;
    }
  }

  // communicate with spdz parties and receive results to compute labels
  // first send computation type, tree type, class num
  // then send private values
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  std::thread spdz_pruning_check_thread(spdz_tree_computation,
      party.party_num,
      party.party_id,
      party.executor_mpc_ports,
      party.host_names,
      public_values.size(),
      public_values,
      private_values.size(),
      private_values,
      comp_type,
      &promise_values);
  std::vector<double> res = future_values.get();
  spdz_pruning_check_thread.join();

  LOG(INFO) << "Node " << node_index << " label = " << res[0];
  std::cout << "Node " << node_index << " label = " << res[0] << std::endl;
  EncodedNumber label;
  label.set_double(phe_pub_key->n[0], res[0], PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, label, label);
  // now assume that it is only an encoded number, instead of a ciphertext
  // djcs_t_aux_encrypt(phe_pub_key, party.phe_random, label, label);
  tree.nodes[node_index].label = label;
  tree.nodes[node_index].node_type = falcon::LEAF;
  // the other node attributes should be updated in another place

  djcs_t_free_public_key(phe_pub_key);
}

void DecisionTreeBuilder::compute_encrypted_statistics(const Party &party,
    int node_index,
    std::vector<int> available_feature_ids,
    EncodedNumber *sample_mask_iv,
    EncodedNumber **encrypted_statistics,
    EncodedNumber *encrypted_labels,
    EncodedNumber *encrypted_left_sample_nums,
    EncodedNumber *encrypted_right_sample_nums,
    bool use_sample_weights,
    EncodedNumber *encrypted_weights) {
  std::cout << " Compute encrypted statistics for node " << node_index << std::endl;
  LOG(INFO) << " Compute encrypted statistics for node " << node_index;
  const clock_t start_time = clock();

  int split_index = 0;
  int available_feature_num = (int) available_feature_ids.size();
  int sample_num = (int) training_data.size();
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  auto* weighted_sample_mask_iv = new EncodedNumber[sample_num];
  if (use_sample_weights) {
    // if use encrypted weights, need to execute an element-wise cipher multiplication
    party.ciphers_multi(weighted_sample_mask_iv,
                        sample_mask_iv,
                        encrypted_weights,
                        sample_num,
                        ACTIVE_PARTY_ID);
  } else {
    for (int i = 0; i < sample_num; i++) {
      weighted_sample_mask_iv[i] = sample_mask_iv[i];
    }
  }

  /// splits of features are flatted, classes_num * 2 are for left and right
  /// in this method, the feature values are sorted,
  /// use the sorted indexes to re-organize the encrypted mask vector,
  /// and compute num_splits + 1 bucket statistics by one traverse
  /// then the encrypted statistics for the num_splits
  /// can be aggregated by num_splits homomorphic additions
  // TODO: this function is a little difficult to read, should add more explanations
  for (int j = 0; j < available_feature_num; j++) {
    int feature_id = available_feature_ids[j];
    int split_num = feature_helpers[feature_id].num_splits;
    std::vector<int> sorted_indices = feature_helpers[feature_id].sorted_indexes;
    auto *sorted_sample_iv = new EncodedNumber[sample_num];
    // copy the sample_iv
    for (int idx = 0; idx < sample_num; idx++) {
      sorted_sample_iv[idx] = weighted_sample_mask_iv[sorted_indices[idx]];
    }
//    // add encrypted_weights logic here, first convert sorted_sample_iv
//    // into double EncodedNumber array
//    EncodedNumber pos_constant;
//    pos_constant.set_double(phe_pub_key->n[0],
//                            CERTAIN_PROBABILITY,
//                            PHE_FIXED_POINT_PRECISION);
//    for (int idx = 0; idx < sample_num; idx++) {
//      djcs_t_aux_ep_mul(phe_pub_key,
//                        sorted_sample_iv[idx],
//                        sorted_sample_iv[idx],
//                        pos_constant);
//    }
    // compute the cipher precision
    int sorted_sample_iv_prec = std::abs(sorted_sample_iv[0].getter_exponent());
    log_info("[compute_encrypted_statistics]: sorted_sample_iv_prec = " + std::to_string(sorted_sample_iv_prec));

    // compute the encrypted aggregation of split_num + 1 buckets
    auto *left_sums = new EncodedNumber[split_num];
    auto *right_sums = new EncodedNumber[split_num];
    EncodedNumber total_sum;
    total_sum.set_double(phe_pub_key->n[0], 0.0, sorted_sample_iv_prec);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, total_sum, total_sum);
    for (int idx = 0; idx < split_num; idx++) {
      left_sums[idx].set_double(phe_pub_key->n[0], 0.0, sorted_sample_iv_prec);
      right_sums[idx].set_double(phe_pub_key->n[0], 0.0, sorted_sample_iv_prec);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, left_sums[idx], left_sums[idx]);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, right_sums[idx], right_sums[idx]);
    }
    log_info("[compute_encrypted_statistics]: init finished");
    // compute sample iv statistics by one traverse
    int split_iterator = 0;
    for (int sample_idx = 0; sample_idx < sample_num; sample_idx++) {
      djcs_t_aux_ee_add(phe_pub_key, total_sum,
          total_sum, sorted_sample_iv[sample_idx]);
      if (split_iterator == split_num) {
        continue;
      }
      int sorted_idx = sorted_indices[sample_idx];
      double sorted_feature_value = feature_helpers[feature_id].origin_feature_values[sorted_idx];

      // find the first split value that larger than the current feature value, usually only step by 1
      if ((sorted_feature_value - feature_helpers[feature_id].split_values[split_iterator]) > ROUNDED_PRECISION) {
        split_iterator += 1;
        if (split_iterator == split_num) continue;
      }
      djcs_t_aux_ee_add(phe_pub_key, left_sums[split_iterator],
          left_sums[split_iterator], sorted_sample_iv[sample_idx]);
    }

    // compute the encrypted statistics for each class
    auto **left_stats = new EncodedNumber*[split_num];
    auto **right_stats = new EncodedNumber*[split_num];
    auto *sums_stats = new EncodedNumber[class_num];
    for (int k = 0; k < split_num; k++) {
      left_stats[k] = new EncodedNumber[class_num];
      right_stats[k] = new EncodedNumber[class_num];
    }
    for (int k = 0; k < split_num; k++) {
      for (int c = 0; c < class_num; c++) {
        left_stats[k][c].set_double(phe_pub_key->n[0], 0);
        right_stats[k][c].set_double(phe_pub_key->n[0], 0);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
            left_stats[k][c], left_stats[k][c]);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
            right_stats[k][c], right_stats[k][c]);
      }
    }
    for (int c = 0; c < class_num; c++) {
      sums_stats[c].set_double(phe_pub_key->n[0], 0);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
          sums_stats[c], sums_stats[c]);
    }
    split_iterator = 0;
    for (int sample_idx = 0; sample_idx < sample_num; sample_idx++) {
      int sorted_idx = sorted_indices[sample_idx];
      for (int c = 0; c < class_num; c++) {
        djcs_t_aux_ee_add(phe_pub_key, sums_stats[c], sums_stats[c],
            encrypted_labels[c * sample_num + sorted_idx]);
      }
      if (split_iterator == split_num) {
        continue;
      }
      double sorted_feature_value = feature_helpers[feature_id].origin_feature_values[sorted_idx];

      // find the first split value that larger than
      // the current feature value, usually only step by 1
      if (sorted_feature_value > feature_helpers[feature_id].split_values[split_iterator]) {
        split_iterator += 1;
        if (split_iterator == split_num) continue;
      }
      for (int c = 0; c < class_num; c++) {
        djcs_t_aux_ee_add(phe_pub_key,
            left_stats[split_iterator][c],
            left_stats[split_iterator][c],
            encrypted_labels[c * sample_num + sorted_idx]);
      }
    }

    // write the left sums to encrypted_left_sample_nums and update the right sums
    EncodedNumber left_num_help, right_num_help, plain_constant_help;
    left_num_help.set_double(phe_pub_key->n[0], 0.0, sorted_sample_iv_prec);
    right_num_help.set_double(phe_pub_key->n[0], 0.0, sorted_sample_iv_prec);
    plain_constant_help.set_integer(phe_pub_key->n[0], -1);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
        left_num_help, left_num_help);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
        right_num_help, right_num_help);

    auto *left_stat_help = new EncodedNumber[class_num];
    auto *right_stat_help = new EncodedNumber[class_num];
    for (int c = 0; c < class_num; c++) {
      left_stat_help[c].set_double(phe_pub_key->n[0], 0);
      right_stat_help[c].set_double(phe_pub_key->n[0], 0);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
          left_stat_help[c], left_stat_help[c]);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
          right_stat_help[c], right_stat_help[c]);
    }

    // compute right sample num of the current split by total_sum + (-1) * left_sum_help
    for (int k = 0; k < split_num; k++) {
      djcs_t_aux_ee_add(phe_pub_key,left_num_help, left_num_help, left_sums[k]);
      encrypted_left_sample_nums[split_index] = left_num_help;
      djcs_t_aux_ep_mul(phe_pub_key,right_num_help, left_num_help, plain_constant_help);
      djcs_t_aux_ee_add(phe_pub_key,encrypted_right_sample_nums[split_index], total_sum, right_num_help);
      for (int c = 0; c < class_num; c++) {
        djcs_t_aux_ee_add(phe_pub_key,left_stat_help[c], left_stat_help[c], left_stats[k][c]);
        djcs_t_aux_ep_mul(phe_pub_key,right_stat_help[c], left_stat_help[c], plain_constant_help);
        djcs_t_aux_ee_add(phe_pub_key,right_stat_help[c], right_stat_help[c], sums_stats[c]);
        encrypted_statistics[split_index][2 * c] = left_stat_help[c];
        encrypted_statistics[split_index][2 * c + 1] = right_stat_help[c];
      }
      // update the global split index
      split_index += 1;
    }

    delete [] sorted_sample_iv;
    delete [] left_sums;
    delete [] right_sums;
    for (int k = 0; k < split_num; k++) {
      delete [] left_stats[k];
      delete [] right_stats[k];
    }
    delete [] left_stats;
    delete [] right_stats;
    delete [] sums_stats;
    delete [] left_stat_help;
    delete [] right_stat_help;
  }

  delete [] weighted_sample_mask_iv;
  djcs_t_free_public_key(phe_pub_key);

  const clock_t finish_time = clock();
  double consumed_time = double(finish_time - start_time) / CLOCKS_PER_SEC;
  std::cout << "Node encrypted statistics computation time = " << consumed_time << std::endl;
  LOG(INFO) << "Node encrypted statistics computation time = " << consumed_time;
  google::FlushLogFiles(google::INFO);
}

std::vector<int> DecisionTreeBuilder::compute_binary_vector(int sample_id,
    std::map<int, int> node_index_2_leaf_index,
    falcon::DatasetType eval_type) {
  std::vector< std::vector<double> > cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;

  std::vector<double> sample_values = cur_test_dataset[sample_id];
  std::vector<int> binary_vector = tree.comp_predict_vector(sample_values, node_index_2_leaf_index);
  return binary_vector;
}

void DecisionTreeBuilder::eval(Party party, falcon::DatasetType eval_type,
    const std::string& report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Start *************";
  const clock_t testing_start_time = clock();

  /// Testing procedure:
  /// 1. Organize the leaf label vector, and record the map
  ///     between tree node index and leaf index
  /// 2. For each sample in the testing dataset, search the whole tree
  ///     and do the following:
  ///     2.1 if meet feature that not belong to self, mark 1,
  ///         and iteratively search left and right branches with 1
  ///     2.2 if meet feature that belongs to self, compare with the split value,
  ///         mark satisfied branch with 1 while the other branch with 0,
  ///         and iteratively search left and right branches
  ///     2.3 if meet the leaf node, record the corresponding leaf index
  ///         with current value
  /// 3. After each client obtaining a 0-1 vector of leaf nodes, do the following:
  ///     3.1 the "party_num-1"-th party element-wise multiply with leaf
  ///         label vector, and encrypt the vector, send to the next party
  ///     3.2 every party on the Robin cycle updates the vector
  ///         by element-wise homomorphic multiplication, and send to the next
  ///     3.3 the last party, i.e., client 0 get the final encrypted vector
  ///         and homomorphic add together, call share decryption
  /// 4. If label is matched, correct_num += 1, otherwise, continue
  /// 5. Return the final test accuracy by correct_num / dataset.size()

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // step 1: init test data
  int dataset_size = (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector< std::vector<double> > cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  // step 2: call tree model predict function to obtain predicted_labels
  auto* predicted_labels = new EncodedNumber[dataset_size];
  tree.predict(party, cur_test_dataset, dataset_size, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  auto* decrypted_labels = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(predicted_labels,
      decrypted_labels,
      dataset_size,
      ACTIVE_PARTY_ID);

  // compute accuracy by the super client
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // init predicted_label_vector
    std::vector<double> predicted_label_vector;
    for (int i = 0; i < dataset_size; i++) {
      predicted_label_vector.push_back(0.0);
    }
    for (int i = 0; i < dataset_size; i++) {
      decrypted_labels[i].decode(predicted_label_vector[i]);
    }

    if (tree_type == falcon::CLASSIFICATION) {
      int correct_num = 0;
      for (int i = 0; i < dataset_size; i++) {
        if (predicted_label_vector[i] == cur_test_dataset_labels[i]) {
          correct_num += 1;
        }
      }
      if (eval_type == falcon::TRAIN) {
        training_accuracy = (double) correct_num / dataset_size;
        LOG(INFO) << "Dataset size = " << dataset_size << ", correct predicted num = "
                  << correct_num << ", training accuracy = " << training_accuracy;
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = (double) correct_num / dataset_size;
        LOG(INFO) << "Dataset size = " << dataset_size << ", correct predicted num = "
                  << correct_num << ", testing accuracy = " << testing_accuracy;
      }
    } else {
      if (eval_type == falcon::TRAIN) {
        training_accuracy = mean_squared_error(predicted_label_vector, cur_test_dataset_labels);
        LOG(INFO) << "Training accuracy = " << training_accuracy;
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = mean_squared_error(predicted_label_vector, cur_test_dataset_labels);
        LOG(INFO) << "Testing accuracy = " << testing_accuracy;
      }
    }
  }

  delete [] predicted_labels;
  delete [] decrypted_labels;
  djcs_t_free_public_key(phe_pub_key);

  const clock_t testing_finish_time = clock();
  double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Evaluation time = " << testing_consumed_time;
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Finished *************";
  google::FlushLogFiles(google::INFO);
}


void DecisionTreeBuilder::distributed_train(const Party &party, const Worker &worker) {

  log_info("************* [DT_train_worker.distributed_train]: distributed train Start *************");
  const clock_t training_start_time = clock();

  /// 1. pre-compute label helper and feature helper based on current features
  // calculate r1 and r2, in form of [[1], [0] ...[1], [0]...]
  // indicator_class_vecs and feature_helpers
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();

  int sample_num = train_data_size;
  // label_size = len(indicator_class_vecs), len([r1, r2])
  int label_size = class_num * sample_num;

  std::vector<int> available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    available_feature_ids.push_back(i);
  }
  log_info("[DT_train_worker.distributed_train]: step 1, finished init label and features");

  /// 2. init mask vector and encrypted labels
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  auto * sample_mask_iv = new EncodedNumber[sample_num];
  // because all samples are available at the beginning of the training, init with 1
  EncodedNumber tmp_mask;
  tmp_mask.set_integer(phe_pub_key->n[0], 1);
  // init encrypted mask vector on the root node
  for (int i = 0; i < sample_num; i++) {
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, sample_mask_iv[i], tmp_mask);
  }

  // if active party, compute the encrypted label info and broadcast and if passive, receive it
  auto * encrypted_labels = new EncodedNumber[class_num * sample_num];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::string encrypted_labels_str;
    for (int i = 0; i < class_num; i++) {
      for (int j = 0; j < sample_num; j++) {
        EncodedNumber tmp_label;
        // classification use indicator_class_vecs, regression use variance_stat_vecs
        if (tree_type == falcon::CLASSIFICATION) {
          tmp_label.set_double(phe_pub_key->n[0], indicator_class_vecs[i][j]);
        } else {
          tmp_label.set_double(phe_pub_key->n[0], variance_stat_vecs[i][j]);
        }
        // encrypt the label
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           encrypted_labels[i * sample_num + j], tmp_label);
      }
    }

    // serialize and send to the other client
    serialize_encoded_number_array(encrypted_labels, label_size, encrypted_labels_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, encrypted_labels_str);
      }
    }
  }
  // if not active party, receive the encrypted label info
  else {
    std::string recv_result_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_result_str);
    deserialize_encoded_number_array(encrypted_labels, label_size, recv_result_str);
  }

  log_info("[DT_train_worker.distributed_train]: step 2, finished broadcasting the encrypted label info and init mask vector");

  /// 3. init the root node info
  tree.nodes[0].depth = 0;
  EncodedNumber max_impurity;
  max_impurity.set_double(phe_pub_key->n[0], MAX_IMPURITY, PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, max_impurity);

  log_info("[DT_train_worker.distributed_train]: step 3, finished init root node info");

  /// 4. begin to train
  int iter = 0;
  int node_index;

  while (true){
    const clock_t node_start_time = clock();

    std::string received_str;
    worker.recv_long_message_from_ps(received_str);
    // only break after receiving ps's stop message
    if (received_str == "stop"){
      log_info("[DT_train_worker.distributed_train]: step 4.1, -------- Worker stop training at Iteration " + std::to_string(iter)
                +" (depth = " + to_string(tree.nodes[node_index].depth) + ")" + "-------- ");
      break;
    }else{
      node_index = std::stoi( received_str );
      log_info("[DT_train_worker.distributed_train]: step 4.1, -------- Worker update node index :"+
                received_str + "at Iteration " + std::to_string(iter) + "-------- ");
    }

    /// 4.2. calculate local splits num for all features, and compute local encrypted statistics
    int local_splits_num = 0;
    for (int feature_id : available_feature_ids) {
      local_splits_num = local_splits_num + feature_helpers[feature_id].num_splits;
    }
    EncodedNumber **encrypted_statistics;
    // n1 and n2
    EncodedNumber *encrypted_left_branch_sample_nums, *encrypted_right_branch_sample_nums;

    if (local_splits_num != 0) {
      encrypted_statistics = new EncodedNumber*[local_splits_num];
      for (int i = 0; i < local_splits_num; i++) {
        // here 2 * class_num refers to left branch and right branch
        encrypted_statistics[i] = new EncodedNumber[2 * class_num];
      }
      encrypted_left_branch_sample_nums = new EncodedNumber[local_splits_num];
      encrypted_right_branch_sample_nums = new EncodedNumber[local_splits_num];
      // call compute function
      compute_encrypted_statistics(party, node_index,
                                   available_feature_ids,
                                   sample_mask_iv,
                                   encrypted_statistics,
                                   encrypted_labels,
                                   encrypted_left_branch_sample_nums,
                                   encrypted_right_branch_sample_nums);
    }
    log_info("[DT_train_worker.distributed_train]: step 4.2, Finished computing local statistics");

    /// 4.3. compare and find the best gain using MPC.
    // the global encrypted statistics for all possible splits
    EncodedNumber **global_encrypted_statistics;
    EncodedNumber *global_left_branch_sample_nums, *global_right_branch_sample_nums;
    std::vector<int> party_split_nums;
    int global_split_num = 0;
    global_encrypted_statistics = new EncodedNumber*[MAX_GLOBAL_SPLIT_NUM];
    for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
      global_encrypted_statistics[i] = new EncodedNumber[2 * class_num];
    }
    global_left_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];
    global_right_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];

    if (party.party_type == falcon::ACTIVE_PARTY) {
      // pack self
      if (local_splits_num == 0) {
        party_split_nums.push_back(0);
      } else {
        party_split_nums.push_back(local_splits_num);
        for (int i = 0; i < local_splits_num; i++) {
          global_left_branch_sample_nums[i] = encrypted_left_branch_sample_nums[i];
          global_right_branch_sample_nums[i] = encrypted_right_branch_sample_nums[i];
          for (int j = 0; j < 2 * class_num; j++) {
            global_encrypted_statistics[i][j] = encrypted_statistics[i][j];
          }
        }
      }
      global_split_num += local_splits_num;

      // receive from the other clients of the encrypted statistics
      for (int i = 0; i < party.party_num; i++) {
        std::string recv_encrypted_statistics_str, recv_local_split_num_str;
        if (i != party.party_id) {
          party.recv_long_message(i, recv_local_split_num_str);
          party.recv_long_message(i, recv_encrypted_statistics_str);
          int recv_party_id, recv_node_index, recv_split_num, recv_classes_num;
          recv_split_num = std::stoi(recv_local_split_num_str);
          // pack the encrypted statistics
          if (recv_split_num == 0) {
            party_split_nums.push_back(0);
            continue;
          }

          if (recv_split_num != 0) {
            auto **recv_encrypted_statistics = new EncodedNumber*[recv_split_num];
            for (int s = 0; s < recv_split_num; s++) {
              recv_encrypted_statistics[s] = new EncodedNumber[2 * class_num];
            }
            auto *recv_left_sample_nums = new EncodedNumber[recv_split_num];
            auto *recv_right_sample_nums = new EncodedNumber[recv_split_num];
            deserialize_encrypted_statistics(recv_party_id, recv_node_index,
                                             recv_split_num, recv_classes_num,
                                             recv_left_sample_nums, recv_right_sample_nums,
                                             recv_encrypted_statistics,
                                             recv_encrypted_statistics_str);

            party_split_nums.push_back(recv_split_num);
            for (int j = 0; j < recv_split_num; j++) {
              global_left_branch_sample_nums[global_split_num + j] = recv_left_sample_nums[j];
              global_right_branch_sample_nums[global_split_num + j] = recv_right_sample_nums[j];
              for (int k = 0; k < 2 * class_num; k++) {
                global_encrypted_statistics[global_split_num + j][k] = recv_encrypted_statistics[j][k];
              }
            }
            global_split_num += recv_split_num;

            delete [] recv_left_sample_nums;
            delete [] recv_right_sample_nums;
            for (int xx = 0; xx < recv_split_num; xx++) {
              delete [] recv_encrypted_statistics[xx];
            }
            delete [] recv_encrypted_statistics;
          }
        }
      }
      log_info("The global_split_num = " + to_string(global_split_num));
      // send the total number of splits for the other clients to generate secret shares
      //logger(logger_out, "Send global split num to the other clients\n");
      std::string split_info_str;
      serialize_split_info(global_split_num, party_split_nums, split_info_str);
      for (int i = 0; i < party.party_num; i++) {
        if (i != party.party_id) {
          party.send_long_message(i, split_info_str);
        }
      }
    }

    if (party.party_type == falcon::PASSIVE_PARTY) {
      // first send local split num
      log_info("Local split num = " + to_string(local_splits_num));
      std::string local_split_num_str = std::to_string(local_splits_num);
      party.send_long_message(ACTIVE_PARTY_ID, local_split_num_str);
      // then send the encrypted statistics
      std::string encrypted_statistics_str;
      serialize_encrypted_statistics(party.party_id, node_index,
                                     local_splits_num, class_num,
                                     encrypted_left_branch_sample_nums,
                                     encrypted_right_branch_sample_nums,
                                     encrypted_statistics, encrypted_statistics_str);
      party.send_long_message(ACTIVE_PARTY_ID, encrypted_statistics_str);

      // recv the split info from active party
      std::string recv_split_info_str;
      party.recv_long_message(0, recv_split_info_str);
      deserialize_split_info(global_split_num, party_split_nums, recv_split_info_str);
      log_info("The global_split_num = " + to_string(global_split_num));

    }

    log_info("[DT_train_worker.distributed_train]: step 4.3, Finished sync the local statistics");

    /// step 4.4: encrypted statistics computed finished, convert to secret shares
    std::vector< std::vector<double> > stats_shares;
    std::vector<double> left_sample_nums_shares;
    std::vector<double> right_sample_nums_shares;
    std::vector<int> public_values;
    std::vector<double> private_values;
    int branch_sample_nums_prec;
    if (party.party_type == falcon::ACTIVE_PARTY) {
      branch_sample_nums_prec = std::abs(global_left_branch_sample_nums[0].getter_exponent());
      for (int id = 0; id < party.party_num; id++) {
        if (id != party.party_id) {
          party.send_long_message(id, std::to_string(branch_sample_nums_prec));
        }
      }
    } else {
      std::string recv_prec;
      party.recv_long_message(ACTIVE_PARTY_ID, recv_prec);
      branch_sample_nums_prec = std::stoi(recv_prec);
    }
    log_info("[build_node]: branch_sample_nums_prec = " + std::to_string(branch_sample_nums_prec));
    party.ciphers_to_secret_shares(global_left_branch_sample_nums,
                                   left_sample_nums_shares, global_split_num,
                                   ACTIVE_PARTY_ID, branch_sample_nums_prec);
    party.ciphers_to_secret_shares(global_right_branch_sample_nums,
                                   right_sample_nums_shares, global_split_num,
                                   ACTIVE_PARTY_ID, branch_sample_nums_prec);
    for (int i = 0; i < global_split_num; i++) {
      std::vector<double> tmp;
      party.ciphers_to_secret_shares(global_encrypted_statistics[i],
                                     tmp, 2 * class_num,
                                     ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      stats_shares.push_back(tmp);
    }

    log_info("[DT_train_worker.distributed_train]: step 4.4, Finished conversion of secret shares");

    /// step 4.5: secret shares conversion finished, talk to spdz for computation
    // communicate with spdz parties and receive results to compute labels
    // first send computation type, tree type, class num
    // then send private values

    // arrange public values and private values
    public_values.push_back(tree_type);
    public_values.push_back(global_split_num);
    public_values.push_back(class_num);
    public_values.push_back(class_num);

    // according to the spdz program,
    // first send the encrypted_statistics: global_split_num * (2 * class_num)
    // then send the left shares, and finally the right shares: global_split_num
    for (int i = 0; i < global_split_num; i++) {
      for (int j = 0; j < 2 * class_num; j++) {
        private_values.push_back(stats_shares[i][j]);
      }
    }
    for (int i = 0; i < global_split_num; i++) {
      private_values.push_back(left_sample_nums_shares[i]);
    }
    for (int i = 0; i < global_split_num; i++) {
      private_values.push_back(right_sample_nums_shares[i]);
    }

    // required by spdz connector and mpc computation
    bigint::init_thread();

    falcon::SpdzTreeCompType comp_type = falcon::FIND_BEST_SPLIT;
    std::promise<std::vector<double>> promise_values;
    std::future<std::vector<double>> future_values = promise_values.get_future();
    std::thread spdz_pruning_check_thread(spdz_tree_computation,
                                          party.party_num,
                                          party.party_id,
                                          party.executor_mpc_ports,
                                          party.host_names,
                                          public_values.size(),
                                          public_values,
                                          private_values.size(),
                                          private_values,
                                          comp_type,
                                          &promise_values);
    std::vector<double> res = future_values.get();
    spdz_pruning_check_thread.join();

    res.push_back(worker.worker_id-1);
    log_info("[DT_train_worker.distributed_train]: step 4.5, Finished computation with spdz");

    /// step 4.6. send to ps and receive the best split for this node
    std::string local_best_split_str;
    serialize_double_array(res, local_best_split_str);
    worker.send_long_message_to_ps(local_best_split_str);

    // only worker 1 help to do the compare
//    if (worker.worker_id == 1){
//      retrieve_global_best_split(worker);
//    }

    std::vector<double> final_decision_res;
    std::string final_decision_str;
    worker.recv_long_message_from_ps(final_decision_str);
    deserialize_double_array(final_decision_res, final_decision_str);

    // the result values are as follows (assume public in this version):
    // best_split_index (global), best_left_impurity, and best_right_impurity
    int best_split_index = (int) final_decision_res[0];
    double left_impurity = final_decision_res[1];
    double right_impurity = final_decision_res[2];
    int best_split_worker_id = final_decision_res[3];

    log_info("[DT_train_worker.distributed_train]: step 4.6, Finished sending local best to ps and received the global best one: "
                 "best_split_index is " + to_string(best_split_index) +
                 "left_impurity is " + to_string(left_impurity) +
                 "right_impurity is " + to_string(right_impurity) +
                 "best_split_worker_id is " + to_string(best_split_worker_id) );

    // encrypted left and right impurity
    EncodedNumber encrypted_left_impurity, encrypted_right_impurity;
    encrypted_left_impurity.set_double(phe_pub_key->n[0],
                                       left_impurity, PHE_FIXED_POINT_PRECISION);
    encrypted_right_impurity.set_double(phe_pub_key->n[0],
                                        right_impurity, PHE_FIXED_POINT_PRECISION);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                       encrypted_left_impurity, encrypted_left_impurity);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                       encrypted_right_impurity, encrypted_right_impurity);

    /// step 4.7: update tree nodes, including sample iv for iterative node computation

    std::vector<int> available_feature_ids_new;
    auto *sample_mask_iv_left = new EncodedNumber[sample_num];
    auto *sample_mask_iv_right = new EncodedNumber[sample_num];
    auto *encrypted_labels_left = new EncodedNumber[sample_num * class_num];
    auto *encrypted_labels_right = new EncodedNumber[sample_num * class_num];

    int left_child_index = 2 * node_index + 1;
    int right_child_index = 2 * node_index + 2;

    /// step 4.8: find party-id, for this global best split
    // convert the index_in_global_split_num to (i_*, index_*)
    int i_star = -1;
    int index_star = -1;
    int index_tmp = best_split_index;
    for (int i = 0; i < party_split_nums.size(); i++) {
      if (index_tmp < party_split_nums[i]) {
        i_star = i;
        index_star = index_tmp;
        break;
      } else {
        index_tmp = index_tmp - party_split_nums[i];
      }
    }

    log_info("[DT_train_worker.distributed_train]: step 4.8, Best split party: i_star = " + to_string(i_star));

    // send party_id to ps
    worker.send_long_message_to_ps(to_string(i_star));

    /// step 4.9: find feature-id and split-it for this global best split, logic is different for different role

    // if the party has the best split:
    // compute locally and broadcast, find the j_* feature and s_* split
    if (i_star == party.party_id && best_split_worker_id+1 == worker.worker_id) {

      int j_star = -1;
      int s_star = -1;
      int index_star_tmp = index_star;

      for (int feature_id : available_feature_ids) {
        if (index_star_tmp < feature_helpers[feature_id].num_splits) {
          j_star = feature_id;
          s_star = index_star_tmp;
          break;
        } else {
          index_star_tmp = index_star_tmp - feature_helpers[feature_id].num_splits;
        }
      }

      log_info("[DT_train_worker.distributed_train]: step 4.9, i_str and worker are matched with best split,"
                "the Best feature id: j_star = " + to_string(j_star) +
                "Best split id: s_star = " + to_string(s_star));

      // now we have (i_*, j_*, s_*), retrieve s_*-th split ivs and update sample_ivs of two branches
      // update current node index for prediction
      tree.nodes[node_index].node_type = falcon::INTERNAL;
      tree.nodes[node_index].is_self_feature = 1;
      tree.nodes[node_index].best_party_id = i_star;
      tree.nodes[node_index].best_feature_id = j_star;
      tree.nodes[node_index].best_split_id = s_star;
      tree.nodes[node_index].split_threshold = feature_helpers[j_star].split_values[s_star];

      tree.nodes[left_child_index].depth = tree.nodes[node_index].depth + 1;
      tree.nodes[left_child_index].impurity = encrypted_left_impurity;
      tree.nodes[right_child_index].depth = tree.nodes[node_index].depth + 1;
      tree.nodes[right_child_index].impurity = encrypted_right_impurity;

      log_info("[DT_train_worker.distributed_train]: step 4.9, matched worker of matched party finish updating tree, "
               "node_index:" + to_string(node_index) +
          "best_party_id:" + to_string(i_star) +
          "recv_best_feature_id) :" + to_string(j_star) +
          "best_split_id:" + to_string(s_star));

      // match worker update mask and labels info, and send to both ps and other party
      // update available features
      for (int feature_id : available_feature_ids) {
        if (j_star != feature_id) {
          available_feature_ids_new.push_back(feature_id);
        }
      }

      // compute between split_iv and sample_iv and update
      std::vector<int> split_left_iv = feature_helpers[j_star].split_ivs_left[s_star];
      std::vector<int> split_right_iv = feature_helpers[j_star].split_ivs_right[s_star];
      for (int i = 0; i < sample_num; i++) {
        EncodedNumber left, right;
        left.set_integer(phe_pub_key->n[0], split_left_iv[i]);
        right.set_integer(phe_pub_key->n[0], split_right_iv[i]);
        djcs_t_aux_ep_mul(phe_pub_key, sample_mask_iv_left[i], sample_mask_iv[i], left);
        djcs_t_aux_ep_mul(phe_pub_key, sample_mask_iv_right[i], sample_mask_iv[i], right);
      }

      // serialize and send masks to the other clients
      std::string update_str_sample_iv;
      serialize_update_info(party.party_id, party.party_id, j_star, s_star,
                            encrypted_left_impurity, encrypted_right_impurity,
                            sample_mask_iv_left, sample_mask_iv_right,
                            sample_num, update_str_sample_iv);
      for (int i = 0; i < party.party_num; i++) {
        if (i != party.party_id) {
          party.send_long_message(i, update_str_sample_iv);
        }
      }

      // send to ps
      worker.send_long_message_to_ps(update_str_sample_iv);

      // print sample_mask_iv_right[sample_num-1] info for debug
      log_info("[DT_train_worker.distributed_train]: step 4.9, sample_num = " + to_string(sample_num));
      log_info("[DT_train_worker.distributed_train]: step 4.9, sample_mask_iv_right[sample_num-1].exponent = " + to_string(sample_mask_iv_right[sample_num-1].getter_exponent()));
      mpz_t t;
      mpz_init(t);
      sample_mask_iv_right[sample_num-1].getter_n(t);
      gmp_printf("[DT_train_worker.distributed_train]: step 4.9, sample_mask_iv_right[sample_num-1].n = %Zd", t);
      mpz_clear(t);

      // compute between split_iv and encrypted_labels and update
      for (int i = 0; i < class_num; i++) {
        for (int j = 0; j < sample_num; j++) {
          EncodedNumber left, right;
          left.set_integer(phe_pub_key->n[0], split_left_iv[j]);
          right.set_integer(phe_pub_key->n[0], split_right_iv[j]);
          djcs_t_aux_ep_mul(phe_pub_key, encrypted_labels_left[i * sample_num + j],
                            encrypted_labels[i * sample_num + j], left);
          djcs_t_aux_ep_mul(phe_pub_key, encrypted_labels_right[i * sample_num + j],
                            encrypted_labels[i * sample_num + j], right);
        }
      }
      // serialize and send to the other client
      std::string update_str_encrypted_labels_left, update_str_encrypted_labels_right;

      log_info("[DT_train_worker.distributed_train]: step 4.9, class num: " + to_string(class_num ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, sample_num: " + to_string(sample_num ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, size of encrypted_labels_right: " + to_string(class_num * sample_num));

      serialize_encoded_number_array(encrypted_labels_left, class_num * sample_num,
                                     update_str_encrypted_labels_left);
      serialize_encoded_number_array(encrypted_labels_right, class_num * sample_num,
                                     update_str_encrypted_labels_right);

      log_info("[DT_train_worker.distributed_train]: step 4.9, matched worker of match party send update_str_sample_iv size: " + to_string(update_str_sample_iv.size() ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, matched worker of match party send update_str_encrypted_labels_left size: " + to_string(update_str_encrypted_labels_left.size() ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, matched worker of match party send update_str_encrypted_labels_right size: " + to_string(update_str_encrypted_labels_right.size() ));


      for (int i = 0; i < party.party_num; i++) {
        if (i != party.party_id) {
          party.send_long_message(i, update_str_encrypted_labels_left);
          party.send_long_message(i, update_str_encrypted_labels_right);
        }
      }

      worker.send_long_message_to_ps(update_str_encrypted_labels_left);
      worker.send_long_message_to_ps(update_str_encrypted_labels_right);

    }

    // every other party update the local tree model
    if (i_star == party.party_id && best_split_worker_id+1 != worker.worker_id) {

      log_info("[DT_train_worker.distributed_train]: step 4.9, party_id is matched with best split, but worker_id is not matched"
               ", so receive mask and label info from ps and send them to other party");

      // receive i_star from ps and sent it to other client and update
      std::string recv_update_str_sample_iv, recv_update_str_encrypted_labels_left, recv_update_str_encrypted_labels_right;
      worker.recv_long_message_from_ps( recv_update_str_sample_iv);
      worker.recv_long_message_from_ps( recv_update_str_encrypted_labels_left);
      worker.recv_long_message_from_ps( recv_update_str_encrypted_labels_right);

      log_info("[DT_train_worker.distributed_train]: step 4.9, other worker of matched party receive from ps recv_update_str_sample_iv size: " + to_string(recv_update_str_sample_iv.size() ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, other worker of matched party receive from ps recv_update_str_encrypted_labels_left size: " + to_string(recv_update_str_encrypted_labels_left.size() ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, other worker of matched party receive from ps recv_update_str_encrypted_labels_right size: " + to_string(recv_update_str_encrypted_labels_right.size() ));

      // send to other party
      for (int i = 0; i < party.party_num; i++) {
        if (i != party.party_id) {
          party.send_long_message(i, recv_update_str_sample_iv);
          party.send_long_message(i, recv_update_str_encrypted_labels_left);
          party.send_long_message(i, recv_update_str_encrypted_labels_right);
        }
      }

      // deserialize and update sample iv
      int recv_source_party_id, recv_best_party_id, recv_best_feature_id, recv_best_split_id;
      EncodedNumber recv_left_impurity, recv_right_impurity;
      deserialize_update_info(recv_source_party_id, recv_best_party_id,
                              recv_best_feature_id, recv_best_split_id,
                              recv_left_impurity, recv_right_impurity,
                              sample_mask_iv_left, sample_mask_iv_right,
                              recv_update_str_sample_iv);

      deserialize_encoded_number_array(encrypted_labels_left, class_num * sample_num, recv_update_str_encrypted_labels_left);
      deserialize_encoded_number_array(encrypted_labels_right, class_num * sample_num, recv_update_str_encrypted_labels_right);

      // update current node index for prediction
      tree.nodes[node_index].node_type = falcon::INTERNAL;
      tree.nodes[node_index].is_self_feature = 0;
      tree.nodes[node_index].best_party_id = recv_best_party_id;
      tree.nodes[node_index].best_feature_id = recv_best_feature_id;
      tree.nodes[node_index].best_split_id = recv_best_split_id;

      tree.nodes[left_child_index].depth = tree.nodes[node_index].depth + 1;
      tree.nodes[left_child_index].impurity = recv_left_impurity;
      tree.nodes[right_child_index].depth = tree.nodes[node_index].depth + 1;
      tree.nodes[right_child_index].impurity = recv_right_impurity;

      available_feature_ids_new = available_feature_ids;

      log_info("[DT_train_worker.distributed_train]: step 4.9, other worker of matched party finish updating tree, "
               "node_index:" + to_string(node_index) +
          "best_party_id:" + to_string(recv_best_party_id) +
          "recv_best_feature_id) :" + to_string(recv_best_feature_id) +
          "best_split_id:" + to_string(recv_best_split_id));
    }

    if (i_star != party.party_id) {

      log_info("[DT_train_worker.distributed_train]: step 4.9, party_id is not matched"
               ", so receive mask and label info from matched party");
      // receive from i_star client and update
      std::string recv_update_str_sample_iv, recv_update_str_encrypted_labels_left, recv_update_str_encrypted_labels_right;
      party.recv_long_message(i_star, recv_update_str_sample_iv);
      party.recv_long_message(i_star, recv_update_str_encrypted_labels_left);
      party.recv_long_message(i_star, recv_update_str_encrypted_labels_right);

      log_info("[DT_train_worker.distributed_train]: step 4.9, other party receive from active party recv_update_str_sample_iv size: " + to_string(recv_update_str_sample_iv.size() ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, other party receive from active party recv_update_str_encrypted_labels_left size: " + to_string(recv_update_str_encrypted_labels_left.size() ));
      log_info("[DT_train_worker.distributed_train]: step 4.9, other party receive from active party recv_update_str_encrypted_labels_right size: " + to_string(recv_update_str_encrypted_labels_right.size() ));

      worker.send_long_message_to_ps(recv_update_str_sample_iv);
      worker.send_long_message_to_ps(recv_update_str_encrypted_labels_left);
      worker.send_long_message_to_ps(recv_update_str_encrypted_labels_right);

      // deserialize and update sample iv
      int recv_source_party_id, recv_best_party_id, recv_best_feature_id, recv_best_split_id;
      EncodedNumber recv_left_impurity, recv_right_impurity;
      deserialize_update_info(recv_source_party_id, recv_best_party_id,
                              recv_best_feature_id, recv_best_split_id,
                              recv_left_impurity, recv_right_impurity,
                              sample_mask_iv_left, sample_mask_iv_right,
                              recv_update_str_sample_iv);

      deserialize_encoded_number_array(encrypted_labels_left, class_num * sample_num, recv_update_str_encrypted_labels_left);
      deserialize_encoded_number_array(encrypted_labels_right, class_num * sample_num, recv_update_str_encrypted_labels_right);

      // update current node index for prediction
      tree.nodes[node_index].node_type = falcon::INTERNAL;
      tree.nodes[node_index].is_self_feature = 0;
      tree.nodes[node_index].best_party_id = recv_best_party_id;
      tree.nodes[node_index].best_feature_id = recv_best_feature_id;
      tree.nodes[node_index].best_split_id = recv_best_split_id;

      tree.nodes[left_child_index].depth = tree.nodes[node_index].depth + 1;
      tree.nodes[left_child_index].impurity = recv_left_impurity;
      tree.nodes[right_child_index].depth = tree.nodes[node_index].depth + 1;
      tree.nodes[right_child_index].impurity = recv_right_impurity;

      available_feature_ids_new = available_feature_ids;

      log_info("[DT_train_worker.distributed_train]: step 4.9, any worker of other party finish updating tree, "
               "node_index:" + to_string(node_index) +
          "best_party_id:" + to_string(recv_best_party_id) +
          "recv_best_feature_id) :" + to_string(recv_best_feature_id) +
          "best_split_id:" + to_string(recv_best_split_id));
    }

    tree.internal_node_num += 1;
    tree.total_node_num += 1;

    /// step 5: clear and log time used

    const clock_t node_finish_time = clock();
    double node_consumed_time = double (node_finish_time - node_start_time) / CLOCKS_PER_SEC;

    log_info("[DT_train_worker.distributed_train]: step 5, time used" + to_string(node_consumed_time));

    // clear at this iteration
    delete [] global_left_branch_sample_nums;
    delete [] global_right_branch_sample_nums;
    delete [] encrypted_left_branch_sample_nums;
    delete [] encrypted_right_branch_sample_nums;
    for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
      delete [] global_encrypted_statistics[i];
    }
    delete [] global_encrypted_statistics;
    for (int i = 0; i < local_splits_num; i++) {
      delete [] encrypted_statistics[i];
    }
    delete [] encrypted_statistics;
  }

  djcs_t_free_public_key(phe_pub_key);

}



void DecisionTreeBuilder::distributed_eval(Party &party,
                                           const Worker &worker,
                                           falcon::DatasetType eval_type) {

  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("[DT_train_worker.distributed_eval]: Evaluation on " + dataset_str + "Start ");
  const clock_t testing_start_time = clock();

  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  /// step 1: init full dataset, used dataset.

  // read from local
  int full_dataset_size = (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector<std::vector<double> > full_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> full_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  // receive used data index from parameter server
  std::string used_indexes_str;
  worker.recv_long_message_from_ps(used_indexes_str);
  std::vector<int> sample_indexes;
  deserialize_int_array(sample_indexes, used_indexes_str);

  log_info("[DT_train_worker.distributed_eval]: "
           "Worker Iteration , worker.receive sample id success, batch size "
               + std::to_string(sample_indexes.size()));

  // generate used vector
  std::vector<std::vector<double> > cur_test_dataset;
  for (int index : sample_indexes) {
    cur_test_dataset.push_back(full_dataset[index]);
  }

  std::vector<double> cur_test_dataset_labels;
  for (int index : sample_indexes) {
    cur_test_dataset_labels.push_back(full_dataset_labels[index]);
  }

  // get dataset_size
  int dataset_size = (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();

  /// step 2: call tree model predict function to obtain predicted_labels
  auto *predicted_labels = new EncodedNumber[dataset_size];
  tree.predict(party, cur_test_dataset, dataset_size, predicted_labels);

  /// step 3: active party aggregates and call collaborative decryption
  auto *decrypted_labels = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(predicted_labels,
                              decrypted_labels,
                              dataset_size,
                              ACTIVE_PARTY_ID);

  // compute accuracy by the super client
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // init predicted_label_vector
    std::vector<double> predicted_label_vector;
    for (int i = 0; i < dataset_size; i++) {
      predicted_label_vector.push_back(0.0);
    }
    for (int i = 0; i < dataset_size; i++) {
      decrypted_labels[i].decode(predicted_label_vector[i]);
    }

    if (tree_type == falcon::CLASSIFICATION) {
      int correct_num = 0;
      for (int i = 0; i < dataset_size; i++) {
        if (predicted_label_vector[i] == cur_test_dataset_labels[i]) {
          correct_num += 1;
        }
      }
      if (eval_type == falcon::TRAIN) {
        training_accuracy = (double) correct_num / full_dataset_size;
        LOG(INFO) << "Dataset size = " << full_dataset_size << ", correct predicted num = "
                  << correct_num << ", training accuracy = " << training_accuracy;
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = (double) correct_num / full_dataset_size;
        LOG(INFO) << "Dataset size = " << full_dataset_size << ", correct predicted num = "
                  << correct_num << ", testing accuracy = " << testing_accuracy;
      }
    } else {

      assert(predicted_label_vector.size() == cur_test_dataset_labels.size());
      double squared_error = 0.0;
      for (int i = 0; i < dataset_size; i++) {
        squared_error = squared_error + (predicted_label_vector[i] - cur_test_dataset_labels[i]) *
            (predicted_label_vector[i] - cur_test_dataset_labels[i]);
      }
      if (eval_type == falcon::TRAIN) {
        training_accuracy = squared_error / full_dataset_size;
        LOG(INFO) << "Training accuracy = " << training_accuracy;
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = squared_error / full_dataset_size;
        LOG(INFO) << "Testing accuracy = " << testing_accuracy;
      }
    }

    if (eval_type == falcon::TRAIN) {
      worker.send_long_message_to_ps(to_string(training_accuracy));
    }
    if (eval_type == falcon::TEST) {
      worker.send_long_message_to_ps(to_string(testing_accuracy));

    }

    delete[] predicted_labels;
    delete[] decrypted_labels;
    djcs_t_free_public_key(phe_pub_key);

    const clock_t testing_finish_time = clock();
    double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
    LOG(INFO) << "Evaluation time = " << testing_consumed_time;
    LOG(INFO) << "************* Evaluation on " << dataset_str << " Finished *************";
    google::FlushLogFiles(google::INFO);

  }
}


//void DecisionTreeBuilder::retrieve_global_best_split(const Worker &worker){
//
//  std::string received_str;
//  worker.recv_long_message_from_ps(received_str);
//
//  log_info("[DT_train_worker.retrieve_global_best_split]: number of workers:" + received_str);
//
//  std::vector< vector <double> > local_best_splits_vector;
//
//  for ( int i = 0; i< std::stoi(received_str); i++){
//    std::string received_encoded_msg;
//    std::vector<double> local_best_split;
//
//    worker.recv_long_message_from_ps(received_encoded_msg);
//    deserialize_double_array(local_best_split, received_encoded_msg);
//    local_best_splits_vector.push_back(local_best_split);
//
//  }
//
//  log_info("[DT_train_worker.retrieve_global_best_split]: begin to compare and get global best split");
//
//  //todo: implement compare
//
//  std::vector<double> global_best_split = local_best_splits_vector[0];
//
//  std::string send_str;
//  serialize_double_array(global_best_split, send_str);
//  worker.send_long_message_to_ps(send_str);
//}


void spdz_tree_computation(int party_num,
    int party_id,
    std::vector<int> mpc_tree_port_bases,
    std::vector<std::string> party_host_names,
    int public_value_size,
    const std::vector<int>& public_values,
    int private_value_size,
    const std::vector<double>& private_values,
    falcon::SpdzTreeCompType tree_comp_type,
    std::promise<std::vector<double>> *res) {
  std::vector<ssl_socket*> mpc_sockets(party_num);
  //  setup_sockets(party_num,
  //                party_id,
  //                std::move(mpc_player_path),
  //                std::move(party_host_names),
  //                mpc_port_base,
  //                mpc_sockets);
  // Here put the whole setup socket code together, as using a function call
  // would result in a problem when deleting the created sockets
  // setup connections from this party to each spdz party socket
  vector<int> plain_sockets(party_num);
  // ssl_ctx ctx(mpc_player_path, "C" + to_string(party_id));
  ssl_ctx ctx("C" + to_string(party_id));
  // std::cout << "correct init ctx" << std::endl;
  ssl_service io_service;
  octetStream specification;
  std::cout << "begin connect to spdz parties" << std::endl;
  std::cout << "party_num = " << party_num << std::endl;
  for (int i = 0; i < party_num; i++)
  {
    set_up_client_socket(plain_sockets[i], party_host_names[i].c_str(), mpc_tree_port_bases[i] + i);
    send(plain_sockets[i], (octet*) &party_id, sizeof(int));
    mpc_sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                    "P" + to_string(i), "C" + to_string(party_id), true);
    if (i == 0){
      // receive gfp prime
      specification.Receive(mpc_sockets[0]);
    }
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                 " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_tree_port_bases[i] + i << ".";
  }
  LOG(INFO) << "Finish setup socket connections to spdz engines.";
  std::cout << "Finish setup socket connections to spdz engines." << std::endl;
  int type = specification.get<int>();
  switch (type)
  {
    case 'p':
    {
      gfp::init_field(specification.get<bigint>());
      LOG(INFO) << "Using prime " << gfp::pr();
      break;
    }
    default:
      LOG(ERROR) << "Type " << type << " not implemented";
      exit(1);
  }
  LOG(INFO) << "Finish initializing gfp field.";
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size() << std::endl;
  google::FlushLogFiles(google::INFO);

  // send data to spdz parties
  std::cout << "party_id = " << party_id << std::endl;
  LOG(INFO) << "party_id = " << party_id;
  if (party_id == ACTIVE_PARTY_ID) {
    // the active party sends computation id for spdz computation
    std::vector<int> computation_id;
    computation_id.push_back(tree_comp_type);
    std::cout << "tree_comp_type = " << tree_comp_type << std::endl;
    LOG(INFO) << "tree_comp_type = " << tree_comp_type;
    google::FlushLogFiles(google::INFO);
    send_public_values(computation_id, mpc_sockets, party_num);
    // the active party sends tree type and class num to spdz parties
    for (int i = 0; i < public_value_size; i++) {
      std::vector<int> x;
      x.push_back(public_values[i]);
      send_public_values(x, mpc_sockets, party_num);
    }
  }
  // all the parties send private shares
  std::cout << "private value size = " << private_value_size << std::endl;
  LOG(INFO) << "private value size = " << private_value_size;
  for (int i = 0; i < private_value_size; i++) {
    vector<double> x;
    x.push_back(private_values[i]);
    send_private_inputs(x, mpc_sockets, party_num);
  }

  // receive result from spdz parties according to the computation type
  switch (tree_comp_type) {
    case falcon::PRUNING_CHECK: {
      LOG(INFO) << "SPDZ tree computation pruning check returned";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, 1);
      res->set_value(return_values);
      break;
    }
    case falcon::COMPUTE_LABEL: {
      LOG(INFO) << "SPDZ tree computation calculate leaf label returned";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, 1);
      res->set_value(return_values);
      break;
    }
    case falcon::FIND_BEST_SPLIT: {
      LOG(INFO) << "SPDZ tree computation type find best split returned";
      std::vector<double> return_values;
      std::vector<double> best_split_index = receive_result(mpc_sockets, party_num, 1);
      std::vector<double> best_left_impurity = receive_result(mpc_sockets, party_num, 1);
      std::vector<double> best_right_impurity = receive_result(mpc_sockets, party_num, 1);
      return_values.push_back(best_split_index[0]);
      return_values.push_back(best_left_impurity[0]);
      return_values.push_back(best_right_impurity[0]);
      res->set_value(return_values);
      break;
    }
    case falcon::GBDT_SQUARE_LABEL: {
      LOG(INFO) << "SPDZ tree computation type compute squared residuals";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, private_value_size);
      res->set_value(return_values);
      break;
    }
    case falcon::GBDT_SOFTMAX: {
      LOG(INFO) << "SPDZ tree computation type compute softmax results";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, private_value_size);
      res->set_value(return_values);
      break;
    }
    case falcon::RF_LABEL_MODE: {
      LOG(INFO) << "SPDZ tree computation type find random forest mode label";
      // public_values[1] denotes the number of predicted samples for computing mode
      LOG(INFO) << "public_values[1], i.e., #predicted sample size = " << public_values[1];
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[1]);
      res->set_value(return_values);
      break;
    }
    case falcon::GBDT_EXPIT: {
      LOG(INFO) << "SPDZ tree computation type compute expit(raw_predictions)";
      // public_values[0] denotes the number of samples
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[0]);
      res->set_value(return_values);
      break;
    }
    case falcon::GBDT_UPDATE_TERMINAL_REGION: {
      LOG(INFO) << "SPDZ tree computation type compute terminal region new labels";
      // public_values[1] denotes the number of leaf nodes
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[1]);
      res->set_value(return_values);
      break;
    }
    default:
      LOG(INFO) << "SPDZ tree computation type is not found.";
      exit(1);
  }

  for (int i = 0; i < party_num; i++) {
    close_client_socket(plain_sockets[i]);
  }

  // free memory and close mpc_sockets
  for (int i = 0; i < party_num; i++) {
    delete mpc_sockets[i];
    mpc_sockets[i] = nullptr;
  }
}

void train_decision_tree(
    Party *party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train, Worker* worker) {

  log_info("Run the example decision tree train");

  DecisionTreeParams params;
  // currently for testing
  params.tree_type = "classification";
  params.criterion = "gini";
  params.split_strategy = "best";
  params.class_num = 2;
  params.max_depth = 5;
  params.max_bins = 8;
  params.min_samples_split = 5;
  params.min_samples_leaf = 5;
  params.max_leaf_nodes = 16;
  params.min_impurity_decrease = 0.01;
  params.min_impurity_split = 0.001;
  params.dp_budget = 0.1;

  LOG(INFO) << "Init decision tree model builder";
  deserialize_dt_params(params, params_str);
  int weight_size = party->getter_feature_num();
  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;

  std::vector< std::vector<double> > training_data;
  std::vector< std::vector<double> > testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;

  // record full train and test data for distributed training
  std::vector< std::vector<double> > full_training_data;
  std::vector< std::vector<double> > full_testing_data;
  std::vector<double> full_training_labels;
  std::vector<double> full_testing_labels;

  // if not distributed train, then the party split the data
  // otherwise, the party/worker receive the data and phe keys from ps
  if (is_distributed_train == 0) {
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    party->split_train_test_data(split_percentage,
                                training_data,
                                testing_data,
                                training_labels,
                                testing_labels);
  }

  // here should receive the train/test data/labels from ps
  else{
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    party->split_train_test_data(split_percentage,
                                full_training_data,
                                full_testing_data,
                                full_training_labels,
                                full_testing_labels);

    std::string recv_training_data_str, recv_testing_data_str;
    std::string recv_training_labels_str, recv_testing_labels_str;
    worker->recv_long_message_from_ps(recv_training_data_str);
    worker->recv_long_message_from_ps(recv_testing_data_str);
    deserialize_double_matrix(training_data, recv_training_data_str);
    deserialize_double_matrix(testing_data, recv_testing_data_str);
    if (party->party_type == falcon::ACTIVE_PARTY) {
      worker->recv_long_message_from_ps(recv_training_labels_str);
      worker->recv_long_message_from_ps(recv_testing_labels_str);
      deserialize_double_array(training_labels, recv_training_labels_str);
      deserialize_double_array(testing_labels, recv_testing_labels_str);
    }

    // also, receive the phe keys from ps
    // and set these to the party
    std::string recv_phe_keys_str;
    log_info("begin to receive phe keys from ps ");

    worker->recv_long_message_from_ps(recv_phe_keys_str);
    log_info("received phe keys from ps: " + recv_phe_keys_str);
    party->load_phe_key_string(recv_phe_keys_str);
  }



  std::cout << "Init decision tree model" << std::endl;
  LOG(INFO) << "Init decision tree model";

  DecisionTreeBuilder decision_tree_builder(params,
      training_data,
      testing_data,
      training_labels,
      testing_labels,
      training_accuracy,
      testing_accuracy);

  LOG(INFO) << "Init decision tree model finished";
  std::cout << "Init decision tree model finished" << std::endl;
  google::FlushLogFiles(google::INFO);

  if (is_distributed_train == 0){
    decision_tree_builder.train(*party);
    decision_tree_builder.eval(*party, falcon::TRAIN);
    decision_tree_builder.eval(*party, falcon::TEST);
    // save model and report
    // save_dt_model(decision_tree_builder.tree, model_save_file);
    std::string pb_dt_model_string;
    serialize_tree_model(decision_tree_builder.tree, pb_dt_model_string);
    save_pb_model_string(pb_dt_model_string, model_save_file);
    save_training_report(decision_tree_builder.getter_training_accuracy(),
                         decision_tree_builder.getter_testing_accuracy(),
                         model_report_file);

    LOG(INFO) << "Trained model and report saved";
    std::cout << "Trained model and report saved" << std::endl;
    google::FlushLogFiles(google::INFO);
  } else {

    // on evaluation stage, each worker should have all features.
    DecisionTreeBuilder decision_tree_builder_eval(params,
                                                   full_training_data,
                                                   full_testing_data,
                                                   full_training_labels,
                                                   full_testing_labels,
                                                   training_accuracy,
                                                   testing_accuracy);

    decision_tree_builder.distributed_train(*party, *worker);
    // update the tree instance in decision_tree_builder_eval.
    decision_tree_builder_eval.tree = decision_tree_builder.tree;
    decision_tree_builder_eval.distributed_eval(*party, *worker, falcon::TRAIN);
    decision_tree_builder_eval.distributed_eval(*party, *worker, falcon::TEST);
    // in is_distributed_train, parameter server will save the model.

  }
}
