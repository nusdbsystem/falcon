//
// Created by wuyuncheng on 3/8/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_loss.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>

#include <numeric>

/////////////////////////////////////////
///    LossFunction methods /////////////
/////////////////////////////////////////
LossFunction::LossFunction() {
  is_multi_class = false;
  num_trees_per_estimator = 1;
}

LossFunction::LossFunction(falcon::TreeType m_tree_type, int m_class_num) {
  if ((m_tree_type == falcon::REGRESSION) ||
      (m_tree_type == falcon::CLASSIFICATION && m_class_num == 2)) {
    is_multi_class = false;
    num_trees_per_estimator = 1;
  } else {
    is_multi_class = true;
    num_trees_per_estimator = m_class_num;
  }
}


/////////////////////////////////////////
/// RegressionLossFunction methods //////
/////////////////////////////////////////
RegressionLossFunction::RegressionLossFunction(falcon::TreeType m_tree_type,
                                               int m_class_num) :
                                               LossFunction(m_tree_type, m_class_num) {
  if (m_tree_type != falcon::REGRESSION) {
    LOG(ERROR) << "gbdt task type must be regression to use this loss function";
    exit(1);
  }
}


/////////////////////////////////////////
/// ClassificationLossFunction methods///
/////////////////////////////////////////
ClassificationLossFunction::ClassificationLossFunction(falcon::TreeType m_tree_type,
                                                       int m_class_num) :
                                                       LossFunction(m_tree_type, m_class_num) {
  if (m_tree_type != falcon::CLASSIFICATION) {
    LOG(ERROR) << "gbdt task type must be classification to use this loss function";
    exit(1);
  }
}


/////////////////////////////////////////
/// LeastSquareError methods ////////////
/////////////////////////////////////////
LeastSquareError::LeastSquareError(falcon::TreeType m_tree_type,
                                   int m_class_num) :
                                   RegressionLossFunction(m_tree_type, m_class_num) {
  if (m_class_num != 1) {
    LOG(ERROR) << "regression class num should be 1.";
    exit(1);
  }
}

void LeastSquareError::negative_gradient(Party party,
                                         EncodedNumber *ground_truth_labels,
                                         EncodedNumber *raw_predictions,
                                         EncodedNumber *residuals,
                                         int size) {
  // for regression tasks, the negative_gradient is (y - raw_predictions)
  // here, both the ground_truth_labels and raw_predictions are with size
  // sample_num, as well as the resulted residual vector

  // the active party compute the encrypted residual and broadcast
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // retrieve phe pub key
    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
    party.getter_phe_pub_key(phe_pub_key);
    EncodedNumber negative_one;
    negative_one.set_integer(phe_pub_key->n[0], -1);
    for (int i = 0; i < size; i++) {
      EncodedNumber tmp;
      // note that the precision of tmp equals to that of raw_prediction,
      // i.e., 2 * PHE_FIXED_POINT_PRECISION
      djcs_t_aux_ep_mul(phe_pub_key, tmp, raw_predictions[i], negative_one);
      djcs_t_aux_ee_add(phe_pub_key, residuals[i], ground_truth_labels[i], tmp);
    }
    // free retrieved public key
    djcs_t_free_public_key(phe_pub_key);
  }
  // active party broadcasts the residuals
  party.broadcast_encoded_number_array(residuals, size, ACTIVE_PARTY_ID);
}

