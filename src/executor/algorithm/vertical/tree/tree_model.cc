//
// Created by wuyuncheng on 13/5/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <cmath>

#include <glog/logging.h>
#include <iostream>
#include <stack>

TreeModel::TreeModel() {}

TreeModel::TreeModel(falcon::TreeType m_type, int m_class_num, int m_max_depth) {
  type = m_type;
  class_num = m_class_num;
  max_depth = m_max_depth;
  internal_node_num = 0;
  total_node_num = 0;
  // the maximum nodes, complete binary tree
  capacity = (int) pow(2, max_depth + 1) - 1;
  nodes = new Node[capacity];
}

TreeModel::~TreeModel() {
  delete [] nodes;
}

TreeModel::TreeModel(const TreeModel &tree) {
  type = tree.type;
  class_num = tree.class_num;
  max_depth = tree.max_depth;
  internal_node_num = tree.internal_node_num;
  total_node_num = tree.total_node_num;
  int maximum_nodes = (int) pow(2, max_depth + 1) - 1;
  nodes = new Node[maximum_nodes];
  for (int i = 0; i < maximum_nodes; i++) {
    nodes[i] = tree.nodes[i];
  }
  capacity = tree.capacity;
}

TreeModel& TreeModel::operator=(const TreeModel &tree) {
  type = tree.type;
  class_num = tree.class_num;
  max_depth = tree.max_depth;
  internal_node_num = tree.internal_node_num;
  total_node_num = tree.total_node_num;
  int maximum_nodes = (int) pow(2, max_depth + 1) - 1;
  nodes = new Node[maximum_nodes];
  for (int i = 0; i < maximum_nodes; i++) {
    nodes[i] = tree.nodes[i];
  }
  capacity = tree.capacity;
}

std::vector<int> TreeModel::comp_predict_vector(std::vector<double> sample,
    std::map<int, int> node_index_2_leaf_index_map) {

  std::vector<int> binary_vector(internal_node_num + 1);
  // traverse the whole tree iteratively, and compute binary_vector
  std::stack<PredictHelper> traverse_prediction_objs;
  // add root node to stack
  PredictHelper prediction_obj((bool) nodes[0].node_type,
                               (bool) nodes[0].is_self_feature,
                               nodes[0].best_party_id,
                               nodes[0].best_feature_id,
                               nodes[0].best_split_id,
                               1,
                               0);
  traverse_prediction_objs.push(prediction_obj);
  while (!traverse_prediction_objs.empty()) {
    PredictHelper pred_obj = traverse_prediction_objs.top();
    if (pred_obj.is_leaf == 1) {
      // find leaf index and record
      int leaf_index = node_index_2_leaf_index_map.find(pred_obj.index)->second;
      binary_vector[leaf_index] = pred_obj.mark;
      traverse_prediction_objs.pop();
    } else if (pred_obj.is_self_feature != 1) {
      // both left and right branches are marked as 1 * current_mark
      traverse_prediction_objs.pop();
      int left_node_index = pred_obj.index * 2 + 1;
      int right_node_index = pred_obj.index * 2 + 2;

      PredictHelper left((bool) nodes[left_node_index].node_type,
                         (bool) nodes[left_node_index].is_self_feature,
                         nodes[left_node_index].best_party_id,
                         nodes[left_node_index].best_feature_id,
                         nodes[left_node_index].best_split_id,
                         pred_obj.mark,
                         left_node_index);
      PredictHelper right((bool) nodes[right_node_index].node_type,
                          (bool) nodes[right_node_index].is_self_feature,
                          nodes[right_node_index].best_party_id,
                          nodes[right_node_index].best_feature_id,
                          nodes[right_node_index].best_split_id,
                          pred_obj.mark,
                          right_node_index);
      traverse_prediction_objs.push(left);
      traverse_prediction_objs.push(right);
    } else {
      // is self feature, retrieve split value and compare
      traverse_prediction_objs.pop();
      int node_index = pred_obj.index;
      int feature_id = pred_obj.best_feature_id;
      int split_id = pred_obj.best_split_id;
      double split_value = nodes[node_index].split_threshold;
      int left_mark, right_mark;
      if (sample[feature_id] <= split_value) {
        left_mark = pred_obj.mark * 1;
        right_mark = pred_obj.mark * 0;
      } else {
        left_mark = pred_obj.mark * 0;
        right_mark = pred_obj.mark * 1;
      }

      int left_node_index = pred_obj.index * 2 + 1;
      int right_node_index = pred_obj.index * 2 + 2;
      PredictHelper left((bool) nodes[left_node_index].node_type,
                         (bool) nodes[left_node_index].is_self_feature,
                         nodes[left_node_index].best_party_id,
                         nodes[left_node_index].best_feature_id,
                         nodes[left_node_index].best_split_id,
                         left_mark,
                         left_node_index);
      PredictHelper right((bool) nodes[right_node_index].node_type,
                          (bool) nodes[right_node_index].is_self_feature,
                          nodes[right_node_index].best_party_id,
                          nodes[right_node_index].best_feature_id,
                          nodes[right_node_index].best_split_id,
                          right_mark,
                          right_node_index);
      traverse_prediction_objs.push(left);
      traverse_prediction_objs.push(right);
    }
  }
  return binary_vector;
}

