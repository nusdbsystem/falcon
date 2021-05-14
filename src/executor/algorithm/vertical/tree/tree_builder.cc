//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>

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
  local_feature_num = m_training_data[0].size();
  for (int i = 0; i < local_feature_num; i++) {
    feature_types.push_back(falcon::CONTINUOUS);
  }
  feature_helpers = new FeatureHelper[local_feature_num];
  // init tree object
  tree = Tree(tree_type, class_num, max_depth);
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
  max_impurity.set_double(phe_pub_key->n[0], std::numeric_limits<double>::max());
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, max_impurity);

  // recursively build the tree
  build_node(party, 0, available_feature_ids, sample_mask_iv, encrypted_labels);

  delete [] sample_mask_iv;
  delete [] encrypted_labels;

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
            << "depth = " << tree.nodes[node_index].depth << "******" << std::endl;
  LOG(INFO) << "****** Build tree node " << node_index
            << "depth = " << tree.nodes[node_index].depth << "******";
  const clock_t node_start_time = clock();

  /// 1. check if some pruning conditions are satisfied
  ///      1.1 feature set is empty;
  ///      1.2 the number of samples are less than a pre-defined threshold
  ///      1.3 if classification, labels are same; if regression, label variance is less than a threshold
  /// 2. if satisfied, return a leaf node with majority class or mean label;
  ///      else, continue to step 3
  /// 3. super client computes some encrypted label information and broadcast to the other clients
  /// 4. every client locally compute necessary encrypted statistics, i.e., #samples per class for classification or variance info
  /// 5. the clients convert the encrypted statistics to shares and send to SPDZ parties
  /// 6. wait for SPDZ parties return (i_*, j_*, s_*), where i_* is client id, j_* is feature id, and s_* is split id
  /// 7. client who owns the best split feature do the splits and update mask vector, and broadcast to the other clients
  /// 8. every client updates mask vector and local tree model
  /// 9. recursively build the next two tree nodes

  if (node_index >= tree.capacity - 1) {
    LOG(ERROR) << "Node index exceeds the maximum tree depth";
    exit(1);
  }



  const clock_t node_finish_time = clock();
  double node_consumed_time = double(node_finish_time - node_start_time) / CLOCKS_PER_SEC;
  std::cout << "Node build time = " << node_consumed_time << std::endl;
  LOG(INFO) << "Node build time = " << node_consumed_time;
  google::FlushLogFiles(google::INFO);
}