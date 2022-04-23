//
// Created by wuyuncheng on 31/7/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_model.h>
#include <falcon/algorithm/vertical/tree/gbdt_loss.h>
#include <falcon/party/info_exchange.h>

#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>

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
  if ((tree_type == falcon::REGRESSION) || (tree_type == falcon::CLASSIFICATION && class_num == 2)) {
    predict_single_estimator(party,predicted_samples,
                             predicted_sample_size, predicted_labels);
  } else {
    predict_multi_estimator(party,predicted_samples,
                            predicted_sample_size, predicted_labels);
  }
}

void GbdtModel::predict_single_estimator(Party &party,
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
  log_info("[GbdtModel.predict_single_estimator] predicted_sample_size = " + std::to_string(predicted_sample_size));
  // if active party, compute the encrypted dummy predictor and broadcast
  // init the encrypted predicted_labels with dummy prediction by 2 * precision
  EncodedNumber* raw_predictions = new EncodedNumber[predicted_sample_size];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    LOG(INFO) << "dummy predictor = " << dummy_predictors[0];
    for (int i = 0; i < predicted_sample_size; i++) {
      raw_predictions[i].set_double(phe_pub_key->n[0], dummy_predictors[0],
                                     2 * PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         raw_predictions[i], raw_predictions[i]);
    }
  }
  broadcast_encoded_number_array(party, raw_predictions, predicted_sample_size, ACTIVE_PARTY_ID);
  // iterate for each tree in gbdt_trees and predict
  for (int t = 0; t < tree_size; t++) {
    // predict for tree t, obtain the predicted labels, should be precision
    EncodedNumber *predicted_labels_tree_t = new EncodedNumber[predicted_sample_size];
    gbdt_trees[t].predict(party, predicted_samples,
                          predicted_sample_size, predicted_labels_tree_t);
    // add the current predicted labels
    EncodedNumber encoded_learning_rate;
    encoded_learning_rate.set_double(phe_pub_key->n[0], learning_rate);
    LOG(INFO) << "learning_rate = " << learning_rate;
    for (int i = 0; i < predicted_sample_size; i++) {
      djcs_t_aux_ep_mul(phe_pub_key, predicted_labels_tree_t[i],
                        predicted_labels_tree_t[i], encoded_learning_rate);
      djcs_t_aux_ee_add(phe_pub_key, raw_predictions[i],
                        raw_predictions[i], predicted_labels_tree_t[i]);
    }
    delete [] predicted_labels_tree_t;
  }
  if (tree_type == falcon::REGRESSION) {
    for (int i = 0; i < predicted_sample_size; i++) {
      predicted_labels[i] = raw_predictions[i];
    }
  } else {
    // call expit function to compute probabilities based on the raw_predictions
    int binary_classification_class_num = 1;
    compute_raw_predictions_expit(party, raw_predictions,
                                  predicted_labels, predicted_sample_size,
                                  binary_classification_class_num,
                                  2 * PHE_FIXED_POINT_PRECISION);
  }
  // free memory
  djcs_t_free_public_key(phe_pub_key);
  delete [] raw_predictions;
}

void GbdtModel::predict_multi_estimator(Party &party,
                                        std::vector<std::vector<double>> predicted_samples,
                                        int predicted_sample_size,
                                        EncodedNumber *predicted_labels) {
  /// the predict method works as follows:
  /// 1. init the dummy predictor for each estimator and add to predicted_labels
  /// 2. for each tree of each estimator in gbdt_trees, predict, multiply learning
  ///    rate and add to predicted_labels
  /// 3. compute the softmax of the predictions and return the predicted labels

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // if active party, compute the encrypted dummy predictors and broadcast
  // init the encrypted predicted_labels with dummy prediction by 2 * precision
  EncodedNumber* raw_predictions = new EncodedNumber[predicted_sample_size * class_num];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int c = 0; c < class_num; c++) {
      LOG(INFO) << "dummy predictor " << c << " = " <<  dummy_predictors[c];
      for (int i = 0; i < predicted_sample_size; i++) {
        int real_id = c * predicted_sample_size + i;
        raw_predictions[real_id].set_double(phe_pub_key->n[0], dummy_predictors[0],
                                      2 * PHE_FIXED_POINT_PRECISION);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           raw_predictions[real_id], raw_predictions[real_id]);
      }
    }
  }
  broadcast_encoded_number_array(party, raw_predictions,
                                 predicted_sample_size * class_num,
                                 ACTIVE_PARTY_ID);
  // iterate for each tree in gbdt_trees and predict
  for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
    for (int c = 0; c < class_num; c++) {
      // this is the tree id in the gbdt_model.trees
      int read_tree_id = tree_id * class_num + c;
      // predict for tree t, obtain the predicted labels, should be precision
      EncodedNumber *predicted_labels_tree_t = new EncodedNumber[predicted_sample_size];
      gbdt_trees[read_tree_id].predict(party, predicted_samples,
                            predicted_sample_size, predicted_labels_tree_t);
      // add the current predicted labels
      EncodedNumber encoded_learning_rate;
      encoded_learning_rate.set_double(phe_pub_key->n[0], learning_rate);
      LOG(INFO) << "learning_rate = " << learning_rate;
      for (int i = 0; i < predicted_sample_size; i++) {
        djcs_t_aux_ep_mul(phe_pub_key, predicted_labels_tree_t[i],
                          predicted_labels_tree_t[i], encoded_learning_rate);
        int real_id = c * predicted_sample_size + i;
        djcs_t_aux_ee_add(phe_pub_key, raw_predictions[real_id],
                          raw_predictions[real_id], predicted_labels_tree_t[i]);
      }
      delete [] predicted_labels_tree_t;
    }
  }
  // call expit function to compute probabilities based on the raw_predictions
  compute_raw_predictions_softmax(party, raw_predictions,
                                  predicted_labels, predicted_sample_size,
                                  class_num,
                                  2 * PHE_FIXED_POINT_PRECISION);
  // free memory
  djcs_t_free_public_key(phe_pub_key);
  delete [] raw_predictions;
}