void TreeModel::compute_label_vec_and_index_map(EncodedNumber *label_vector,
    std::map<int, int> &node_index_2_leaf_index_map) {
  int leaf_cur_index = 0;
  for (int i = 0; i < pow(2, max_depth + 1) - 1; i++) {
    // if the node is leaf node, record its index.
    if (nodes[i].node_type == falcon::LEAF) {
      node_index_2_leaf_index_map.insert(std::make_pair(i, leaf_cur_index));
      // record leaf label vector
      label_vector[leaf_cur_index] = nodes[i].label;
      leaf_cur_index ++;
    }
  }
}

void TreeModel::predict(Party &party,
    std::vector< std::vector<double> > predicted_samples,
    int predicted_sample_size,
    EncodedNumber *predicted_labels) {

  /// prediction procedure:
  /// 1. organize the leaf label vector, and record the map
  ///     between tree node index and leaf index
  /// 2. for each sample in the predicted samples, search the whole tree
  ///     and do the following:
  ///     2.1 if meet feature that not belong to self, mark 1,
  ///         and iteratively search left and right branches with 1
  ///     2.2 if meet feature that belongs to self, compare with the split value,
  ///         mark satisfied branch with 1 while the other branch with 0,
  ///         and iteratively search left and right branches
  ///     2.3 if meet the leaf node, record the corresponding leaf index
  ///         with current value
  ///     2.4 after each party obtaining a 0-1 vector of leaf nodes, do the following:
  ///     2.5 the "party_num-1"-th party element-wise multiply with leaf
  ///         label vector, and encrypt the vector, send to the next party
  ///     2.6 every party on the Robin cycle updates the vector
  ///         by element-wise homomorphic multiplication, and send to the next
  ///     2.7 the last party, i.e., client 0 get the final encrypted vector
  ///         and homomorphic add together, call share decryption

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // step 1: organize the leaf label vector, compute the map
  LOG(INFO) << "Tree internal node num = " << internal_node_num;
  auto* label_vector = new EncodedNumber[internal_node_num + 1];
  std::map<int, int> node_index_2_leaf_index_map;
  compute_label_vec_and_index_map(label_vector, node_index_2_leaf_index_map);
  LOG(INFO) << "Compute label vector and index map finished";

  // step 2: compute prediction for each sample
  // for each sample
  for (int i = 0; i < predicted_sample_size; i++) {
    // compute binary vector for the current sample
    // each element in binary_vector is 1 or 0
    // 1: current leaf node still needs to be checked
    // 0: current leaf node doesn't need to be checked
    std::vector<int> binary_vector = comp_predict_vector(predicted_samples[i],
                                                         node_index_2_leaf_index_map);
    auto *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
    auto *updated_label_vector = new EncodedNumber[binary_vector.size()];

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
        djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j],
            updated_label_vector[j], encoded_binary_vector[j]);
      }
    }

    // after computing, only one element in updated_label_vector is 1, other is 0,
    // 1 corresponding to real label.

    // aggregate
    if (party.party_type == falcon::ACTIVE_PARTY) {
      predicted_labels[i].set_double(phe_pub_key->n[0], 0, PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
          predicted_labels[i], predicted_labels[i]);
      for (int j = 0; j < binary_vector.size(); j++) {
        djcs_t_aux_ee_add(phe_pub_key, predicted_labels[i],
            predicted_labels[i], updated_label_vector[j]);
      }
    }
    delete [] encoded_binary_vector;
    delete [] updated_label_vector;
  }

  // broadcast the predicted labels and call share decrypt
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::string s;
    serialize_encoded_number_array(predicted_labels, predicted_sample_size, s);
    for (int x = 0; x < party.party_num; x++) {
      if (x != party.party_id) {
        party.send_long_message(x, s);
      }
    }
  } else {
    std::string recv_s;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_s);
    deserialize_encoded_number_array(predicted_labels, predicted_sample_size, recv_s);
  }

  delete [] label_vector;
  djcs_t_free_public_key(phe_pub_key);
  LOG(INFO) << "Compute predictions on samples finished";
}