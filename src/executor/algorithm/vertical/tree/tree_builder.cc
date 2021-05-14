//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/operator/mpc/spdz_connector.h>
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
  max_impurity.set_double(phe_pub_key->n[0], MAX_IMPURITY);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, tree.nodes[0].impurity, max_impurity);

  // required by spdz connector and mpc computation
  bigint::init_thread();
  // recursively build the tree
  build_node(party, 0, available_feature_ids, sample_mask_iv, encrypted_labels);

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
            << "depth = " << tree.nodes[node_index].depth << "******" << std::endl;
  LOG(INFO) << "****** Build tree node " << node_index
            << "depth = " << tree.nodes[node_index].depth << "******";
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

  if (node_index >= tree.capacity - 1) {
    LOG(ERROR) << "Node index exceeds the maximum tree depth";
    exit(1);
  }

  /// step 1: check pruning condition
  bool is_satisfied = check_pruning_conditions(party, node_index, sample_mask_iv);

  /// step 2: if satisfied, compute label
  if (is_satisfied) {

  } else {
    /// step 3: start to find the best split and update
  }

  const clock_t node_finish_time = clock();
  double node_consumed_time = double(node_finish_time - node_start_time) / CLOCKS_PER_SEC;
  std::cout << "Node build time = " << node_consumed_time << std::endl;
  LOG(INFO) << "Node build time = " << node_consumed_time;
  google::FlushLogFiles(google::INFO);
}

bool DecisionTreeBuilder::check_pruning_conditions(Party &party,
    int node_index,
    EncodedNumber *sample_mask_iv) {
  bool is_satisfied = false;
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
  label.set_double(phe_pub_key->n[0], res[0], 2 * PHE_FIXED_POINT_PRECISION);
  // now assume that it is only an encoded number, instead of a ciphertext
  // djcs_t_aux_encrypt(phe_pub_key, party.phe_random, label, label);
  tree.nodes[node_index].label = label;
  tree.nodes[node_index].node_type = falcon::LEAF;
  // the other node attributes should be updated in another place

  djcs_t_free_public_key(phe_pub_key);
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

  // send data to spdz parties according to the computation type
  switch (tree_comp_type) {
    case falcon::PRUNING_CHECK: {
      LOG(INFO) << "SPDZ tree computation type is pruning check";
      if (party_id == ACTIVE_PARTY_ID) {
        // the active party sends computation id for spdz computation
        std::vector<int> computation_id;
        computation_id.push_back(tree_comp_type);
        send_public_values(computation_id, mpc_sockets, party_num);
        // the active party sends tree type to spdz parties
        std::vector<int> tree_type;
        tree_type.push_back(public_values[0]);
        send_public_values(tree_type, mpc_sockets, party_num);
      }
      // all the parties send private shares
      for (int i = 0; i < private_value_size; i++) {
        vector<double> x;
        x.push_back(private_values[i]);
        send_private_inputs(x, mpc_sockets, party_num);
      }
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, 1);
      res->set_value(return_values);
      break;
    }
    case falcon::COMPUTE_LABEL: {
      LOG(INFO) << "SPDZ tree computation type is calculate the leaf label";
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
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, 1);
      res->set_value(return_values);
      break;
    }
    case falcon::FIND_BEST_SPLIT: {
      LOG(INFO) << "SPDZ tree computation type is find best split";
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