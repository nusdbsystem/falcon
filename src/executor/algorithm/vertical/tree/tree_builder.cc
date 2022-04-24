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
#include <falcon/utils/alg/tree_util.h>
#include <falcon/utils/alg/debug_util.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/party/info_exchange.h>

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
#include <queue>

DecisionTreeBuilder::DecisionTreeBuilder() {}

DecisionTreeBuilder::~DecisionTreeBuilder() {
  log_info("[DecisionTreeBuilder]: destructor called");
  delete [] feature_helpers;
}

DecisionTreeBuilder::DecisionTreeBuilder(const DecisionTreeBuilder &builder) : ModelBuilder(
    builder.training_data,builder.testing_data,builder.training_labels,
    builder.testing_labels, builder.training_accuracy, builder.testing_accuracy) {
  log_info("[DecisionTreeBuilder]: copy constructor started.");
  train_data_size = builder.train_data_size;
  test_data_size = builder.test_data_size;
  tree_type = builder.tree_type;
  criterion = builder.criterion;
  split_strategy = builder.split_strategy;
  class_num = builder.class_num;
  max_depth = builder.max_depth;
  max_bins = builder.max_bins;
  min_samples_split = builder.min_samples_split;
  min_samples_leaf = builder.min_samples_leaf;
  max_leaf_nodes = builder.max_leaf_nodes;
  min_impurity_split = builder.min_impurity_split;
  min_impurity_decrease = builder.min_impurity_decrease;
  dp_budget = builder.dp_budget;
  local_feature_num = builder.local_feature_num;
  feature_types = builder.feature_types;
  indicator_class_vecs = builder.indicator_class_vecs;
  variance_stat_vecs = builder.variance_stat_vecs;
  tree = builder.tree;
  feature_helpers = new FeatureHelper[local_feature_num];
  for (int i = 0; i < local_feature_num; i++) {
    feature_helpers[i] = builder.feature_helpers[i];
  }
  log_info("[DecisionTreeBuilder]: copy constructor finished.");
}

DecisionTreeBuilder::DecisionTreeBuilder(DecisionTreeParams params,
    std::vector<std::vector<double> > m_training_data,
    std::vector<std::vector<double> > m_testing_data,
    std::vector<double> m_training_labels, std::vector<double> m_testing_labels,
    double m_training_accuracy, double m_testing_accuracy) : ModelBuilder(
        std::move(m_training_data),         std::move(m_testing_data),
          std::move(m_training_labels),std::move(m_testing_labels),
          m_training_accuracy, m_testing_accuracy) {
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
  // init tree object， call assign copy constructor
  tree = TreeModel(tree_type, class_num, max_depth);
}

void DecisionTreeBuilder::precompute_label_helper(falcon::PartyType party_type) {
  if (party_type == falcon::PASSIVE_PARTY) {
    return;
  }
  // only the active party has label information
  // pre-compute indicator vectors or variance vectors for labels
  // here already assume that it is computed on active party
  log_info("[DecisionTreeBuilder.precompute_label_helper] Pre-compute label assist info for training");
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
      log_info("[DecisionTreeBuilder.precompute_label_helper] Class "
        + std::to_string(i) + "'s sample num = " + std::to_string(sample_num_classes[i]));
    }
    delete [] sample_num_classes;
  } else {
    // regression, compute variance necessary stats
    std::vector<double> label_square_vec;
    for (int i = 0; i < training_labels.size(); i++) {
      label_square_vec.push_back(training_labels[i] * training_labels[i]);
    }
    // the first vector is the actual label vector
    variance_stat_vecs.push_back(training_labels);
    // the second vector is the squared label vector
    variance_stat_vecs.push_back(label_square_vec);
  }
}

void DecisionTreeBuilder::precompute_feature_helpers() {
  log_info("[DecisionTreeBuilder.precompute_feature_helpers] Pre-compute feature helpers for training");
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
#ifdef DEBUG
    for (int x = 0; x < feature_helpers[i].split_values.size(); x++) {
      log_info("Feature " + std::to_string(i) + "'s split values[" + std::to_string(x) + "] = " + std::to_string(feature_helpers[i].split_values[x]));
    }
#endif
  }
}

void DecisionTreeBuilder::calc_root_impurity(const Party& party) {
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  auto* root_impu = new EncodedNumber[1];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    double impu = root_impurity(training_labels, tree_type, class_num);
    log_info("[DecisionTreeBuilder.train] root_impu = " + std::to_string(impu));
    root_impu[0].set_double(phe_pub_key->n[0], impu, PHE_FIXED_POINT_PRECISION);
  }
  broadcast_encoded_number_array(party, root_impu, 1, ACTIVE_PARTY_ID);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, root_impu[0]);

  delete [] root_impu;
  djcs_t_free_public_key(phe_pub_key);
}

void DecisionTreeBuilder::train(Party party) {
  // To avoid each tree node disclose which samples are on available,
  // we use an encrypted mask vector to protect the information
  // while ensure that the training can still be processed.
  // Note that for the root node, all the samples are available and every
  // party can initialize an encrypted mask vector with [1], but for the
  // labels, only the active party has the info, then it initializes
  // and sends the encrypted label info to other parties.
  log_info("[DecisionTreeBuilder.train] ************* Training Start *************");
  const clock_t training_start_time = clock();

  // pre-compute label helper and feature helper
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();

  int sample_num = training_data.size();
  std::vector<int> available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    available_feature_ids.push_back(i);
  }
  EncodedNumber * sample_mask_iv = new EncodedNumber[sample_num];
  // label_size = class_num * sample_num;
  EncodedNumber * encrypted_labels = new EncodedNumber[class_num * sample_num];
  initialize_sample_mask_iv(party, sample_mask_iv, sample_num);
  initialize_encrypted_labels(party, encrypted_labels, sample_num);

#ifdef DEBUG
  debug_cipher_array<long>(party, sample_mask_iv, 10, ACTIVE_PARTY_ID, true, 10);
  debug_cipher_array<double>(party, encrypted_labels, 10, ACTIVE_PARTY_ID, true, 10);
#endif

  // init the root node info
  tree.nodes[0].depth = 0;
  tree.nodes[0].node_sample_num = train_data_size;
  calc_root_impurity(party);

  // required by spdz connector and mpc computation
  bigint::init_thread();
  // recursively build the tree
  build_node(party, 0, available_feature_ids, sample_mask_iv, encrypted_labels);
  log_info("[DecisionTreeBuilder.train] tree capacity = " + std::to_string(tree.capacity));

  const clock_t training_finish_time = clock();
  double training_consumed_time = double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("[DecisionTreeBuilder.train] Training time = " + std::to_string(training_consumed_time));
  log_info("[DecisionTreeBuilder.train] ************* Training Finished *************");

  delete [] sample_mask_iv;
  delete [] encrypted_labels;
}

void DecisionTreeBuilder::train(Party party, EncodedNumber *encrypted_labels) {
  // To avoid each tree node disclose which samples are on available,
  // we use an encrypted mask vector to protect the information
  // while ensure that the training can still be processed.
  // Note that for the root node, all the samples are available and every
  // party can initialize an encrypted mask vector with [1]
  // for the labels, the format should be encrypted (with size class_num * sample_num),
  // and is known to other parties as well (should be sent before calling this train method)
  log_info("[DecisionTreeBuilder.train-with-enc-labels] ************* Training Start *************");
  const clock_t training_start_time = clock();
  // pre-compute label helper and feature helper
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();
  log_info("[DecisionTreeBuilder.train-with-enc-labels]: finished init label and features");
  int sample_num = training_data.size();
  int label_size = class_num * sample_num;
  std::vector<int> available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    available_feature_ids.push_back(i);
  }
  // init sample mask iv
  auto *sample_mask_iv = new EncodedNumber[sample_num];
  initialize_sample_mask_iv(party, sample_mask_iv, sample_num);
  // init the root node info
  tree.nodes[0].depth = 0;
  tree.nodes[0].node_sample_num = train_data_size;
  calc_root_impurity(party);

  // required by spdz connector and mpc computation
  bigint::init_thread();
  // recursively build the tree
  build_node(party, 0, available_feature_ids, sample_mask_iv, encrypted_labels);

  log_info("[DecisionTreeBuilder.train-with-enc-labels] tree capacity = " + std::to_string(tree.capacity));
  const clock_t training_finish_time = clock();
  double training_consumed_time = double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("[DecisionTreeBuilder.train-with-enc-labels] Training time = " + std::to_string(training_consumed_time));
  log_info("[DecisionTreeBuilder.train-with-enc-labels] ************* Training Finished *************");

  delete [] sample_mask_iv;
}

void DecisionTreeBuilder::initialize_sample_mask_iv(Party party, EncodedNumber *sample_mask_iv, int sample_num) {
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
  djcs_t_free_public_key(phe_pub_key);
}

