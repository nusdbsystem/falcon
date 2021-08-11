//
// Created by wuyuncheng on 3/8/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_loss.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>

#include <numeric>
#include <cmath>

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
                                               DecisionTreeBuilder &decision_tree_builder,
                                               EncodedNumber *ground_truth_labels,
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
  // call the update function, inplace update
  update_raw_predictions_with_learning_rate(party,
                                            decision_tree_builder,
                                            raw_predictions,
                                            size,
                                            learning_rate);
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
  // for binomial deviance, the negative gradient is y - expit(raw_predictions)
  // where expit = 1/(1 + exp(-x)), i.e., the logistic function, which needs
  // the help of mpc for the computation
  int binary_classification_class_num = 1;
  EncodedNumber* expit_raw_predictions = new EncodedNumber[size];
  compute_raw_predictions_expit(party, raw_predictions,
                                expit_raw_predictions, size,
                                binary_classification_class_num,
                                PHE_FIXED_POINT_PRECISION);
  // compute the negative gradient, i.e., residuals
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  EncodedNumber negative_one;
  negative_one.set_integer(phe_pub_key->n[0], -1);
  for (int i = 0; i < size; i++) {
    EncodedNumber tmp;
    djcs_t_aux_ep_mul(phe_pub_key, tmp, expit_raw_predictions[i], negative_one);
    djcs_t_aux_ee_add(phe_pub_key, residuals[i], ground_truth_labels[i], tmp);
  }

  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
  delete [] expit_raw_predictions;
}

void BinomialDeviance::update_terminal_regions(Party party,
                                               DecisionTreeBuilder &decision_tree_builder,
                                               EncodedNumber *ground_truth_labels,
                                               EncodedNumber *residuals,
                                               EncodedNumber *raw_predictions,
                                               int size,
                                               double learning_rate,
                                               int estimator_index) {
  // for binary classification, need to update the leaves of the current
  // tree model by sum(y - prob) / sum (prob * (1 - prob)), where
  // prob = expit(raw_prediction), and (y - prob) = residual, thus, the
  // equation is: sum(residual) / sum((y - residual) / (1 - y + residual))
  // in our privacy-preserving decision tree training algorithm, the samples
  // on the leaf nodes are unknown, thus, we need a trick here: convert
  // the predictions to secret shares and check if it matches a leaf label
  int binary_classification_num = 1;
  update_terminal_regions_for_classification(party,
                                             decision_tree_builder,
                                             ground_truth_labels,
                                             residuals,
                                             raw_predictions,
                                             size,
                                             learning_rate,
                                             binary_classification_num);
}

