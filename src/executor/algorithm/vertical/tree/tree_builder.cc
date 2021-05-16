//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>

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
    double m_testing_accuracy) : Model(std::move(m_training_data),
          std::move(m_testing_data),
          std::move(m_training_labels),
          std::move(m_testing_labels),
          m_training_accuracy,
          m_testing_accuracy) {
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
  local_feature_num = training_data[0].size();
  for (int i = 0; i < local_feature_num; i++) {
    feature_types.push_back(falcon::CONTINUOUS);
  }
  feature_helpers = new FeatureHelper[local_feature_num];
  // init tree object
  tree = new Tree(tree_type, class_num, max_depth);
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
  // as the samples are available at the beginning of the training, init with 1
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
    party.recv_long_message(0, recv_result_str);
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

void DecisionTreeBuilder::build_node(Party &party,
    int node_index,
    std::vector<int> available_feature_ids,
    EncodedNumber *sample_mask_iv,
    EncodedNumber *encrypted_labels) {
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
    LOG(ERROR) << "Node index exceeds the maximum tree depth";
    exit(1);
  }

  /// step 1: check pruning condition via spdz computation
  bool is_satisfied = check_pruning_conditions(party, node_index, sample_mask_iv);

  /// step 2: if satisfied, compute label via spdz computation
  if (is_satisfied) {
    compute_leaf_statistics(party, node_index, sample_mask_iv, encrypted_labels);
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
  int available_local_feature_num = available_feature_ids.size();
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


  if (party.party_type == falcon::ACTIVE_PARTY) {
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
          encrypted_right_branch_sample_nums);
    }
    LOG(INFO) << "Finish computing local statistics";

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
      std::string recv_encrypted_statistics_str;
      if (i != party.party_id) {
        party.recv_long_message(i, recv_encrypted_statistics_str);
        int recv_party_id, recv_node_index, recv_split_num, recv_classes_num;
        EncodedNumber **recv_encrypted_statistics;
        EncodedNumber *recv_left_sample_nums;
        EncodedNumber *recv_right_sample_nums;
        deserialize_encrypted_statistics(recv_party_id, recv_node_index,
            recv_split_num, recv_classes_num,
            recv_left_sample_nums, recv_right_sample_nums,
            recv_encrypted_statistics,
            recv_encrypted_statistics_str);

        // pack the encrypted statistics
        if (recv_split_num == 0) {
          party_split_nums.push_back(0);
          continue;
        } else {
          party_split_nums.push_back(recv_split_num);
          for (int j = 0; j < recv_split_num; j++) {
            global_left_branch_sample_nums[global_split_num + j] = recv_left_sample_nums[j];
            global_right_branch_sample_nums[global_split_num + j] = recv_right_sample_nums[j];
            for (int k = 0; k < 2 * class_num; k++) {
              global_encrypted_statistics[global_split_num + j][k] = recv_encrypted_statistics[j][k];
            }
          }
          global_split_num += recv_split_num;
        }

        delete [] recv_left_sample_nums;
        delete [] recv_right_sample_nums;
        for (int xx = 0; xx < recv_split_num; xx++) {
          delete [] recv_encrypted_statistics[xx];
        }
        delete [] recv_encrypted_statistics;
      }
    }
    LOG(INFO) << "The global_split_num = " << global_split_num;
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
    if (local_splits_num == 0) {
      LOG(INFO) << "Local feature used up";
      std::string s;
      serialize_encrypted_statistics(party.party_id, node_index,
          local_splits_num, class_num,
          encrypted_left_branch_sample_nums,
          encrypted_right_branch_sample_nums,
          encrypted_statistics, s);
      party.send_long_message(ACTIVE_PARTY_ID, s);
    } else {
      encrypted_statistics = new EncodedNumber*[local_splits_num];
      for (int i = 0; i < local_splits_num; i++) {
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
      LOG(INFO) << "Finish computing local statistics";
      // send encrypted statistics to the super client
      std::string s;
      serialize_encrypted_statistics(party.party_id, node_index,
          local_splits_num, class_num,
          encrypted_left_branch_sample_nums,
          encrypted_right_branch_sample_nums,
          encrypted_statistics, s);
      party.send_long_message(ACTIVE_PARTY_ID, s);
    }

    std::string recv_split_info_str;
    party.recv_long_message(0, recv_split_info_str);
    deserialize_split_info(global_split_num, party_split_nums, recv_split_info_str);
    LOG(INFO) << "The global_split_num = " << global_split_num;
  }

  /// step 5: encrypted statistics computed finished, convert to secret shares
  std::vector< std::vector<double> > stats_shares;
  std::vector<double> left_sample_nums_shares;
  std::vector<double> right_sample_nums_shares;
  std::vector<int> public_values;
  std::vector<double> private_values;
  party.ciphers_to_secret_shares(global_left_branch_sample_nums,
      left_sample_nums_shares, global_split_num,
      ACTIVE_PARTY_ID, 0);
  party.ciphers_to_secret_shares(global_right_branch_sample_nums,
      right_sample_nums_shares, global_split_num,
      ACTIVE_PARTY_ID, 0);
  for (int i = 0; i < global_split_num; i++) {
    std::vector<double> tmp;
    party.ciphers_to_secret_shares(global_encrypted_statistics[i],
        tmp, 2 * class_num,
        ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
    stats_shares.push_back(tmp);
  }

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
      SPDZ_PORT_TREE,
      party.host_names,
      public_values.size(),
      public_values,
      private_values.size(),
      private_values,
      comp_type,
      &promise_values);
  std::vector<double> res = future_values.get();
  spdz_pruning_check_thread.join();

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
  int sample_num = training_data.size();
  EncodedNumber *sample_mask_iv_left = new EncodedNumber[sample_num];
  EncodedNumber *sample_mask_iv_right = new EncodedNumber[sample_num];
  EncodedNumber *encrypted_labels_left = new EncodedNumber[sample_num * class_num];
  EncodedNumber *encrypted_labels_right = new EncodedNumber[sample_num * class_num];

  int left_child_index = 2 * node_index + 1;
  int right_child_index = 2 * node_index + 2;

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
  LOG(INFO) << "Best split party: i_star = " << i_star;

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

    LOG(INFO) << "Best feature id: j_star = " << j_star;
    LOG(INFO) << "Best split id: s_star = " << s_star;

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

  LOG(INFO) << "End build tree node: " << node_index;
}