void DecisionTreeBuilder::initialize_encrypted_labels(Party party, EncodedNumber *encrypted_labels, int sample_num) {
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

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
  }
  // active party broadcast the encrypted labels
  broadcast_encoded_number_array(party, encrypted_labels, class_num * sample_num, ACTIVE_PARTY_ID);
  log_info("[DecisionTreeBuilder.train] Finish broadcasting the encrypted label info");

  djcs_t_free_public_key(phe_pub_key);
}

void DecisionTreeBuilder::build_node(Party &party, int node_index,
    std::vector<int> available_feature_ids, EncodedNumber *sample_mask_iv,
    EncodedNumber *encrypted_labels, bool use_sample_weights, EncodedNumber *encrypted_weights) {
  // 1. check if some pruning conditions are satisfied
  //      1.1 feature set is empty;
  //      1.2 the number of samples are less than a pre-defined threshold
  //      1.3 if classification, labels are same; if regression,
  //         label variance is less than a threshold
  // 2. if satisfied, return a leaf node with majority class or mean label;
  //      else, continue to step 3
  // 3. super client computes some encrypted label information
  //     and broadcast to the other clients
  // 4. every client locally compute necessary encrypted statistics,
  //     i.e., #samples per class for classification or variance info
  // 5. the clients convert the encrypted statistics to shares
  //     and send to SPDZ parties
  // 6. wait for SPDZ parties return (i_*, j_*, s_*),
  //     where i_* is client id, j_* is feature id, and s_* is split id
  // 7. client who owns the best split feature do the splits
  //     and update mask vector, and broadcast to the other clients
  // 8. every client updates mask vector and local tree model
  // 9. recursively build the next two tree nodes
  log_info("[DecisionTreeBuilder.build_node] ****** Build tree node " + std::to_string(node_index)
               + " (depth = " + std::to_string(tree.nodes[node_index].depth) + ")******");
  const clock_t node_start_time = clock();
  if (node_index >= tree.capacity) {
    log_info("[DecisionTreeBuilder.build_node] Node index exceeds the maximum tree depth");
    exit(EXIT_FAILURE);
  }

#ifdef DEBUG
  // aggregate sample_iv and set node number for debug
  auto* encrypted_node_num = new EncodedNumber[1];
  auto* decrypted_node_num = new EncodedNumber[1];
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key_tmp = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key_tmp);
  if (party.party_type == falcon::ACTIVE_PARTY) {
    encrypted_node_num[0].set_integer(phe_pub_key_tmp->n[0], 0);
    djcs_t_aux_encrypt(phe_pub_key_tmp, party.phe_random, encrypted_node_num[0], encrypted_node_num[0]);
    int sample_num = (int) training_data.size();
    for (int i = 0; i < sample_num; i++) {
      djcs_t_aux_ee_add(phe_pub_key_tmp, encrypted_node_num[0], encrypted_node_num[0], sample_mask_iv[i]);
    }
  }
  broadcast_encoded_number_array(party, encrypted_node_num, 1, ACTIVE_PARTY_ID);
  // decrypt
  collaborative_decrypt(party, encrypted_node_num, decrypted_node_num, 1, ACTIVE_PARTY_ID);
  long node_sample_num;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    decrypted_node_num[0].decode(node_sample_num);
    tree.nodes[node_index].node_sample_num = node_sample_num;
    log_info("[DecisionTreeBuilder: build_node] node_index = "
      + std::to_string(node_index) + ", node_sample_num = " + std::to_string(node_sample_num));
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, std::to_string(node_sample_num));
      }
    }
  } else {
    std::string recv_node_sample_num;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_node_sample_num);
    node_sample_num = std::stoi(recv_node_sample_num);
    tree.nodes[node_index].node_sample_num = node_sample_num;
  }
  delete [] encrypted_node_num;
  delete [] decrypted_node_num;
  djcs_t_free_public_key(phe_pub_key_tmp);
#endif

  // step 1: check pruning condition via spdz computation
  bool is_satisfied = check_pruning_conditions(party, node_index, sample_mask_iv);
  // step 2: if satisfied, compute label via spdz computation
  if (is_satisfied) {
    compute_leaf_statistics(party, node_index, sample_mask_iv, encrypted_labels,
                            use_sample_weights, encrypted_weights);
    return;
  }

  // step 3: invoke to find the best split using phe and spdz
  std::vector<int> party_split_nums;
  std::vector<double> res = find_best_split(party, node_index,
                                            available_feature_ids, sample_mask_iv,
                                            encrypted_labels, party_split_nums,
                                            use_sample_weights, encrypted_weights);
  int best_split_index = (int) res[0];
  double left_impurity = res[1];
  double right_impurity = res[2];
  log_info("[DecisionTreeBuilder.build_node] best_split_index = " + std::to_string(best_split_index));
  log_info("[DecisionTreeBuilder.build_node] left_impurity = " + std::to_string(left_impurity));
  log_info("[DecisionTreeBuilder.build_node] right_impurity = " + std::to_string(right_impurity));

  EncodedNumber encrypted_left_impurity, encrypted_right_impurity;
  encrypt_left_right_impu(party, left_impurity, right_impurity, encrypted_left_impurity, encrypted_right_impurity);

  // step 7: update tree nodes, including sample iv for iterative node computation
  std::vector<int> available_feature_ids_new;
  int sample_num = (int) training_data.size();
  EncodedNumber *sample_mask_iv_left = new EncodedNumber[sample_num];
  EncodedNumber *sample_mask_iv_right = new EncodedNumber[sample_num];
  EncodedNumber *encrypted_labels_left = new EncodedNumber[sample_num * class_num];
  EncodedNumber *encrypted_labels_right = new EncodedNumber[sample_num * class_num];

  int left_child_index = 2 * node_index + 1;
  int right_child_index = 2 * node_index + 2;

  int i_star, index_star;
  best_split_party_idx(best_split_index, party_split_nums, i_star, index_star);
  log_info("[DecisionTreeBuilder.build_node] Best split party: i_star = " + std::to_string(i_star));

  // if the party has the best split:
  // compute locally and broadcast, find the j_* feature and s_* split
  if (i_star == party.party_id) {
    // define j_star is the best feature id, s_star is best split id of j_star
    int j_star, s_star;
    best_feature_split_idx(index_star, available_feature_ids, j_star, s_star);
    log_info("[DecisionTreeBuilder.build_node] Best feature id: j_star = " + std::to_string(j_star));
    log_info("[DecisionTreeBuilder.build_node] Best split id: s_star = " + std::to_string(s_star));
    // update the current node info
    update_node_info(node_index, falcon::INTERNAL, 1, i_star, j_star, s_star,
                     feature_helpers[j_star].split_values[s_star],
                     left_child_index, right_child_index,
                     encrypted_left_impurity, encrypted_right_impurity);
    // update available feature ids
    for (int i = 0; i < available_feature_ids.size(); i++) {
      int feature_id = available_feature_ids[i];
      if (j_star != feature_id) {
        available_feature_ids_new.push_back(feature_id);
      }
    }

    // compute mask iv and labels for child nodes
    std::string update_str_sample_iv, update_str_encrypted_labels_left, update_str_encrypted_labels_right;
    update_mask_iv_and_labels(party, j_star, s_star, sample_num,
                              sample_mask_iv, encrypted_labels,
                              encrypted_left_impurity,
                              encrypted_right_impurity,
                              sample_mask_iv_left, sample_mask_iv_right,
                              encrypted_labels_left, encrypted_labels_right,
                              update_str_sample_iv,
                              update_str_encrypted_labels_left,
                              update_str_encrypted_labels_right);

    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, update_str_sample_iv);
        party.send_long_message(i, update_str_encrypted_labels_left);
        party.send_long_message(i, update_str_encrypted_labels_right);
      }
    }
  }

  // step 8: every other party update the local tree model
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

    // update the current node info (set split threshold = -1 for simplicity)
    update_node_info(node_index, falcon::INTERNAL, 0, recv_best_party_id,
                     recv_best_feature_id, recv_best_split_id,-1,
                     left_child_index, right_child_index,
                     encrypted_left_impurity, encrypted_right_impurity);
    available_feature_ids_new = available_feature_ids;
  }

  tree.internal_node_num += 1;
  tree.total_node_num += 1;

  const clock_t node_finish_time = clock();
  double node_consumed_time = double (node_finish_time - node_start_time) / CLOCKS_PER_SEC;
  log_info("[DecisionTreeBuilder.build_node] Node build time = " + std::to_string(node_consumed_time));

  // step 9: recursively build the next child tree nodes
  build_node(party, left_child_index,available_feature_ids_new,
      sample_mask_iv_left, encrypted_labels_left, use_sample_weights, encrypted_weights);
  build_node(party, right_child_index,available_feature_ids_new,
      sample_mask_iv_right, encrypted_labels_right, use_sample_weights, encrypted_weights);

  delete [] sample_mask_iv_left;
  delete [] sample_mask_iv_right;
  delete [] encrypted_labels_left;
  delete [] encrypted_labels_right;
  log_info("[DecisionTreeBuilder.build_node] End build tree node: " + std::to_string(node_index));
}