void BinomialDeviance::get_init_raw_predictions(Party party,
                                                EncodedNumber *raw_predictions,
                                                int size,
                                                std::vector<std::vector<double>> data,
                                                std::vector<double> labels) {
  // only the active party has the label, thus it computes the raw_predictions
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // for binomial deviance, use the probability of #event / (#event + #non-event)
    // as the dummy prediction, and the raw prediction is log(odds) = log(p/1-p)
    int num_event = 0;
    for (double l : labels) {
      if ((int) l == 1) {
        num_event++;
      }
    }
    // compute the probability of class 1
    double p = ((double) num_event) / ((double) labels.size());
    // clip to interval [eps, 1-eps]
    if (p < ROUNDED_PRECISION) {
      p = ROUNDED_PRECISION;
    }
    if (p > 1 - ROUNDED_PRECISION) {
      p = 1 - ROUNDED_PRECISION;
    }
    // raw prediction = log(odds) = log(p/1-p)
    double odds = p / (1 - p);
    dummy_prediction = log(odds);
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

void BinomialDeviance::raw_predictions_to_probas(Party party,
                                                 EncodedNumber *raw_predictions,
                                                 EncodedNumber *probas,
                                                 int size,
                                                 int phe_precision) {
  // given the raw_predictions, need to compute expit() to get the
  // probabilities for the positive class
  int binary_classification_class_num = 1;
  compute_raw_predictions_expit(party, raw_predictions, probas,
                                size, binary_classification_class_num,
                                phe_precision);
}

void BinomialDeviance::raw_predictions_to_decision(Party party,
                                                   EncodedNumber *raw_predictions,
                                                   EncodedNumber *decisions,
                                                   int size,
                                                   int phe_precision) {
  // leave this function here for future usage, as the current implementation
  // only return the probability before decryption
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
  // for multinomial deviance, the negative gradient is y - softmax(raw_predictions)
  // which needs the help of mpc for the computation
  EncodedNumber* softmax_raw_predictions = new EncodedNumber[size];
  int sample_size = size / num_trees_per_estimator;
  compute_raw_predictions_softmax(party, raw_predictions,
                                  softmax_raw_predictions, sample_size,
                                  num_trees_per_estimator,
                                  PHE_FIXED_POINT_PRECISION);
  // compute the negative gradient, i.e., residuals
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  EncodedNumber negative_one;
  negative_one.set_integer(phe_pub_key->n[0], -1);
  for (int i = 0; i < size; i++) {
    EncodedNumber tmp;
    djcs_t_aux_ep_mul(phe_pub_key, tmp, softmax_raw_predictions[i], negative_one);
    djcs_t_aux_ee_add(phe_pub_key, residuals[i], ground_truth_labels[i], tmp);
  }

  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
  delete [] softmax_raw_predictions;
}

void MultinomialDeviance::update_terminal_regions(Party party,
                                                  DecisionTreeBuilder &decision_tree_builder,
                                                  EncodedNumber *ground_truth_labels,
                                                  EncodedNumber *residuals,
                                                  EncodedNumber *raw_predictions,
                                                  int size,
                                                  double learning_rate,
                                                  int estimator_index) {
  // for multinomial classification, need to update the leaves of the current
  // tree model by (((k - 1) / k) * sum(y - prob)) / sum (prob * (1 - prob)), where
  // prob = expit(raw_prediction), and (y - prob) = residual, and k is the class num,
  // thus, the equation is: sum(residual) * ((k-1)/k) / sum((y - residual) / (1 - y + residual))
  // in our privacy-preserving decision tree training algorithm, the samples
  // on the leaf nodes are unknown, thus, we need a trick here: convert
  // the predictions to secret shares and check if it matches a leaf label
  update_terminal_regions_for_classification(party,
                                             decision_tree_builder,
                                             ground_truth_labels,
                                             residuals,
                                             raw_predictions,
                                             size,
                                             learning_rate,
                                             num_trees_per_estimator);
}

void MultinomialDeviance::get_init_raw_predictions(Party party,
                                                   EncodedNumber *raw_predictions,
                                                   int size,
                                                   std::vector<std::vector<double>> data,
                                                   std::vector<double> labels) {
  // only the active party has the label, thus it computes the raw_predictions
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // for multinomial deviance, use the probability of #event / (#event + #non-event)
    // as the dummy prediction for each class, and the raw prediction is log(odds) = log(p/1-p)
    int sample_size = size / num_trees_per_estimator;
    LOG(INFO) << "sample size = " << sample_size;
    // retrieve phe pub key
    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
    party.getter_phe_pub_key(phe_pub_key);
    for (int c = 0; c < num_trees_per_estimator; c++) {
      int num_event = 0;
      for (double l : labels) {
        if ((int) l == c) {
          num_event++;
        }
      }
      // compute the probability of class 1
      double p = ((double) num_event) / ((double) labels.size());
      // clip to interval [eps, 1-eps]
      if (p < ROUNDED_PRECISION) {
        p = ROUNDED_PRECISION;
      }
      if (p > 1 - ROUNDED_PRECISION) {
        p = 1 - ROUNDED_PRECISION;
      }
      // raw prediction = log(odds) = log(p/1-p)
      double odds = p / (1 - p);
      double cur_dummy_prediction = log(odds);
      dummy_predictions.push_back(cur_dummy_prediction);
      for (int i = 0; i < sample_size; i++) {
        int real_sample_id = c * sample_size + i;
        raw_predictions[real_sample_id].set_double(phe_pub_key->n[0], cur_dummy_prediction);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           raw_predictions[real_sample_id],
                           raw_predictions[real_sample_id]);
      }
    }
    // free retrieved public key
    djcs_t_free_public_key(phe_pub_key);
  }
  // active party broadcasts the raw_predictions to other parties
  party.broadcast_encoded_number_array(raw_predictions, size, ACTIVE_PARTY_ID);
}

