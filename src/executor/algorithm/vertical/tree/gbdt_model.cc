//
// Created by wuyuncheng on 31/7/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_model.h>

#include <glog/logging.h>

GbdtModel::GbdtModel(int m_tree_size, std::string m_tree_type,
    int m_n_estimator, int m_class_num, double m_learning_rate) {
  tree_size = m_tree_size;
  // copy builder parameters
  if (m_tree_type == "classification") {
    tree_type = falcon::CLASSIFICATION;
  } else {
    tree_type = falcon::REGRESSION;
  }
  n_estimator = m_n_estimator;
  class_num = m_class_num;
  learning_rate = m_learning_rate;
  gbdt_trees.reserve(tree_size);
  if (tree_type == falcon::CLASSIFICATION && class_num > 2) {
    dummy_predictors.reserve(class_num);
  } else {
    dummy_predictors.reserve(1);
  }
}

GbdtModel::GbdtModel(const GbdtModel &gbdt_model) {
  tree_size = gbdt_model.tree_size;
  tree_type = gbdt_model.tree_type;
  n_estimator = gbdt_model.n_estimator;
  class_num = gbdt_model.class_num;
  learning_rate = gbdt_model.learning_rate;
  dummy_predictors = gbdt_model.dummy_predictors;
  gbdt_trees = gbdt_model.gbdt_trees;
}

GbdtModel& GbdtModel::operator=(const GbdtModel &gbdt_model) {
  tree_size = gbdt_model.tree_size;
  tree_type = gbdt_model.tree_type;
  n_estimator = gbdt_model.n_estimator;
  class_num = gbdt_model.class_num;
  learning_rate = gbdt_model.learning_rate;
  dummy_predictors = gbdt_model.dummy_predictors;
  gbdt_trees = gbdt_model.gbdt_trees;
}

void GbdtModel::predict(Party &party,
                        std::vector<std::vector<double>> predicted_samples,
                        int predicted_sample_size,
                        EncodedNumber *predicted_labels) {
  // the prediction method for regression and classification are different
  if (tree_type == falcon::REGRESSION) {
    predict_regression(party,predicted_samples,
                       predicted_sample_size, predicted_labels);
  } else {
    predict_classification(party,predicted_samples,
                           predicted_sample_size, predicted_labels);
  }
}

void GbdtModel::predict_regression(Party &party,
                                   std::vector<std::vector<double>> predicted_samples,
                                   int predicted_sample_size,
                                   EncodedNumber *predicted_labels) {
  /// the predict method works as follows:
  /// 1. init the dummy predictor and add to predicted_labels
  /// 2. for each tree in gbdt_trees, predict, multiply learning
  ///    rate and add to predicted_labels
  /// 3. return the predicted labels

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // if active party, compute the encrypted dummy predictor and broadcast
  // init the encrypted predicted_labels with dummy prediction by 2 * precision
  if (party.party_type == falcon::ACTIVE_PARTY) {
    LOG(INFO) << "dummy predictor = " << dummy_predictors[0];
    for (int i = 0; i < predicted_sample_size; i++) {
      predicted_labels[i].set_double(phe_pub_key->n[0], dummy_predictors[0],
                                     2 * PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         predicted_labels[i], predicted_labels[i]);
    }
  }
  party.broadcast_encoded_number_array(predicted_labels, predicted_sample_size, ACTIVE_PARTY_ID);
  // iterate for each tree in gbdt_trees and predict
  for (int t = 0; t < tree_size; t++) {
    // predict for tree t, obtain the predicted labels, should be precision
    EncodedNumber *predicted_labels_tree_t = new EncodedNumber[predicted_sample_size];
    gbdt_trees[t].predict(party, predicted_samples,
                          predicted_sample_size, predicted_labels_tree_t);
    // add the current predicted labels
    EncodedNumber encoded_learning_rate;
    encoded_learning_rate.set_double(phe_pub_key->n[0], learning_rate);
    for (int i = 0; i < predicted_sample_size; i++) {
      djcs_t_aux_ep_mul(phe_pub_key, predicted_labels_tree_t[i],
                        predicted_labels_tree_t[i], encoded_learning_rate);
      djcs_t_aux_ee_add(phe_pub_key, predicted_labels[i],
                        predicted_labels[i], predicted_labels_tree_t[i]);
    }
    delete [] predicted_labels_tree_t;
  }
  // free memory
  djcs_t_free_public_key(phe_pub_key);
}

void GbdtModel::predict_classification(Party &party,
                                       std::vector<std::vector<double>> predicted_samples,
                                       int predicted_sample_size,
                                       EncodedNumber *predicted_labels) {
  // to be added
}