bool DecisionTreeBuilder::check_pruning_conditions(Party &party, int node_index, EncodedNumber *sample_mask_iv) {
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
  std::vector<double> private_values, condition_shares1, condition_shares2;

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
    ciphers_to_secret_shares(party, encrypted_sample_count, condition_shares1, 1,
        ACTIVE_PARTY_ID, 0);
    ciphers_to_secret_shares(party, encrypted_impurity, condition_shares2, 1,
        ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  } else {
    ciphers_to_secret_shares(party, encrypted_sample_count, condition_shares1, 1,
        ACTIVE_PARTY_ID, 0);
    ciphers_to_secret_shares(party, encrypted_impurity, condition_shares2, 1,
        ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  }
  private_values.push_back(condition_shares1[0]);
  private_values.push_back(condition_shares2[0]);

#ifdef DEBUG
  // print executor_mpc_ports for debug
  for (int i = 0; i < party.executor_mpc_ports.size(); i++) {
    log_info("[DecisionTreeBuilder.check_pruning_conditions]: party.executor_mpc_ports["
      + std::to_string(i) + "] = " + std::to_string(party.executor_mpc_ports[i]));
  }
#endif

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

void DecisionTreeBuilder::compute_leaf_statistics(Party &party, int node_index,
    EncodedNumber *sample_mask_iv, EncodedNumber *encrypted_labels,
    bool use_sample_weights, EncodedNumber *encrypted_weights) {
  log_info("[DecisionTreeBuilder.compute_leaf_statistics] begin to compute leaf values");
  int sample_num = (int) training_data.size();
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // set spdz computation values
  falcon::SpdzTreeCompType comp_type = falcon::COMPUTE_LABEL;
  std::vector<int> public_values;
  std::vector<double> private_values, shares1, shares2;
  public_values.push_back(tree_type);
  public_values.push_back(class_num);
  // compute leaf values logic
  if (party.party_type == falcon::ACTIVE_PARTY) {
    if (tree_type == falcon::CLASSIFICATION) {
      // compute how many samples in each class, eg: [100, 90], 100 samples for c1, and 90 for c2
      auto *class_sample_nums = new EncodedNumber[class_num];
      for (int c = 0; c < class_num; c++) {
        class_sample_nums[c] = encrypted_labels[c * sample_num + 0];
        for (int j = 1; j < sample_num; j++) {
          djcs_t_aux_ee_add(phe_pub_key, class_sample_nums[c],
              class_sample_nums[c], encrypted_labels[c * sample_num + j]);
        }
      }
      ciphers_to_secret_shares(party, class_sample_nums, private_values,
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
      ciphers_to_secret_shares(party, label_info, shares1, 1,
          ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      ciphers_to_secret_shares(party, encrypted_sample_num_aux, shares2, 1,
          ACTIVE_PARTY_ID, 0);
      private_values.push_back(shares1[0]);
      private_values.push_back(shares2[0]);
      delete [] label_info;
      delete [] encrypted_sample_num_aux;
    }
  } else { // PASSIVE PARTY
    if (tree_type == falcon::CLASSIFICATION) {
      EncodedNumber *class_sample_nums = new EncodedNumber[class_num];
      ciphers_to_secret_shares(party, class_sample_nums, private_values,
          class_num, ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      delete [] class_sample_nums;
    } else { // REGRESSION
      EncodedNumber *label_info = new EncodedNumber[1];
      EncodedNumber *encrypted_sample_num_aux = new EncodedNumber[1];
      ciphers_to_secret_shares(party, label_info, shares1, 1,
          ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
      ciphers_to_secret_shares(party, encrypted_sample_num_aux, shares2, 1,
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

  log_info("[DecisionTreeBuilder.compute_leaf_statistics] Node "
    + std::to_string(node_index) + " label = " + std::to_string(res[0]));
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

std::vector<double> DecisionTreeBuilder::find_best_split(const Party &party,
                                          int node_index,
                                          std::vector<int> available_feature_ids,
                                          EncodedNumber *sample_mask_iv,
                                          EncodedNumber *encrypted_labels,
                                          std::vector<int> &party_split_nums,
                                          bool use_sample_weights,
                                          EncodedNumber *encrypted_weights) {
  // step 3: active party computes some encrypted label
  // information and broadcast to the other clients
  // step 4: every party locally computes necessary encrypted statistics,
  // i.e., #samples per class for classification or variance info
  //     specifically, for each feature, for each split, for each class,
  //     compute necessary encrypted statistics,
  //     store the encrypted statistics,
  //     convert to secret shares,
  //     and send to spdz parties for mpc computation
  //     finally, receive the (i_*, j_*, s_*) and encrypted impurity
  // for the left child and right child, update tree_nodes
  // step 4.1: party inits a two-dimensional encrypted vector
  //     with size \sum_{i=0}^{node[available_feature_ids.size()]} features[i].num_splits
  // step 4.2: party computes encrypted statistics
  // step 4.3: party sends encrypted statistics
  // step 4.4: active party computes a large encrypted
  //     statistics matrix, and broadcasts total splits num
  // step 4.5: party converts the encrypted statistics matrix into secret shares
  // step 4.6: parties jointly send shares to spdz parties

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
  global_encrypted_statistics = new EncodedNumber*[MAX_GLOBAL_SPLIT_NUM];
  for (int i = 0; i < MAX_GLOBAL_SPLIT_NUM; i++) {
    global_encrypted_statistics[i] = new EncodedNumber[2 * class_num];
  }
  global_left_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];
  global_right_branch_sample_nums = new EncodedNumber[MAX_GLOBAL_SPLIT_NUM];
  // std::vector<int> party_split_nums;

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
    compute_encrypted_statistics(party, node_index,available_feature_ids,
                                 sample_mask_iv, encrypted_statistics, encrypted_labels,
                                 encrypted_left_branch_sample_nums,
                                 encrypted_right_branch_sample_nums,
                                 use_sample_weights, encrypted_weights);
  }
  log_info("[DecisionTreeBuilder.find_best_split] Finish computing local statistics");

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
                                           recv_encrypted_statistics,recv_encrypted_statistics_str);

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
    log_info("[DecisionTreeBuilder.find_best_split] The global_split_num = " + std::to_string(global_split_num));
    // send the total number of splits for the other clients to generate secret shares
    log_info("[DecisionTreeBuilder.find_best_split] Send party split nums to other parties");
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
    log_info("[DecisionTreeBuilder.find_best_split] Local split num = " + std::to_string(local_splits_num));
    std::string local_split_num_str = std::to_string(local_splits_num);
    party.send_long_message(ACTIVE_PARTY_ID, local_split_num_str);
    // then send the encrypted statistics
    std::string encrypted_statistics_str;
    serialize_encrypted_statistics(party.party_id, node_index, local_splits_num, class_num,
                                   encrypted_left_branch_sample_nums, encrypted_right_branch_sample_nums,
                                   encrypted_statistics, encrypted_statistics_str);
    party.send_long_message(ACTIVE_PARTY_ID, encrypted_statistics_str);

    // recv the split info from active party
    std::string recv_split_info_str;
    party.recv_long_message(0, recv_split_info_str);
    deserialize_split_info(global_split_num, party_split_nums, recv_split_info_str);
    log_info("[DecisionTreeBuilder.find_best_split] The global_split_num = " + std::to_string(global_split_num));
  }

#ifdef DEBUG
  log_info("[DecisionTreeBuilder.build_node] debug global statistics print");
  debug_cipher_matrix<double>(party, global_encrypted_statistics, 10,
                              2 * class_num, ACTIVE_PARTY_ID, true, 10, 2 * class_num);
#endif

  // step 5: encrypted statistics computed finished, convert to secret shares
  std::vector< std::vector<double> > stats_shares;
  std::vector<double> left_sample_nums_shares, right_sample_nums_shares;
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
  log_info("[DecisionTreeBuilder.find_best_split]: branch_sample_nums_prec = " + std::to_string(branch_sample_nums_prec));
  ciphers_to_secret_shares(party, global_left_branch_sample_nums,
                           left_sample_nums_shares, global_split_num,
                           ACTIVE_PARTY_ID, branch_sample_nums_prec);
  ciphers_to_secret_shares(party, global_right_branch_sample_nums,
                           right_sample_nums_shares, global_split_num,
                           ACTIVE_PARTY_ID, branch_sample_nums_prec);
  for (int i = 0; i < global_split_num; i++) {
    std::vector<double> tmp;
    ciphers_to_secret_shares(party, global_encrypted_statistics[i],tmp, 2 * class_num,
                             ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
    stats_shares.push_back(tmp);
  }
  log_info("[DecisionTreeBuilder.find_best_split]: convert secret shares finished ");

  // step 6: secret shares conversion finished, talk to spdz for computation
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
  // the result values are as follows (assume public in this version):
  // best_split_index (global), best_left_impurity, and best_right_impurity
  std::vector<double> res = future_values.get();
  spdz_pruning_check_thread.join();
  log_info("[DecisionTreeBuilder.find_best_split]: communicate with spdz program finished");

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

  return res;
}

void DecisionTreeBuilder::encrypt_left_right_impu(const Party& party, double left_impurity,
                                                  double right_impurity,
                                                  EncodedNumber &encrypted_left_impurity,
                                                  EncodedNumber &encrypted_right_impurity) {
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  encrypted_left_impurity.set_double(phe_pub_key->n[0],
                                     left_impurity, PHE_FIXED_POINT_PRECISION);
  encrypted_right_impurity.set_double(phe_pub_key->n[0],
                                      right_impurity, PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                     encrypted_left_impurity, encrypted_left_impurity);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                     encrypted_right_impurity, encrypted_right_impurity);
  djcs_t_free_public_key(phe_pub_key);
}


void DecisionTreeBuilder::compute_encrypted_statistics(const Party &party,
                                                       int node_index, std::vector<int> available_feature_ids,
                                                       EncodedNumber *sample_mask_iv, EncodedNumber **encrypted_statistics,
                                                       EncodedNumber *encrypted_labels,
                                                       EncodedNumber *encrypted_left_sample_nums,
                                                       EncodedNumber *encrypted_right_sample_nums,
                                                       bool use_sample_weights, EncodedNumber *encrypted_weights) {
#ifdef DEBUG
  if (use_sample_weights) {
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] Compute encrypted statistics for node " + std::to_string(node_index));
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] sample_mask_iv precision =  " + std::to_string(std::abs(sample_mask_iv[0].getter_exponent())));
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] encrypted_labels precision =  " + std::to_string(std::abs(encrypted_labels[0].getter_exponent())));
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] encrypted_weights precision =  " + std::to_string(std::abs(encrypted_weights[0].getter_exponent())));
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] use_sample_weights =  " + std::to_string(use_sample_weights));
  }
#endif
  const clock_t start_time = clock();

  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  int split_index = 0;
  int available_feature_num = (int) available_feature_ids.size();
  int sample_num = (int) training_data.size();

  // TODO: add another encrypted vector at the beginning, instead of cipher multiplication on each node
  auto* weighted_sample_mask_iv = new EncodedNumber[sample_num];
  if (use_sample_weights) {
    // if use encrypted weights, need to execute an element-wise cipher multiplication
    ciphers_multi(party, weighted_sample_mask_iv, sample_mask_iv, encrypted_weights,
                  sample_num, ACTIVE_PARTY_ID);
  } else {
    for (int i = 0; i < sample_num; i++) {
      weighted_sample_mask_iv[i] = sample_mask_iv[i];
    }
  }

  // TODO: this function is difficult to read, should add more explanations
  // splits of features are flatted, classes_num * 2 are for left and right
  // in this method, the feature values are sorted,
  // use the sorted indexes to re-organize the encrypted mask vector,
  // and compute num_splits + 1 bucket statistics by one traverse
  // then the encrypted statistics for the num_splits
  // can be aggregated by num_splits homomorphic additions
  for (int j = 0; j < available_feature_num; j++) {
    int feature_id = available_feature_ids[j];
    int split_num = feature_helpers[feature_id].num_splits;
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] split_num = " + std::to_string(split_num));
    std::vector<int> sorted_indices = feature_helpers[feature_id].sorted_indexes;
    auto *sorted_sample_iv = new EncodedNumber[sample_num];
    // copy the sample_iv
    for (int idx = 0; idx < sample_num; idx++) {
      sorted_sample_iv[idx] = weighted_sample_mask_iv[sorted_indices[idx]];
    }
    // compute the cipher precision
    int sorted_sample_iv_prec = std::abs(sorted_sample_iv[0].getter_exponent());
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] sorted_sample_iv_prec = " + std::to_string(sorted_sample_iv_prec));

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
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,left_stats[k][c], left_stats[k][c]);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,right_stats[k][c], right_stats[k][c]);
      }
    }
    for (int c = 0; c < class_num; c++) {
      sums_stats[c].set_double(phe_pub_key->n[0], 0);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,sums_stats[c], sums_stats[c]);
    }
    // compute sample iv statistics by one traverse
    int split_iterator = 0;
    for (int sample_idx = 0; sample_idx < sample_num; sample_idx++) {
      int sorted_idx = sorted_indices[sample_idx];
      double sorted_feature_value = feature_helpers[feature_id].origin_feature_values[sorted_idx];
      djcs_t_aux_ee_add(phe_pub_key, total_sum,total_sum, sorted_sample_iv[sample_idx]);
      for (int c = 0; c < class_num; c++) {
        djcs_t_aux_ee_add(phe_pub_key, sums_stats[c],
                          sums_stats[c],encrypted_labels[c * sample_num + sorted_idx]);
      }
      if (split_iterator == split_num) {
        continue;
      }
      // find the first split value that larger than the current feature value, usually only step by 1
      // if ((sorted_feature_value - feature_helpers[feature_id].split_values[split_iterator]) > ROUNDED_PRECISION) {
      while ((split_iterator < split_num) && (sorted_feature_value > feature_helpers[feature_id].split_values[split_iterator])) {
        split_iterator += 1;
      }
      if (split_iterator == split_num) {
        continue;
      }
      djcs_t_aux_ee_add(phe_pub_key, left_sums[split_iterator],
                        left_sums[split_iterator], sorted_sample_iv[sample_idx]);
      for (int c = 0; c < class_num; c++) {
        djcs_t_aux_ee_add(phe_pub_key,left_stats[split_iterator][c],
                          left_stats[split_iterator][c],encrypted_labels[c * sample_num + sorted_idx]);
      }
    }
    log_info("[DecisionTreeBuilder.compute_encrypted_statistics] finish computing statistics left");

    // write the left sums to encrypted_left_sample_nums and update the right sums
    EncodedNumber left_num_help, right_num_help, plain_constant_help;
    left_num_help.set_double(phe_pub_key->n[0], 0.0, sorted_sample_iv_prec);
    right_num_help.set_double(phe_pub_key->n[0], 0.0, sorted_sample_iv_prec);
    plain_constant_help.set_integer(phe_pub_key->n[0], -1);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,left_num_help, left_num_help);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,right_num_help, right_num_help);

    auto *left_stat_help = new EncodedNumber[class_num];
    auto *right_stat_help = new EncodedNumber[class_num];
    for (int c = 0; c < class_num; c++) {
      left_stat_help[c].set_double(phe_pub_key->n[0], 0);
      right_stat_help[c].set_double(phe_pub_key->n[0], 0);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,left_stat_help[c], left_stat_help[c]);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,right_stat_help[c], right_stat_help[c]);
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
  log_info("[DecisionTreeBuilder.compute_encrypted_statistics] Node encrypted statistics computation time = " + std::to_string(consumed_time));
}