void MultinomialDeviance::raw_predictions_to_probas(Party party,
                                                    EncodedNumber *raw_predictions,
                                                    EncodedNumber *probas,
                                                    int size,
                                                    int phe_precision) {
  // given the raw_predictions, need to compute softmatx() to get the
  // prediction probabilities for the classes
  int sample_size = size / num_trees_per_estimator;
  compute_raw_predictions_softmax(party, raw_predictions,
                                  probas, sample_size,
                                  num_trees_per_estimator,
                                  phe_precision);
}

void MultinomialDeviance::raw_predictions_to_decision(Party party,
                                                      EncodedNumber *raw_predictions,
                                                      EncodedNumber *decisions,
                                                      int size,
                                                      int phe_precision) {
  // leave this function here for future usage, as the current implementation
  // only return the probability before decryption
}

void update_raw_predictions_with_learning_rate(Party party,
                                               DecisionTreeBuilder &decision_tree_builder,
                                               EncodedNumber* raw_predictions,
                                               int size,
                                               double learning_rate) {
  // first predict based on the current tree builder
  EncodedNumber *cur_predictions = new EncodedNumber[size];
  decision_tree_builder.tree.predict(party,
                                     decision_tree_builder.getter_training_data(),
                                     size, cur_predictions);
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

void compute_raw_predictions_expit(Party party,
                                   EncodedNumber* raw_predictions,
                                   EncodedNumber* expit_raw_predictions,
                                   int size,
                                   int class_num,
                                   int phe_precision) {
  // step 1: convert the raw_predictions into secret shares
  std::vector<double> raw_predictions_shares;
  party.ciphers_to_secret_shares(raw_predictions, raw_predictions_shares,
                                 size, ACTIVE_PARTY_ID, phe_precision);
  // step 2: send to mpc for computing logistic function
  std::vector<int> public_values;
  public_values.push_back(size);
  public_values.push_back(class_num);
  falcon::SpdzTreeCompType comp_type = falcon::GBDT_EXPIT;
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
                                        raw_predictions_shares.size(),
                                        raw_predictions_shares,
                                        comp_type,
                                        &promise_values);
  std::vector<double> expit_raw_predictions_shares = future_values.get();
  spdz_pruning_check_thread.join();
  // step 3: convert the resulted shares back into ciphers,
  // which is encrypted expit raw predictions
  party.secret_shares_to_ciphers(expit_raw_predictions,
                                 expit_raw_predictions_shares,
                                 size,
                                 ACTIVE_PARTY_ID,
                                 phe_precision);
}

void compute_raw_predictions_softmax(Party party,
                                     EncodedNumber* raw_predictions,
                                     EncodedNumber* softmax_raw_predictions,
                                     int sample_size,
                                     int class_num,
                                     int phe_precision) {
  // step 1: convert the raw_predictions into secret shares, the size is sample_size * class_num
  int size = sample_size * class_num;
  std::vector<double> raw_predictions_shares;
  party.ciphers_to_secret_shares(raw_predictions, raw_predictions_shares,
                                 size, ACTIVE_PARTY_ID, phe_precision);
  // step 2: send to mpc for computing softmax
  std::vector<int> public_values;
  public_values.push_back(sample_size);
  public_values.push_back(class_num);
  falcon::SpdzTreeCompType comp_type = falcon::GBDT_SOFTMAX;
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
                                        raw_predictions_shares.size(),
                                        raw_predictions_shares,
                                        comp_type,
                                        &promise_values);
  std::vector<double> softmax_raw_predictions_shares = future_values.get();
  spdz_pruning_check_thread.join();
  // step 3: convert the resulted shares back into ciphers,
  // which is encrypted expit raw predictions
  party.secret_shares_to_ciphers(softmax_raw_predictions,
                                 softmax_raw_predictions_shares,
                                 size,
                                 ACTIVE_PARTY_ID,
                                 phe_precision);
}

