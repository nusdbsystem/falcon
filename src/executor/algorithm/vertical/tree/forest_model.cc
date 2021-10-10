//
// Created by wuyuncheng on 9/7/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/algorithm/vertical/tree/forest_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>

#include <cmath>
#include <glog/logging.h>
#include <iostream>

ForestModel::ForestModel() {}

ForestModel::ForestModel(int m_tree_size, std::string m_tree_type) {
  tree_size = m_tree_size;
  // copy builder parameters
  if (m_tree_type == "classification") {
    tree_type = falcon::CLASSIFICATION;
  } else {
    tree_type = falcon::REGRESSION;
  }
  forest_trees.reserve(tree_size);
}

ForestModel::~ForestModel() {}

ForestModel::ForestModel(const ForestModel &forest_model) {
  tree_size = forest_model.tree_size;
  tree_type = forest_model.tree_type;
  forest_trees = forest_model.forest_trees;
}

ForestModel& ForestModel::operator=(const ForestModel &forest_model) {
  tree_size = forest_model.tree_size;
  tree_type = forest_model.tree_type;
  forest_trees = forest_model.forest_trees;
}

void ForestModel::predict(Party &party,
                          const std::vector<std::vector<double> >& predicted_samples,
                          int predicted_sample_size,
                          EncodedNumber *predicted_labels) {
  /// the prediction workflow is as follows:
  ///     step 1: the parties compute a prediction for each tree for each sample
  ///     step 2: the parties convert the predictions to secret shares and send to mpc to compute mode
  ///     step 3: the parties convert the secret shared results back to ciphertext

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // init predicted forest labels to record the predictions, the first dimension
  // is the number of trees in the forest, and the second dimension is the number
  // of the samples in the predicted dataset
  auto** predicted_forest_labels = new EncodedNumber*[tree_size];
  for (int tree_id = 0; tree_id < tree_size; tree_id++) {
    predicted_forest_labels[tree_id] = new EncodedNumber[predicted_sample_size];
  }

  // compute predictions
  for (int tree_id = 0; tree_id < tree_size; tree_id++) {
    // compute predictions for each tree in the forest
    forest_trees[tree_id].predict(party,
        predicted_samples,
        predicted_sample_size,
        predicted_forest_labels[tree_id]);
  }
  int cipher_precision = abs(predicted_forest_labels[0][0].getter_exponent());
  LOG(INFO) << "cipher_precision = " << cipher_precision;
  std::cout << "cipher_precision = " << cipher_precision << std::endl;

  // if classification, needs to communicate with mpc
  // otherwise, compute average of the prediction
  if (tree_type == falcon::REGRESSION) {
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int i = 0; i < predicted_sample_size; i++) {
        auto *aggregation = new EncodedNumber[1];
        aggregation[0].set_double(phe_pub_key->n[0], 0.0, cipher_precision);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           aggregation[0], aggregation[0]);
        // aggregate the predicted labels for sample i
        for (int tree_id = 0; tree_id < tree_size; tree_id++) {
          djcs_t_aux_ee_add(phe_pub_key, aggregation[0],
                            aggregation[0], predicted_forest_labels[tree_id][i]);
        }
        // compute average of the aggregation
        EncodedNumber enc_tree_size;
        enc_tree_size.set_double(phe_pub_key->n[0], (1.0 / tree_size), cipher_precision);
        djcs_t_aux_ep_mul(phe_pub_key, predicted_labels[i], aggregation[0], enc_tree_size);
        delete[] aggregation;
      }
    }
    party.broadcast_encoded_number_array(predicted_labels, predicted_sample_size, ACTIVE_PARTY_ID);
  } else {
    std::vector<int> public_values;
    std::vector<double> private_values;
    public_values.push_back(tree_size);
    public_values.push_back(predicted_sample_size);

    // convert the predicted_forest_labels into secret shares
    // the structure is one-dimensional vector, [tree_1 * sample_size] ... [tree_n * sample_size]
    std::vector< std::vector<double> > forest_label_secret_shares;
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      std::vector<double> tree_label_secret_shares;
      party.ciphers_to_secret_shares(predicted_forest_labels[tree_id],
                                     tree_label_secret_shares,
                                     predicted_sample_size,
                                     ACTIVE_PARTY_ID,
                                     cipher_precision);
      forest_label_secret_shares.push_back(tree_label_secret_shares);
    }
    // pack to private values for sending to mpc parties
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      for (int i = 0; i < predicted_sample_size; i++) {
        private_values.push_back(forest_label_secret_shares[tree_id][i]);
      }
    }

    falcon::SpdzTreeCompType comp_type = falcon::RF_LABEL_MODE;

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
    std::vector<double> predicted_sample_shares = future_values.get();
    spdz_pruning_check_thread.join();

    // convert the secret shares to ciphertext, which is the predicted labels
    party.secret_shares_to_ciphers(predicted_labels,
                                   predicted_sample_shares,
                                   predicted_sample_size,
                                   ACTIVE_PARTY_ID,
                                   cipher_precision);
  }

  // free memory
  djcs_t_free_public_key(phe_pub_key);
  for (int tree_id = 0; tree_id < tree_size; tree_id++) {
    delete [] predicted_forest_labels[tree_id];
  }
  delete [] predicted_forest_labels;
}