void DecisionTreeBuilder::best_split_party_idx(int best_split_index,
                                                    std::vector<int> party_split_nums,
                                                    int &i_star,
                                                    int &index_star) {
  // convert the index_in_global_split_num to (i_*, index_*)
  i_star = -1, index_star = -1;
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
}

void DecisionTreeBuilder::best_feature_split_idx(int index_star,
                                                 std::vector<int> available_feature_ids,
                                                 int &j_star, int &s_star) {
  j_star = -1, s_star = -1;
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
}

void DecisionTreeBuilder::update_node_info(int node_index,
                                           falcon::TreeNodeType node_type,
                                           int is_self_feature,
                                           int best_party_id,
                                           int best_feature_id,
                                           int best_split_id,
                                           double split_threshold,
                                           int left_child_index,
                                           int right_child_index,
                                           EncodedNumber encrypted_left_impurity,
                                           EncodedNumber encrypted_right_impurity) {
  // now we have (i_*, j_*, s_*), retrieve s_*-th split ivs and update sample_ivs of two branches
  // update current node index for prediction
  tree.nodes[node_index].node_type = node_type;
  tree.nodes[node_index].is_self_feature = is_self_feature;
  tree.nodes[node_index].best_party_id = best_party_id;
  tree.nodes[node_index].best_feature_id = best_feature_id;
  tree.nodes[node_index].best_split_id = best_split_id;
  tree.nodes[node_index].split_threshold = split_threshold;
  tree.nodes[node_index].left_child = left_child_index;
  tree.nodes[node_index].right_child = right_child_index;
  tree.nodes[left_child_index].depth = tree.nodes[node_index].depth + 1;
  tree.nodes[left_child_index].impurity = encrypted_left_impurity;
  tree.nodes[right_child_index].depth = tree.nodes[node_index].depth + 1;
  tree.nodes[right_child_index].impurity = encrypted_right_impurity;

  log_info("[DecisionTreeBuilder.update_node_info]: finish updating tree, "
           "\nnode_index:" + to_string(node_index) +
      "\nis_self_feature:" + to_string(tree.nodes[node_index].is_self_feature) +
      "\nbest_party_id:" + to_string(tree.nodes[node_index].best_party_id) +
      "\nbest_feature_id:" + to_string(tree.nodes[node_index].best_feature_id) +
      "\nbest_split_id:" + to_string(tree.nodes[node_index].best_split_id) +
      "\nsplit_threshold:" + to_string(tree.nodes[node_index].split_threshold) +
      "\nleft_child_index:" + to_string(left_child_index) +
      "\nright_child_index:" + to_string(right_child_index));
}