void update_terminal_regions_for_classification(Party party,
                                                DecisionTreeBuilder &decision_tree_builder,
                                                EncodedNumber *ground_truth_labels,
                                                EncodedNumber* residuals,
                                                EncodedNumber* raw_predictions,
                                                int sample_size,
                                                double learning_rate,
                                                int class_num) {
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // step 1: convert the residual to secret shares
  std::vector<double> residuals_shares;
  party.ciphers_to_secret_shares(residuals, residuals_shares, sample_size,
                                 ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  LOG(INFO) << "Compute residual secret shares finished";
  // step 2: compute ground-truth label secret shares y
  std::vector<double> ground_truth_labels_shares;
  party.ciphers_to_secret_shares(ground_truth_labels, ground_truth_labels_shares, sample_size,
                                 ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  LOG(INFO) << "Compute ground truth labels secret shares finished";
  // step 3: predict based on current tree, for terminal region check using mpc
  EncodedNumber* pre_predictions = new EncodedNumber[sample_size];
  decision_tree_builder.tree.predict(party, decision_tree_builder.getter_training_data(),
                                     sample_size, pre_predictions);
  std::vector<double> pre_predictions_shares;
  party.ciphers_to_secret_shares(pre_predictions, pre_predictions_shares, sample_size,
                                 ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  LOG(INFO) << "Predict on the current tree model finished";
  // step 4: find the match between leaf node index and tree node index
  int leaf_node_num = decision_tree_builder.tree.internal_node_num + 1;
  LOG(INFO) << "leaf node num = " << leaf_node_num;
  EncodedNumber* leaf_labels = new EncodedNumber[leaf_node_num];
  std::map<int, int> node_index_2_leaf_index_map;
  decision_tree_builder.tree.compute_label_vec_and_index_map(leaf_labels,
                                                             node_index_2_leaf_index_map);
  LOG(INFO) << "Compute label vector and index map finished";
  // step 5: convert the encrypted labels to secret shares
  std::vector<double> leaf_lables_shares;
  party.ciphers_to_secret_shares(leaf_labels, leaf_lables_shares, leaf_node_num,
                                 ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  LOG(INFO) << "Compute leaf label shares finished";
  // step 6: call the GBDT_UPDATE_TERMINAL_REGION mpc program to compute
  // the new leaf node label
  std::vector<int> public_values;
  std::vector<double> private_values;
  std::vector<std::vector<double>> two_dim_private_values;
  public_values.push_back(sample_size);
  public_values.push_back(leaf_node_num);
  public_values.push_back(class_num);
  two_dim_private_values.push_back(residuals_shares);
  two_dim_private_values.push_back(ground_truth_labels_shares);
  two_dim_private_values.push_back(pre_predictions_shares);
  two_dim_private_values.push_back(leaf_lables_shares);
  // flatten the two_dim_private_values
  for (int i = 0; i < two_dim_private_values.size(); i++) {
    for (int j = 0; j < two_dim_private_values[i].size(); j++) {
      private_values.push_back(two_dim_private_values[i][j]);
    }
  }
  falcon::SpdzTreeCompType comp_type = falcon::GBDT_UPDATE_TERMINAL_REGION;
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
  std::vector<double> updated_leaf_label_shares = future_values.get();
  spdz_pruning_check_thread.join();
  LOG(INFO) << "Compute mpc update terminal region finished";
  // step 7: convert the returned new leaf label shares to ciphers
  EncodedNumber* updated_leaf_labels = new EncodedNumber[leaf_node_num];
  party.secret_shares_to_ciphers(updated_leaf_labels, updated_leaf_label_shares,
                                 leaf_node_num, ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  LOG(INFO) << "Convert new label shares to ciphers finished";
  // step 8: update the tree model
  for (int i = 0; i < leaf_node_num; i++) {
    // find the corresponding node index in the tree model and update
    int node_index;
    for (const auto& key: node_index_2_leaf_index_map) {
      if (key.second == i) {
        node_index = key.first;
      }
    }
    decision_tree_builder.tree.nodes[node_index].label = updated_leaf_labels[i];
  }
  LOG(INFO) << "Update the current tree model finished";
  // step 9: after update the terminal regions, execute another predict to get the
  // current tree prediction, update the raw_predictions by:
  // raw_predictions += current_prediction * learning_rate
  // call the update function, inplace update
  update_raw_predictions_with_learning_rate(party,
                                            decision_tree_builder,
                                            raw_predictions,
                                            sample_size,
                                            learning_rate);
  LOG(INFO) << "Update the raw predictions finished";
  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
  delete [] pre_predictions;
  delete [] leaf_labels;
  delete [] updated_leaf_labels;
}