bool DecisionTreeBuilder::check_pruning_conditions(Party &party,
    int node_index,
    EncodedNumber *sample_mask_iv) {
  bool is_satisfied = false;

  if (tree.nodes[node_index].depth == max_depth) {
    is_satisfied = true;
    return is_satisfied;
  }

  int sample_num = training_data.size();
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // init temp values
  EncodedNumber *encrypted_sample_count = new EncodedNumber[1];
  EncodedNumber *encrypted_impurity = new EncodedNumber[1];
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

  // check if encrypted sample count is less than a threshold (prune_sample_num),
  // or if the impurity satisfies the pruning condition
  // communicate with spdz parties and receive results
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  std::thread spdz_pruning_check_thread(spdz_tree_computation,
      party.party_num,
      party.party_id,
      SPDZ_PORT_TREE,
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

bool DecisionTreeBuilder::compute_leaf_statistics(Party &party,
    int node_index,
    EncodedNumber *sample_mask_iv,
    EncodedNumber *encrypted_labels) {
  int sample_num = training_data.size();
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  falcon::SpdzTreeCompType comp_type = falcon::COMPUTE_LABEL;
  std::vector<int> public_values;
  std::vector<double> private_values, shares1, shares2;
  public_values.push_back(tree_type);
  public_values.push_back(class_num);
  if (party.party_type == falcon::ACTIVE_PARTY) {
    if (tree_type == falcon::CLASSIFICATION) {
      // compute sample num of each class
      EncodedNumber *class_sample_nums = new EncodedNumber[class_num];
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
      EncodedNumber *label_info = new EncodedNumber[1];
      EncodedNumber *encrypted_sample_num_aux = new EncodedNumber[1];
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
          ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
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
          ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
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
      SPDZ_PORT_TREE,
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

void DecisionTreeBuilder::compute_encrypted_statistics(Party &party,
    int node_index,
    std::vector<int> available_feature_ids,
    EncodedNumber *sample_mask_iv,
    EncodedNumber **encrypted_statistics,
    EncodedNumber *encrypted_labels,
    EncodedNumber *encrypted_left_sample_nums,
    EncodedNumber *encrypted_right_sample_nums) {
  std::cout << " Compute encrypted statistics for node " << node_index << std::endl;
  LOG(INFO) << " Compute encrypted statistics for node " << node_index;
  const clock_t start_time = clock();

  int split_index = 0;
  int available_feature_num = available_feature_ids.size();
  int sample_num = training_data.size();
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

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
    EncodedNumber *sorted_sample_iv = new EncodedNumber[sample_num];

    // copy the sample_iv
    for (int idx = 0; idx < sample_num; idx++) {
      sorted_sample_iv[idx] = sample_mask_iv[sorted_indices[idx]];
    }

    // compute the encrypted aggregation of split_num + 1 buckets
    EncodedNumber *left_sums = new EncodedNumber[split_num];
    EncodedNumber *right_sums = new EncodedNumber[split_num];
    EncodedNumber total_sum;
    total_sum.set_integer(phe_pub_key->n[0], 0);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, total_sum, total_sum);
    for (int idx = 0; idx < split_num; idx++) {
      left_sums[idx].set_integer(phe_pub_key->n[0], 0);
      right_sums[idx].set_integer(phe_pub_key->n[0], 0);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, left_sums[idx], left_sums[idx]);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, right_sums[idx], right_sums[idx]);
    }

    // compute sample iv statistics by one traverse
    int split_iterator = 0;
    for (int sample_idx = 0; sample_idx < sample_num; sample_idx++) {
      djcs_t_aux_ee_add(phe_pub_key, total_sum,
          total_sum, sorted_sample_iv[sample_idx]);
      if (split_iterator == split_num) {
        continue;
      }
      int sorted_idx = sorted_indices[sample_idx];
      float sorted_feature_value = feature_helpers[feature_id].origin_feature_values[sorted_idx];

      // find the first split value that larger than the current feature value, usually only step by 1
      if (sorted_feature_value > feature_helpers[feature_id].split_values[split_iterator]) {
        split_iterator += 1;
        if (split_iterator == split_num) continue;
      }
      djcs_t_aux_ee_add(phe_pub_key, left_sums[split_iterator],
          left_sums[split_iterator], sorted_sample_iv[sample_idx]);
    }

    // compute the encrypted statistics for each class
    EncodedNumber **left_stats = new EncodedNumber*[split_num];
    EncodedNumber **right_stats = new EncodedNumber*[split_num];
    EncodedNumber *sums_stats = new EncodedNumber[class_num];
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
    left_num_help.set_integer(phe_pub_key->n[0], 0);
    right_num_help.set_integer(phe_pub_key->n[0], 0);
    plain_constant_help.set_integer(phe_pub_key->n[0], -1);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
        left_num_help, left_num_help);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
        right_num_help, right_num_help);

    EncodedNumber *left_stat_help = new EncodedNumber[class_num];
    EncodedNumber *right_stat_help = new EncodedNumber[class_num];
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
  std::vector<int> binary_vector(tree.internal_node_num + 1);

  // traverse the whole tree iteratively, and compute binary_vector
  std::stack<PredictHelper> traverse_prediction_objs;
  PredictHelper prediction_obj(tree.nodes[0].node_type,
      tree.nodes[0].is_self_feature,
      tree.nodes[0].best_party_id,
      tree.nodes[0].best_feature_id,
      tree.nodes[0].best_split_id,
      1,
      0);
  traverse_prediction_objs.push(prediction_obj);
  while (!traverse_prediction_objs.empty()) {
    PredictHelper pred_obj = traverse_prediction_objs.top();
    if (pred_obj.is_leaf == 1) {
      // find leaf index and record
      int leaf_index = node_index_2_leaf_index.find(pred_obj.index)->second;
      binary_vector[leaf_index] = pred_obj.mark;
      traverse_prediction_objs.pop();
    } else if (pred_obj.is_self_feature != 1) {
      // both left and right branches are marked as 1 * current_mark
      traverse_prediction_objs.pop();
      int left_node_index = pred_obj.index * 2 + 1;
      int right_node_index = pred_obj.index * 2 + 2;

      PredictHelper left(tree.nodes[left_node_index].node_type,
          tree.nodes[left_node_index].is_self_feature,
          tree.nodes[left_node_index].best_party_id,
          tree.nodes[left_node_index].best_feature_id,
          tree.nodes[left_node_index].best_split_id,
          pred_obj.mark,
          left_node_index);
      PredictHelper right(tree.nodes[right_node_index].node_type,
          tree.nodes[right_node_index].is_self_feature,
          tree.nodes[right_node_index].best_party_id,
          tree.nodes[right_node_index].best_feature_id,
          tree.nodes[right_node_index].best_split_id,
          pred_obj.mark,
          right_node_index);
      traverse_prediction_objs.push(left);
      traverse_prediction_objs.push(right);
    } else {
      // is self feature, retrieve split value and compare
      traverse_prediction_objs.pop();
      int feature_id = pred_obj.best_feature_id;
      int split_id = pred_obj.best_split_id;
      double split_value = feature_helpers[feature_id].split_values[split_id];
      int left_mark, right_mark;
      if (sample_values[feature_id] <= split_value) {
        left_mark = pred_obj.mark * 1;
        right_mark = pred_obj.mark * 0;
      } else {
        left_mark = pred_obj.mark * 0;
        right_mark = pred_obj.mark * 1;
      }

      int left_node_index = pred_obj.index * 2 + 1;
      int right_node_index = pred_obj.index * 2 + 2;
      PredictHelper left(tree.nodes[left_node_index].node_type,
          tree.nodes[left_node_index].is_self_feature,
          tree.nodes[left_node_index].best_party_id,
          tree.nodes[left_node_index].best_feature_id,
          tree.nodes[left_node_index].best_split_id,
          left_mark,
          left_node_index);
      PredictHelper right(tree.nodes[right_node_index].node_type,
          tree.nodes[right_node_index].is_self_feature,
          tree.nodes[right_node_index].best_party_id,
          tree.nodes[right_node_index].best_feature_id,
          tree.nodes[right_node_index].best_split_id,
          right_mark,
          right_node_index);
      traverse_prediction_objs.push(left);
      traverse_prediction_objs.push(right);
    }
  }
  return binary_vector;
}