void DecisionTreeBuilder::update_mask_iv_and_labels(const Party& party,
                                                    int j_star,
                                                    int s_star,
                                                    int sample_num,
                                                    EncodedNumber *sample_mask_iv,
                                                    EncodedNumber *encrypted_labels,
                                                    EncodedNumber encrypted_left_impurity,
                                                    EncodedNumber encrypted_right_impurity,
                                                    EncodedNumber *sample_mask_iv_left,
                                                    EncodedNumber *sample_mask_iv_right,
                                                    EncodedNumber *encrypted_labels_left,
                                                    EncodedNumber *encrypted_labels_right,
                                                    std::string &update_str_sample_iv,
                                                    std::string &update_str_encrypted_labels_left,
                                                    std::string &update_str_encrypted_labels_right) {
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

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
  serialize_update_info(party.party_id, party.party_id, j_star, s_star,
                        encrypted_left_impurity, encrypted_right_impurity,
                        sample_mask_iv_left, sample_mask_iv_right,
                        sample_num, update_str_sample_iv);

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
  serialize_encoded_number_array(encrypted_labels_left, class_num * sample_num,
                                 update_str_encrypted_labels_left);
  serialize_encoded_number_array(encrypted_labels_right, class_num * sample_num,
                                 update_str_encrypted_labels_right);

  djcs_t_free_public_key(phe_pub_key);
}

void DecisionTreeBuilder::lime_train(Party party, bool use_encrypted_labels,
                                     EncodedNumber *encrypted_true_labels, bool use_sample_weights,
                                     EncodedNumber *encrypted_weights) {
  // To avoid each tree node disclose which samples are on available,
  // we use an encrypted mask vector to protect the information
  // while ensure that the training can still be processed.
  // Note that for the root node, all the samples are available and every
  // party can initialize an encrypted mask vector with [1]
  // for the labels, the format should be encrypted (with size class_num * sample_num),
  // and is known to other parties as well (should be sent before calling this train method)
  log_info("[DecisionTreeBuilder.lime_train] ************* Training Start *************");
  const clock_t training_start_time = clock();

  // pre-compute label helper and feature helper
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();
  log_info("[DecisionTreeBuilder.lime_train]: finished init label and features");

  int sample_num = (int) training_data.size();
  int label_size = class_num * sample_num;
  log_info("[DecisionTreeBuilder.lime_train]: sample_num = "
               + std::to_string(sample_num) + ", label_size = " + std::to_string(label_size));
  std::vector<int> available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    available_feature_ids.push_back(i);
  }
  auto * sample_mask_iv = new EncodedNumber[sample_num];
  initialize_sample_mask_iv(party, sample_mask_iv, sample_num);

  // check whether use sample_weights, if so, compute cipher multi
  auto* weighted_encrypted_true_labels = new EncodedNumber[label_size];
  if (use_sample_weights) {
    auto* assist_encrypted_weights = new EncodedNumber[label_size];
    for (int i = 0; i < sample_num; i++) {
      assist_encrypted_weights[i] = encrypted_weights[i];
      assist_encrypted_weights[i+sample_num] = encrypted_weights[i];
    }
    ciphers_multi(party, weighted_encrypted_true_labels, encrypted_true_labels,
                  assist_encrypted_weights, label_size, ACTIVE_PARTY_ID);
    broadcast_encoded_number_array(party, weighted_encrypted_true_labels, class_num * sample_num, ACTIVE_PARTY_ID);
    delete [] assist_encrypted_weights;
  } else {
    for (int i = 0; i < label_size; i++) {
      weighted_encrypted_true_labels[i] = encrypted_true_labels[i];
    }
  }

  // TODO: make the label precision automatically match
  // truncate encrypted labels to PHE_FIXED_POINT_PRECISION
  truncate_ciphers_precision(party, weighted_encrypted_true_labels, label_size,
                             ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

  // init the root node info
  tree.nodes[0].depth = 0;
  tree.nodes[0].node_sample_num = train_data_size;
  //  EncodedNumber max_impurity;
  //  max_impurity.set_double(phe_pub_key->n[0], MAX_IMPURITY, PHE_FIXED_POINT_PRECISION);
  //  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, max_impurity);
  EncodedNumber enc_root_impurity;
  double root_impurity = lime_reg_tree_root_impurity(
      party, enc_root_impurity, use_encrypted_labels, weighted_encrypted_true_labels,
      sample_num, class_num, use_sample_weights, encrypted_weights);
  tree.nodes[0].impurity = enc_root_impurity;

  // required by spdz connector and mpc computation
  bigint::init_thread();
  // recursively build the tree
  build_node(party,0,available_feature_ids, sample_mask_iv,
             weighted_encrypted_true_labels, use_sample_weights, encrypted_weights);
  log_info("[DecisionTreeBuilder.lime_train] tree capacity = " + std::to_string(tree.capacity));

  const clock_t training_finish_time = clock();
  double training_consumed_time = double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("[DecisionTreeBuilder.lime_train] Training time = " + std::to_string(training_consumed_time));
  log_info("[DecisionTreeBuilder.lime_train] ************* Training Finished *************");

  delete [] sample_mask_iv;
  delete [] weighted_encrypted_true_labels;
}

void DecisionTreeBuilder::distributed_train(const Party &party, const Worker &worker) {
  log_info("************* [DT_train_worker.distributed_train]: distributed train Start *************");
  const clock_t training_start_time = clock();

  // 1. pre-compute label helper and feature helper based on current features
  // calculate r1 and r2, in form of [[1], [0] ...[1], [0]...]
  // indicator_class_vecs and feature_helpers
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();

  int sample_num = train_data_size;
  // label_size = len(indicator_class_vecs), len([r1, r2])
  int label_size = class_num * sample_num;
  // default available feature ids
  std::vector<int> init_available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    init_available_feature_ids.push_back(i);
  }
  log_info("[DT_train_worker.distributed_train]: step 1, finished init label and features");

  // 2. init mask vector and encrypted labels
  auto * init_sample_mask_iv = new EncodedNumber[sample_num];
  initialize_sample_mask_iv(party, init_sample_mask_iv, sample_num);
  // if active party, compute the encrypted label info and broadcast and if passive, receive it
  auto * init_encrypted_labels = new EncodedNumber[class_num * sample_num];
  initialize_encrypted_labels(party, init_encrypted_labels, sample_num);

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // 3. init the root node info
  tree.nodes[0].depth = 0;
  tree.nodes[0].node_sample_num = train_data_size;
  calc_root_impurity(party);
  log_info("[DT_train_worker.distributed_train]: step 3, finished init root node info");

  // call the distributed build node logic
  distributed_build_nodes(party, worker, sample_num, init_available_feature_ids,
                          init_sample_mask_iv, init_encrypted_labels,
                          false, nullptr);

  delete [] init_sample_mask_iv;
  delete [] init_encrypted_labels;
}