void LeastSquareError::update_terminal_regions(Party party,
                                               TreeModel tree_model,
                                               std::vector<std::vector<double>> data,
                                               EncodedNumber *residuals,
                                               EncodedNumber *raw_predictions,
                                               int size,
                                               double learning_rate,
                                               int estimator_index) {
  LOG(INFO) << "begin to update terminal regions";
  google::FlushLogFiles(google::INFO);
  // for least square error loss function, there is no need to update the
  // terminal regions, but need to update the raw_predictions
  // i.e., raw_predictions += learning_rate * tree.predict()
  EncodedNumber *cur_predictions = new EncodedNumber[size];
  tree_model.predict(party, data, size, cur_predictions);
  // multiply by learning_rate, here the precision of tree_predicted_labels
  // should be PHE_FIXED_POINT_PRECISION, otherwise, the rest of computations
  // would be problematic, thus, add a check here
  if (cur_predictions[0].getter_exponent() != 0 - PHE_FIXED_POINT_PRECISION) {
    LOG(ERROR) << "The precision of tree_predicted_labels is not expected!";
    exit(1);
  }

  LOG(INFO) << "finish current tree model prediction";
  google::FlushLogFiles(google::INFO);

  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  EncodedNumber *assists = new EncodedNumber[size];
  // if active party, update the raw_predictions
  if (party.party_type == falcon::ACTIVE_PARTY) {
    EncodedNumber encoded_learning_rate;
    encoded_learning_rate.set_double(phe_pub_key->n[0], learning_rate);
    for (int i = 0; i < size; i++) {
      // note that the precision here would be 2 * PHE_FIXED_POINT_PRECISION
      djcs_t_aux_ep_mul(phe_pub_key, assists[i],
                        cur_predictions[i], encoded_learning_rate);
    }
  }
  LOG(INFO) << "begin truncate the ciphers precision";
  google::FlushLogFiles(google::INFO);
  // broadcast the assists and truncate the precision to PHE
  party.broadcast_encoded_number_array(assists, size, ACTIVE_PARTY_ID);
  party.truncate_ciphers_precision(assists, size, ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  for (int i = 0; i < size; i++) {
    djcs_t_aux_ee_add(phe_pub_key, raw_predictions[i],
                      raw_predictions[i], assists[i]);
  }
  LOG(INFO) << "finish truncate the cipher precision";
  google::FlushLogFiles(google::INFO);

  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
  delete [] cur_predictions;
  delete [] assists;
}

void LeastSquareError::get_init_raw_predictions(Party party,
                                                EncodedNumber *raw_predictions,
                                                int size,
                                                std::vector<std::vector<double>> data,
                                                std::vector<double> labels) {
  // only the active party has the label, thus it computes the raw_predictions
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // for least square error, use the mean label as the dummy prediction
    // and the raw_predictions are exactly the dummy prediction
    double label_sum = std::accumulate(labels.begin(), labels.end(),
                                       decltype(labels)::value_type(0));
    dummy_prediction = label_sum / (double) labels.size();
    // retrieve phe pub key
    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
    party.getter_phe_pub_key(phe_pub_key);
    for (int i = 0; i < size; i++) {
      raw_predictions[i].set_double(phe_pub_key->n[0], dummy_prediction);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         raw_predictions[i], raw_predictions[i]);
    }
    // free retrieved public key
    djcs_t_free_public_key(phe_pub_key);
  }
  // active party broadcasts the raw_predictions to other parties
  party.broadcast_encoded_number_array(raw_predictions, size, ACTIVE_PARTY_ID);
}


/////////////////////////////////////////
/// BinomialDeviance methods ////////////
/////////////////////////////////////////
BinomialDeviance::BinomialDeviance(falcon::TreeType m_tree_type,
                                   int m_class_num) :
                                   ClassificationLossFunction(m_tree_type, m_class_num) {
  if (m_class_num != 2) {
    LOG(ERROR) << "binary classification class num should be 2.";
    exit(1);
  }
}

void BinomialDeviance::negative_gradient(Party party,
                                         EncodedNumber *ground_truth_labels,
                                         EncodedNumber *raw_predictions,
                                         EncodedNumber *residuals,
                                         int size) {

}

void BinomialDeviance::update_terminal_regions(Party party,
                                               TreeModel tree_model,
                                               std::vector<std::vector<double>> data,
                                               EncodedNumber *residuals,
                                               EncodedNumber *raw_predictions,
                                               int size,
                                               double learning_rate,
                                               int estimator_index) {

}

void BinomialDeviance::get_init_raw_predictions(Party party,
                                                EncodedNumber *raw_predictions,
                                                int size,
                                                std::vector<std::vector<double>> data,
                                                std::vector<double> labels) {

}

void BinomialDeviance::raw_predictions_to_probas(Party party,
                                                 EncodedNumber *raw_predictions,
                                                 EncodedNumber *probas,
                                                 int size) {

}

void BinomialDeviance::raw_predictions_to_decision(Party party,
                                                   EncodedNumber *raw_predictions,
                                                   EncodedNumber *decisions,
                                                   int size) {

}


/////////////////////////////////////////
/// MultinomialDeviance methods /////////
/////////////////////////////////////////
MultinomialDeviance::MultinomialDeviance(falcon::TreeType m_tree_type,
                                         int m_class_num) :
                                         ClassificationLossFunction(m_tree_type,
                                                                    m_class_num) {
  if (m_class_num < 2) {
    LOG(ERROR) << "multi-class classification class num should > 2.";
    exit(1);
  }
}

void MultinomialDeviance::negative_gradient(Party party,
                                            EncodedNumber *ground_truth_labels,
                                            EncodedNumber *raw_predictions,
                                            EncodedNumber *residuals,
                                            int size) {

}

void MultinomialDeviance::update_terminal_regions(Party party,
                                                  TreeModel tree_model,
                                                  std::vector<std::vector<double>> data,
                                                  EncodedNumber *residuals,
                                                  EncodedNumber *raw_predictions,
                                                  int size,
                                                  double learning_rate,
                                                  int estimator_index) {

}

void MultinomialDeviance::get_init_raw_predictions(Party party,
                                                   EncodedNumber *raw_predictions,
                                                   int size,
                                                   std::vector<std::vector<double>> data,
                                                   std::vector<double> labels) {

}

void MultinomialDeviance::raw_predictions_to_probas(Party party,
                                                    EncodedNumber *raw_predictions,
                                                    EncodedNumber *probas,
                                                    int size) {

}

void MultinomialDeviance::raw_predictions_to_decision(Party party,
                                                      EncodedNumber *raw_predictions,
                                                      EncodedNumber *decisions,
                                                      int size) {

}