void DecisionTreeBuilder::eval(Party party, falcon::DatasetType eval_type) {
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

  // step 1: organize the leaf label vector, compute the map
  LOG(INFO) << "Tree internal node num = " << tree.internal_node_num;
  EncodedNumber *label_vector = new EncodedNumber[tree.internal_node_num + 1];
  std::map<int, int> node_index_2_leaf_index_map;
  int leaf_cur_index = 0;
  for (int i = 0; i < pow(2, max_depth + 1) - 1; i++) {
    if (tree.nodes[i].node_type == falcon::LEAF) {
      node_index_2_leaf_index_map.insert(std::make_pair(i, leaf_cur_index));
      label_vector[leaf_cur_index] = tree.nodes[i].label;  // record leaf label vector
      leaf_cur_index ++;
    }
  }

  // record the ciphertext of the label
  EncodedNumber *encrypted_aggregation = new EncodedNumber[dataset_size];
  // for each sample
  for (int i = 0; i < dataset_size; i++) {
    // compute binary vector for the current sample
    std::vector<int> binary_vector = compute_binary_vector(i, node_index_2_leaf_index_map, eval_type);
    EncodedNumber *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
    EncodedNumber *updated_label_vector = new EncodedNumber[binary_vector.size()];

    // update in Robin cycle, from the last client to client 0
    if (party.party_id == party.party_num - 1) {
      // updated_label_vector = new EncodedNumber[binary_vector.size()];
      for (int j = 0; j < binary_vector.size(); j++) {
        encoded_binary_vector[j].set_integer(phe_pub_key->n[0], binary_vector[j]);
        djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j],
            label_vector[j], encoded_binary_vector[j]);
      }
      // send to the next client
      std::string send_s;
      serialize_encoded_number_array(updated_label_vector, binary_vector.size(), send_s);
      party.send_long_message(party.party_id - 1, send_s);
    } else if (party.party_id > 0) {
      std::string recv_s;
      party.recv_long_message(party.party_id + 1, recv_s);
      deserialize_encoded_number_array(updated_label_vector, binary_vector.size(), recv_s);
      for (int j = 0; j < binary_vector.size(); j++) {
        encoded_binary_vector[j].set_integer(phe_pub_key->n[0], binary_vector[j]);
        djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j],
            updated_label_vector[j], encoded_binary_vector[j]);
      }
      std::string resend_s;
      serialize_encoded_number_array(updated_label_vector, binary_vector.size(), resend_s);
      party.send_long_message(party.party_id - 1, resend_s);
    } else {
      // the super client update the last, and aggregate before calling share decryption
      std::string final_recv_s;
      party.recv_long_message(party.party_id + 1, final_recv_s);
      deserialize_encoded_number_array(updated_label_vector, binary_vector.size(), final_recv_s);
      for (int j = 0; j < binary_vector.size(); j++) {
        encoded_binary_vector[j].set_integer(phe_pub_key->n[0], binary_vector[j]);
        djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
      }
    }

    // aggregate and call share decryption
    if (party.party_type == falcon::ACTIVE_PARTY) {
      encrypted_aggregation[i].set_double(phe_pub_key->n[0], 0, PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         encrypted_aggregation[i], encrypted_aggregation[i]);
      for (int j = 0; j < binary_vector.size(); j++) {
        djcs_t_aux_ee_add(phe_pub_key, encrypted_aggregation[i],
                          encrypted_aggregation[i], updated_label_vector[j]);
      }
    }
    delete [] encoded_binary_vector;
    delete [] updated_label_vector;
  }

  // broadcast the predicted labels and call share decrypt
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::string s;
    serialize_encoded_number_array(encrypted_aggregation, dataset_size, s);
    for (int x = 0; x < party.party_num; x++) {
      if (x != party.party_id) {
        party.send_long_message(x, s);
      }
    }
  } else {
    std::string recv_s;
    party.recv_long_message(0, recv_s);
    deserialize_encoded_number_array(encrypted_aggregation, dataset_size, recv_s);
  }

  // step 3: active party aggregates and call collaborative decryption
  EncodedNumber* decrypted_aggregation = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(encrypted_aggregation,
      decrypted_aggregation,
      dataset_size,
      ACTIVE_PARTY_ID);

  // init predicted_label_vector
  std::vector<double> predicted_label_vector;
  for (int i = 0; i < dataset_size; i++) {
    predicted_label_vector.push_back(0.0);
  }
  for (int i = 0; i < dataset_size; i++) {
    decrypted_aggregation[i].decode(predicted_label_vector[i]);
  }

  // compute accuracy by the super client
  if (party.party_type == falcon::ACTIVE_PARTY) {
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

  delete [] encrypted_aggregation;
  delete [] decrypted_aggregation;

  const clock_t testing_finish_time = clock();
  double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Evaluation time = " << testing_consumed_time;
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Finished *************";
  google::FlushLogFiles(google::INFO);
}