void DecisionTreeBuilder::distributed_lime_train(Party party,
                                                 const Worker &worker,
                                                 bool use_encrypted_labels,
                                                 EncodedNumber *encrypted_true_labels,
                                                 bool use_sample_weights,
                                                 EncodedNumber *encrypted_sample_weights) {
  log_info("************* [DT_train_worker.distributed_train]: distributed train Start *************");
  const clock_t training_start_time = clock();

  // 1. pre-compute label helper and feature helper based on current features
  // calculate r1 and r2, in form of [[1], [0] ...[1], [0]...]
  // indicator_class_vecs and feature_helpers
  precompute_label_helper(party.party_type);
  precompute_feature_helpers();

  int sample_num = train_data_size;
  // label_size = len(indicator_class_vecs), len([r1, r2])
  int label_size = class_num * sample_num;
  // default available feature ids
  std::vector<int> init_available_feature_ids;
  for (int i = 0; i < local_feature_num; i++) {
    init_available_feature_ids.push_back(i);
  }
  log_info("[DT_train_worker.distributed_train]: step 1, finished init label and features");

  // 2. init mask vector and encrypted labels
  auto * init_sample_mask_iv = new EncodedNumber[sample_num];
  initialize_sample_mask_iv(party, init_sample_mask_iv, sample_num);

  // check whether use sample_weights, if so, compute cipher multi
  auto* init_encrypted_labels = new EncodedNumber[label_size];
  if (use_sample_weights) {
    auto* assist_encrypted_weights = new EncodedNumber[label_size];
    for (int i = 0; i < sample_num; i++) {
      assist_encrypted_weights[i] = encrypted_sample_weights[i];
      assist_encrypted_weights[i+sample_num] = encrypted_sample_weights[i];
    }
    ciphers_multi(party, init_encrypted_labels, encrypted_true_labels,
                  assist_encrypted_weights, label_size, ACTIVE_PARTY_ID);
    broadcast_encoded_number_array(party, init_encrypted_labels, class_num * sample_num, ACTIVE_PARTY_ID);
    delete [] assist_encrypted_weights;
  } else {
    for (int i = 0; i < label_size; i++) {
      init_encrypted_labels[i] = encrypted_true_labels[i];
    }
  }
  // TODO: make the label precision automatically match
  // truncate encrypted labels to PHE_FIXED_POINT_PRECISION
  truncate_ciphers_precision(party, init_encrypted_labels, label_size,
                             ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // 3. init the root node info
  tree.nodes[0].depth = 0;
  tree.nodes[0].node_sample_num = train_data_size;
  EncodedNumber enc_root_impurity;
  double root_impurity = lime_reg_tree_root_impurity(
      party, enc_root_impurity, use_encrypted_labels, init_encrypted_labels,
      sample_num, class_num, use_sample_weights, encrypted_sample_weights);
  tree.nodes[0].impurity = enc_root_impurity;
  log_info("[DT_train_worker.distributed_train]: step 3, finished init root node info");

  // call the distributed build node logic
  distributed_build_nodes(party, worker, sample_num, init_available_feature_ids,
                          init_sample_mask_iv, init_encrypted_labels,
                          use_sample_weights, encrypted_sample_weights);

  delete [] init_sample_mask_iv;
  delete [] init_encrypted_labels;
}

void DecisionTreeBuilder::distributed_build_nodes(const Party &party,
                                                  const Worker &worker,
                                                  int sample_num,
                                                  std::vector<int> init_available_feature_ids,
                                                  EncodedNumber *init_sample_mask_iv,
                                                  EncodedNumber *init_encrypted_labels,
                                                  bool use_sample_weights,
                                                  EncodedNumber *encrypted_sample_weights) {
  // init map to main node_index: encrypted_label and encrypted_mask_iv
  map<int, EncodedNumber*> node_index_sample_mask_iv;
  map<int, EncodedNumber*> node_index_encrypted_labels;
  map<int, std::vector<int>> node_index_available_features;

  // append the nodex_index 0
  node_index_sample_mask_iv[0] = init_sample_mask_iv;
  node_index_encrypted_labels[0] = init_encrypted_labels;
  node_index_available_features[0] = init_available_feature_ids;

  // 4. begin to distributed train
  int iter = 0;
  int node_index = 0;

  EncodedNumber* current_used_sample_mask_iv;
  EncodedNumber* current_used_encrypted_labels;
  std::vector<int> current_used_available_features_ids;
  std::vector<EncodedNumber*> garbage_collection;

  while (true) {
    const clock_t node_start_time = clock();
    std::string received_str;
    worker.recv_long_message_from_ps(received_str);
    // only break after receiving ps's stop message
    if (received_str == "stop"){
      log_info("[DT_train_worker.distributed_train]: step 6, -------- Worker stop training at Iteration " + std::to_string(iter) + "-------- ");
      break;
    }
      // receive LEAF
    else if (received_str == "LEAF") {
      // receive node index
      std::string received_node_index;
      worker.recv_long_message_from_ps(received_node_index);
      int lef_node_index = std::stoi( received_node_index );
      // receive node label
      std::string received_label_str;
      EncodedNumber label;
      worker.recv_long_message_from_ps(received_label_str);
      deserialize_encoded_number(label, received_label_str);

      tree.nodes[lef_node_index].label = label;
      tree.nodes[lef_node_index].node_type = falcon::LEAF;

      log_info("[DT_train_worker.distributed_train]: step 6, -------- Worker mark node_index =" +
          std::to_string(lef_node_index) + " to leaf node");
      continue;
    }
      // receive node index
    else{
      node_index = std::stoi( received_str );
      log_info("[DT_train_worker.distributed_train]: step 6, -------- Worker update tree, node index :"+
          received_str + " at Iteration " + std::to_string(iter) + "-------- ");
      current_used_sample_mask_iv = node_index_sample_mask_iv[node_index];
      current_used_encrypted_labels = node_index_encrypted_labels[node_index];
      current_used_available_features_ids = node_index_available_features[node_index];
      log_info("[DT_train_worker.distributed_train]: step 6, read sample_mask_iv, labels, available features from map done! ");
    }

    // 4.2: invoke to find the best split using phe and spdz
    std::vector<int> party_split_nums;
    std::vector<double> res = find_best_split(party, node_index,
                                              current_used_available_features_ids, current_used_sample_mask_iv,
                                              current_used_encrypted_labels, party_split_nums,
                                              use_sample_weights, encrypted_sample_weights);

    res.push_back(worker.worker_id - 1);
    log_info("[DT_train_worker.distributed_train]: step 4.5, Finished computation with spdz");

    // step 4.6. send to ps and receive the best split for this node
    std::string local_best_split_str;
    serialize_double_array(res, local_best_split_str);
    worker.send_long_message_to_ps(local_best_split_str);

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
    encrypt_left_right_impu(party, left_impurity, right_impurity, encrypted_left_impurity, encrypted_right_impurity);

    // step 4.7: update tree nodes, including sample iv for iterative node computation

    std::vector<int> available_feature_ids_new;
    auto *sample_mask_iv_left = new EncodedNumber[sample_num];
    auto *sample_mask_iv_right = new EncodedNumber[sample_num];
    auto *encrypted_labels_left = new EncodedNumber[sample_num * class_num];
    auto *encrypted_labels_right = new EncodedNumber[sample_num * class_num];

    garbage_collection.push_back(sample_mask_iv_left);
    garbage_collection.push_back(sample_mask_iv_right);
    garbage_collection.push_back(encrypted_labels_left);
    garbage_collection.push_back(encrypted_labels_right);

    int left_child_index = 2 * node_index + 1;
    int right_child_index = 2 * node_index + 2;

    node_index_sample_mask_iv[left_child_index] = sample_mask_iv_left;
    node_index_sample_mask_iv[right_child_index] = sample_mask_iv_right;
    node_index_encrypted_labels[left_child_index] = encrypted_labels_left;
    node_index_encrypted_labels[right_child_index] = encrypted_labels_right;

    // step 4.8: find party-id, for this global best split
    // convert the index_in_global_split_num to (i_*, index_*)
    int i_star, index_star;
    if (worker.worker_id == best_split_worker_id + 1) {
      best_split_party_idx(best_split_index, party_split_nums, i_star, index_star);
      log_info("[DT_train_worker.distributed_train]: step 4.8, Best split party: i_star = " + to_string(i_star));
      log_info("[DT_train_worker.distributed_train]: step 4.8, Best split index star: index_star = " + to_string(index_star));

      // send party_id to ps
      worker.send_long_message_to_ps(to_string(i_star));
      worker.send_long_message_to_ps(to_string(index_star));
    }

    string recv_i_star_str, recv_index_star_str;
    worker.recv_long_message_from_ps(recv_i_star_str);
    worker.recv_long_message_from_ps(recv_index_star_str);
    i_star = stoi(recv_i_star_str);
    index_star = stoi(recv_index_star_str);

    log_info("[DT_train_worker.distributed_train]: step 4.8, receive from ps, Best split party: i_star = " + to_string(i_star));
    log_info("[DT_train_worker.distributed_train]: step 4.8, receive from ps, Best split index star: index_star = " + to_string(index_star));

    // step 4.9: find feature-id and split-it for this global best split, logic is different for different role
    // if the party has the best split:
    // compute locally and broadcast, find the j_* feature and s_* split
    if (i_star == party.party_id && best_split_worker_id + 1 == worker.worker_id) {
      // define j_star is the best feature id, s_star is best split id of j_star
      int j_star, s_star;
      best_feature_split_idx(index_star, current_used_available_features_ids, j_star, s_star);

      // match worker update mask and labels info, and send to both ps and other party
      // update available features
      for (int feature_id : current_used_available_features_ids) {
        if (j_star != feature_id) {
          available_feature_ids_new.push_back(feature_id);
        }
      }
      int global_j_star = j_star + worker.get_train_feature_prefix();
      log_info("[DT_train_worker.distributed_train]: step 4.9, i_str and worker are matched with best split,"
               "the Best feature id: j_star = " + to_string(j_star) +
          "Best split id: s_star = " + to_string(s_star) + ", global_j_star = " + to_string(global_j_star));

      // update the current node info
      update_node_info(node_index, falcon::INTERNAL, 1, i_star, global_j_star, s_star,
                       feature_helpers[j_star].split_values[s_star],
                       left_child_index, right_child_index,
                       encrypted_left_impurity, encrypted_right_impurity);

      // compute mask iv and labels for child nodes
      std::string update_str_sample_iv, update_str_encrypted_labels_left, update_str_encrypted_labels_right;
      update_mask_iv_and_labels(party, j_star, s_star, sample_num,
                                current_used_sample_mask_iv, current_used_encrypted_labels,
                                encrypted_left_impurity,
                                encrypted_right_impurity,
                                sample_mask_iv_left, sample_mask_iv_right,
                                encrypted_labels_left, encrypted_labels_right,
                                update_str_sample_iv,
                                update_str_encrypted_labels_left,
                                update_str_encrypted_labels_right);
      // send to other parties
      for (int i = 0; i < party.party_num; i++) {
        if (i != party.party_id) {
          party.send_long_message(i, update_str_sample_iv);
          party.send_long_message(i, update_str_encrypted_labels_left);
          party.send_long_message(i, update_str_encrypted_labels_right);
        }
      }
      // send to ps
      worker.send_long_message_to_ps(update_str_sample_iv);
      worker.send_long_message_to_ps(update_str_encrypted_labels_left);
      worker.send_long_message_to_ps(update_str_encrypted_labels_right);

#ifdef DEBUG
      mpz_t t;
      mpz_init(t);
      sample_mask_iv_right[sample_num-1].getter_n(t);
      gmp_printf("[DT_train_worker.distributed_train]: step 4.9, sample_mask_iv_right[sample_num-1].n = %Zd", t);
      mpz_clear(t);
#endif
    }

    // every other worker of matched party update the local tree model
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

      // update the current node info (set split threshold = -1 for simplicity)
      update_node_info(node_index, falcon::INTERNAL, 0, recv_best_party_id,
                       recv_best_feature_id, recv_best_split_id,-1,
                       left_child_index, right_child_index,
                       recv_left_impurity, recv_right_impurity);
      available_feature_ids_new = current_used_available_features_ids;
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

      // update the current node info (set split threshold = -1 for simplicity)
      update_node_info(node_index, falcon::INTERNAL, 0, recv_best_party_id,
                       recv_best_feature_id, recv_best_split_id,-1,
                       left_child_index, right_child_index,
                       recv_left_impurity, recv_right_impurity);
      available_feature_ids_new = current_used_available_features_ids;
    }

    tree.internal_node_num += 1;
    tree.total_node_num += 1;

    node_index_available_features[left_child_index] = available_feature_ids_new;
    node_index_available_features[right_child_index] = available_feature_ids_new;

    /// step 5: clear and log time used

    const clock_t node_finish_time = clock();
    double node_consumed_time = double (node_finish_time - node_start_time) / CLOCKS_PER_SEC;

    log_info("[DT_train_worker.distributed_train]: step 5, time used" + to_string(node_consumed_time));

    iter ++;
  }

  log_info("[DT_train_worker.distributed_train]: step 6, clear used memory");

  for (auto ele: garbage_collection){
    delete [] ele;
  }
  node_index_sample_mask_iv.erase(node_index_sample_mask_iv.begin(), node_index_sample_mask_iv.end());
  node_index_encrypted_labels.erase(node_index_encrypted_labels.begin(), node_index_encrypted_labels.end());
  node_index_available_features.erase(node_index_available_features.begin(), node_index_available_features.end());
}

void DecisionTreeBuilder::eval(Party party, falcon::DatasetType eval_type,
                               const std::string& report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("[DecisionTreeBuilder.eval] ************* Evaluation on " + dataset_str + " Start *************");
  const clock_t testing_start_time = clock();

  // step 1: init test data
  int dataset_size = (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector< std::vector<double> > cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  calc_eval_accuracy(party, eval_type, cur_test_dataset, cur_test_dataset_labels);

  const clock_t testing_finish_time = clock();
  double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  log_info("[DecisionTreeBuilder.eval] Evaluation time = " + std::to_string(testing_consumed_time));
  log_info("[DecisionTreeBuilder.eval] ************* Evaluation on " + dataset_str + " Finished *************");
}

void DecisionTreeBuilder::distributed_eval(Party &party, const Worker &worker, falcon::DatasetType eval_type) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("[DT_train_worker.distributed_eval]: ----- Evaluation on " + dataset_str + "Start -----");
  const clock_t testing_start_time = clock();

  // step 1: init full dataset, used dataset.
  log_info("[DT_train_worker.distributed_eval]: step 1 init full dataset, used dataset ");
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

  log_info("[DT_train_worker.distributed_eval]: step 2 Worker Iteration, worker.receive sample id success");
  log_info("[DT_train_worker.distributed_eval]: step 2 Worker received batch_size " +
      std::to_string(sample_indexes.size()) +
      ", current worker id" + std::to_string(worker.worker_id) +
      ", first last index are [" + to_string(sample_indexes[0]) + ", " + to_string(sample_indexes.back()) + "]");

  // generate used vector
  int dataset_size = sample_indexes.size();
  std::vector<std::vector<double> > cur_test_dataset;
  std::vector<double> cur_test_dataset_labels;
  for (int index : sample_indexes) {
    cur_test_dataset.push_back(full_dataset[index]);
    if (party.party_type == falcon::ACTIVE_PARTY) {
      cur_test_dataset_labels.push_back(full_dataset_labels[index]);
    }
  }

  calc_eval_accuracy(party, eval_type, cur_test_dataset, cur_test_dataset_labels);

  if (party.party_type == falcon::ACTIVE_PARTY) {
    log_info("[DT_train_worker.distributed_eval]: step 5.6 send training_accuracy = " +
        to_string(training_accuracy) + " and test accuracy = " +
        to_string(testing_accuracy) + " to ps");
    if (eval_type == falcon::TRAIN) {
      // map back to the full dataset size level
      training_accuracy = (training_accuracy * dataset_size) / full_dataset_size;
      worker.send_long_message_to_ps(to_string(training_accuracy));
    }
    if (eval_type == falcon::TEST) {
      // map back to the full dataset size level
      testing_accuracy = (testing_accuracy * dataset_size) / full_dataset_size;
      worker.send_long_message_to_ps(to_string(testing_accuracy));
    }
  }

  const clock_t testing_finish_time = clock();
  double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  log_info("[DT_train_worker.distributed_eval]: Evaluation time = " + std::to_string(testing_consumed_time));
  log_info("[DT_train_worker.distributed_eval] ************* Evaluation on " + dataset_str + " Finished *************");
}