void spdz_tree_computation(int party_num,
    int party_id,
    int mpc_tree_port_base,
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
    set_up_client_socket(plain_sockets[i], party_host_names[i].c_str(), mpc_tree_port_base + i);
    send(plain_sockets[i], (octet*) &party_id, sizeof(int));
    mpc_sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                    "P" + to_string(i), "C" + to_string(party_id), true);
    if (i == 0){
      // receive gfp prime
      specification.Receive(mpc_sockets[0]);
    }
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                 " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_tree_port_base + i << ".";
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
  if (party_id == ACTIVE_PARTY_ID) {
    // the active party sends computation id for spdz computation
    std::vector<int> computation_id;
    computation_id.push_back(tree_comp_type);
    send_public_values(computation_id, mpc_sockets, party_num);
    // the active party sends tree type and class num to spdz parties
    for (int i = 0; i < public_value_size; i++) {
      std::vector<int> x;
      x.push_back(public_values[i]);
      send_public_values(x, mpc_sockets, party_num);
    }
  }
  // all the parties send private shares
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

void train_decision_tree(Party party, std::string params_str,
    std::string model_save_file, std::string model_report_file) {

  LOG(INFO) << "Run the example decision tree train";
  std::cout << "Run the example decision tree train" << std::endl;

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
//  deserialize_dt_params(params, params_str);
  int weight_size = party.getter_feature_num();
  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;

  std::vector< std::vector<double> > training_data;
  std::vector< std::vector<double> > testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  party.split_train_test_data(split_percentage,
                              training_data,
                              testing_data,
                              training_labels,
                              testing_labels);

  LOG(INFO) << "Init decision tree model builder";
  LOG(INFO) << "params.tree_type = " << params.tree_type;
  LOG(INFO) << "params.criterion = " << params.criterion;
  LOG(INFO) << "params.split_strategy = " << params.split_strategy;
  LOG(INFO) << "params.class_num = " << params.class_num;
  LOG(INFO) << "params.max_depth = " << params.max_depth;
  LOG(INFO) << "params.max_bins = " << params.max_bins;
  LOG(INFO) << "params.min_samples_split = " << params.min_samples_split;
  LOG(INFO) << "params.min_samples_leaf = " << params.min_samples_leaf;
  LOG(INFO) << "params.max_leaf_nodes = " << params.max_leaf_nodes;
  LOG(INFO) << "params.min_impurity_decrease = " << params.min_impurity_decrease;
  LOG(INFO) << "params.min_impurity_split = " << params.min_impurity_split;
  LOG(INFO) << "params.dp_budget = " << params.dp_budget;

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

  decision_tree_builder.train(party);
  decision_tree_builder.eval(party, falcon::TRAIN);
  decision_tree_builder.eval(party, falcon::TEST);

  // TODO: save model and report
//  EncodedNumber* model_weights = new EncodedNumber[log_reg_model.getter_weight_size()];
//  log_reg_model.getter_encoded_weights(model_weights);
//  save_lr_model(model_weights, log_reg_model.getter_weight_size(), model_save_file);
//  save_lr_report(training_accuracy, testing_accuracy, model_report_file);
//
//  delete [] model_weights;
}