void DecisionTreeBuilder::calc_eval_accuracy(Party &party,
    falcon::DatasetType eval_type,
    const std::vector<std::vector<double>> &eval_dataset,
    const std::vector<double> &eval_dataset_labels) {
  // Testing procedure:
  // 1. Organize the leaf label vector, and record the map
  //     between tree node index and leaf index
  // 2. For each sample in the testing dataset, search the whole tree
  //     and do the following:
  //     2.1 if meet feature that not belong to self, mark 1,
  //         and iteratively search left and right branches with 1
  //     2.2 if meet feature that belongs to self, compare with the split value,
  //         mark satisfied branch with 1 while the other branch with 0,
  //         and iteratively search left and right branches
  //     2.3 if meet the leaf node, record the corresponding leaf index
  //         with current value
  // 3. After each client obtaining a 0-1 vector of leaf nodes, do the following:
  //     3.1 the "party_num-1"-th party element-wise multiply with leaf
  //         label vector, and encrypt the vector, send to the next party
  //     3.2 every party on the Robin cycle updates the vector
  //         by element-wise homomorphic multiplication, and send to the next
  //     3.3 the last party, i.e., client 0 get the final encrypted vector
  //         and homomorphic add together, call share decryption
  // 4. If label is matched, correct_num += 1, otherwise, continue
  // 5. Return the final test accuracy by correct_num / dataset.size()

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  int dataset_size = (int) eval_dataset.size();

  // step 2: call tree model predict function to obtain predicted_labels
  auto* predicted_labels = new EncodedNumber[dataset_size];
  tree.predict(party, eval_dataset, dataset_size, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  auto* decrypted_labels = new EncodedNumber[dataset_size];
  collaborative_decrypt(party, predicted_labels, decrypted_labels, dataset_size, ACTIVE_PARTY_ID);

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
        if (predicted_label_vector[i] == eval_dataset_labels[i]) {
          correct_num += 1;
        }
      }
      if (eval_type == falcon::TRAIN) {
        training_accuracy = (double) correct_num / dataset_size;
        log_info("[DecisionTreeBuilder.calc_eval_accuracy] Dataset size = " + std::to_string(dataset_size)
                     + ", correct predicted num = " + std::to_string(correct_num)
                     + ", training accuracy = " + std::to_string(training_accuracy));
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = (double) correct_num / dataset_size;
        log_info("[DecisionTreeBuilder.calc_eval_accuracy] Dataset size = " + std::to_string(dataset_size)
                     + ", correct predicted num = " + std::to_string(correct_num)
                     + ", testing accuracy = " + std::to_string(testing_accuracy));
      }
    } else {
      if (eval_type == falcon::TRAIN) {
        training_accuracy = mean_squared_error(predicted_label_vector, eval_dataset_labels);
        log_info("[DecisionTreeBuilder.calc_eval_accuracy] Training accuracy = " + std::to_string(training_accuracy));
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = mean_squared_error(predicted_label_vector, eval_dataset_labels);
        log_info("[DecisionTreeBuilder.calc_eval_accuracy] Testing accuracy = " + std::to_string(testing_accuracy));
      }
    }
  }

  delete [] predicted_labels;
  delete [] decrypted_labels;
  djcs_t_free_public_key(phe_pub_key);
}

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
  log_info("[spdz_tree_computation] begin connect to spdz parties");
  log_info("[spdz_tree_computation] party_num = " + std::to_string(party_num));
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
    std::cout << "Set up socket connections for " << i << "-th spdz party succeed,"
                  " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_tree_port_bases[i] + i << "." << endl;
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                 " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_tree_port_bases[i] + i << ".";
  }
  log_info("[spdz_tree_computation] Finish setup socket connections to spdz engines.");
  int type = specification.get<int>();
  switch (type)
  {
    case 'p':
    {
      gfp::init_field(specification.get<bigint>());
      std::cout << "Using prime " << gfp::pr() << std::endl;
      LOG(INFO) << "Using prime " << gfp::pr();
      break;
    }
    default:
      log_info("[spdz_tree_computation] Type " + std::to_string(type) + " not implemented");
      exit(EXIT_FAILURE);
  }
  log_info("[spdz_tree_computation] Finish initializing gfp field.");
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size() << std::endl;
  // send data to spdz parties
  log_info("[spdz_tree_computation] party_id = " + std::to_string(party_id));
  if (party_id == ACTIVE_PARTY_ID) {
    // the active party sends computation id for spdz computation
    std::vector<int> computation_id;
    computation_id.push_back(tree_comp_type);
    log_info("[spdz_tree_computation] tree_comp_type = " + std::to_string(tree_comp_type));
    send_public_values(computation_id, mpc_sockets, party_num);
    // the active party sends tree type and class num to spdz parties
    for (int i = 0; i < public_value_size; i++) {
      std::vector<int> x;
      x.push_back(public_values[i]);
      send_public_values(x, mpc_sockets, party_num);
    }
  }
  // all the parties send private shares
  log_info("[spdz_tree_computation] private value size = " + std::to_string(private_value_size));
  for (int i = 0; i < private_value_size; i++) {
    vector<double> x;
    x.push_back(private_values[i]);
    send_private_inputs(x, mpc_sockets, party_num);
  }

  // receive result from spdz parties according to the computation type
  switch (tree_comp_type) {
    case falcon::PRUNING_CHECK: {
      log_info("[spdz_tree_computation] SPDZ tree computation pruning check returned");
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, 1);
      res->set_value(return_values);
      break;
    }
    case falcon::COMPUTE_LABEL: {
      log_info("[spdz_tree_computation] SPDZ tree computation calculate leaf label returned");
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, 1);
      res->set_value(return_values);
      break;
    }
    case falcon::FIND_BEST_SPLIT: {
      log_info("[spdz_tree_computation] SPDZ tree computation type find best split returned");
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
      log_info("[spdz_tree_computation] SPDZ tree computation type compute squared residuals");
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, private_value_size);
      res->set_value(return_values);
      break;
    }
    case falcon::GBDT_SOFTMAX: {
      log_info("[spdz_tree_computation] SPDZ tree computation type compute softmax results");
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, private_value_size);
      res->set_value(return_values);
      break;
    }
    case falcon::RF_LABEL_MODE: {
      log_info("[spdz_tree_computation] SPDZ tree computation type find random forest mode label");
      // public_values[1] denotes the number of predicted samples for computing mode
      log_info("[spdz_tree_computation] public_values[1], i.e., #predicted sample size = " + std::to_string(public_values[1]));
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[1]);
      res->set_value(return_values);
      break;
    }
    case falcon::GBDT_EXPIT: {
      log_info("[spdz_tree_computation] SPDZ tree computation type compute expit(raw_predictions)");
      // public_values[0] denotes the number of samples
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[0]);
      res->set_value(return_values);
      break;
    }
    case falcon::GBDT_UPDATE_TERMINAL_REGION: {
      log_info("[spdz_tree_computation] SPDZ tree computation type compute terminal region new labels");
      // public_values[1] denotes the number of leaf nodes
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[1]);
      res->set_value(return_values);
      break;
    }
    default:
      log_info("[spdz_tree_computation] SPDZ tree computation type is not found.");
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

TreeModel DecisionTreeBuilder::aggregate_decrypt_tree_model(Party& party) {
  log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] start aggregating the tree model");
  TreeModel agg_model;
  int maximum_nodes = (int) pow(2, max_depth + 1) - 1;
  std::vector<int> parties_feature_nums = sync_up_int_arr(party, local_feature_num);
  for (int i = 0; i < (int) parties_feature_nums.size(); i++) {
    log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] parties_features_nums["
      + std::to_string(i) + "] = " + std::to_string(parties_feature_nums[i]));
  }
  if (party.party_type == falcon::ACTIVE_PARTY) {
    agg_model = tree;
    // store the party's tree models
    TreeModel* tree_models = new TreeModel[party.party_num];
    tree_models[0] = tree;
    // collect the tree models from other parties
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_tree_model_id;
        party.recv_long_message(id, recv_tree_model_id);
        deserialize_tree_model(tree_models[id], recv_tree_model_id);
      }
    }
    // aggregate the tree model
    int root_node_idx = 0;
    std::queue<int> q;
    q.push(root_node_idx);

    for (int i = 0; i < maximum_nodes; i++) {
      if (agg_model.nodes[i].is_self_feature == 1) {
        continue;
      } else {
        // check the corresponding party_id and local index id
        int party_id = agg_model.nodes[i].best_party_id;
        int idx_id = agg_model.nodes[i].best_feature_id;
        log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] party_id = " + std::to_string(party_id));
        log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] idx_id = " + std::to_string(idx_id));
        if (party_id != -1) {
          // compute the global index id
          int global_idx_id = global_idx(parties_feature_nums, party_id, idx_id);
          log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] global_idx_id = " + std::to_string(global_idx_id));
          // copy the node from the corresponding party_id
          agg_model.nodes[i] = tree_models[party_id].nodes[i];
          // update the global index id
          agg_model.nodes[i].is_self_feature = 1;
          agg_model.nodes[i].best_feature_id = global_idx_id;
        }
      }
    }
    // active party broadcast aggregated tree model
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string agg_model_str;
        serialize_tree_model(agg_model, agg_model_str);
        party.send_long_message(id, agg_model_str);
      }
    }
    delete [] tree_models;
  } else {
    // serialize the tree model and send it to the active party
    std::string tree_model_str;
    serialize_tree_model(tree, tree_model_str);
    party.send_long_message(ACTIVE_PARTY_ID, tree_model_str);

    // receive the aggregate tree model for decryption
    std::string recv_agg_model_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_agg_model_str);
    deserialize_tree_model(agg_model, recv_agg_model_str);
  }

  // parties decrypt the agg_model and return
  // note that now the parties all have the same agg_model, where the
  // impurity and label on each node is ciphertext
  log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] start decrypting the tree model");
  int root_node_idx = 0;
  std::queue<int> q;
  q.push(root_node_idx);
  while (!q.empty()) {
    int idx = q.front();
    q.pop();
    log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] node idx = " + std::to_string(idx));
    if (agg_model.nodes[idx].node_type == falcon::INTERNAL) {
      auto* enc = new EncodedNumber[1];
      auto* dec = new EncodedNumber[1];
      enc[0] = agg_model.nodes[idx].impurity;
      log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] internal node impurity type = " + std::to_string(enc[0].getter_type()));
      collaborative_decrypt(party, enc, dec, 1, ACTIVE_PARTY_ID);
      agg_model.nodes[idx].impurity = dec[0];

      int left_node_index = idx * 2 + 1;
      int right_node_index = idx * 2 + 2;
      q.push(left_node_index);
      q.push(right_node_index);

      delete [] enc;
      delete [] dec;
    }

    if (agg_model.nodes[idx].node_type == falcon::LEAF) {
      auto* enc = new EncodedNumber[2];
      auto* dec = new EncodedNumber[2];
      enc[0] = agg_model.nodes[idx].impurity;
      enc[1] = agg_model.nodes[idx].label;
      log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] leaf node impurity type = " + std::to_string(enc[0].getter_type()));
      log_info("[DecisionTreeBuilder: aggregate_decrypt_tree_model] leaf node label type = " + std::to_string(enc[1].getter_type()));
      collaborative_decrypt(party, enc, dec, 2, ACTIVE_PARTY_ID);
      agg_model.nodes[idx].impurity = dec[0];
      agg_model.nodes[idx].label = dec[1];

      delete [] enc;
      delete [] dec;
    }
  }
  return agg_model;
}