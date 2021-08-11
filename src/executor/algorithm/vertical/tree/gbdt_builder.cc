//
// Created by wuyuncheng on 31/7/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_model.h>
#include <falcon/algorithm/vertical/tree/gbdt_loss.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/model/model_io.h>

#include <glog/logging.h>

#include <iomanip>
#include <random>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <map>
#include <stack>

#include <Networking/ssl_sockets.h>

GbdtBuilder::GbdtBuilder() {}

GbdtBuilder::~GbdtBuilder() {
  // automatically memory free
}

GbdtBuilder::GbdtBuilder(GbdtParams gbdt_params,
    std::vector<std::vector<double> > m_training_data,
    std::vector<std::vector<double> > m_testing_data,
    std::vector<double> m_training_labels,
    std::vector<double> m_testing_labels,
    double m_training_accuracy,
    double m_testing_accuracy) : ModelBuilder(std::move(m_training_data),
        std::move(m_testing_data),
        std::move(m_training_labels),
        std::move(m_testing_labels),
        m_training_accuracy,
        m_testing_accuracy){
  n_estimator = gbdt_params.n_estimator;
  loss = gbdt_params.loss;
  learning_rate = gbdt_params.learning_rate;
  subsample = gbdt_params.subsample;
  dt_param = gbdt_params.dt_param;
  int tree_size;
  // if regression or binary classification, tree size = n_estimator
  if (dt_param.tree_type == "classification" && dt_param.class_num > 2) {
    tree_size = n_estimator * dt_param.class_num;
  } else {
    tree_size = n_estimator;
  }
  gbdt_model = GbdtModel(tree_size, dt_param.tree_type,
      n_estimator, dt_param.class_num, learning_rate);
  tree_builders.reserve(tree_size);
  local_feature_num = training_data[0].size();
}

void GbdtBuilder::train(Party party) {
  // two branches for training gbdt model: regression and classification
  LOG(INFO) << "************ Begin to train the GBDT model ************";
  // required by spdz connector and mpc computation
  // bigint::init_thread();
  if (gbdt_model.tree_type == falcon::REGRESSION) {
    train_regression_task(party);
  } else {
    train_classification_task(party);
  }
  LOG(INFO) << "End train the GBDT model";
  google::FlushLogFiles(google::INFO);
}

void GbdtBuilder::train_regression_task(Party party) {
  /// For regression, build the trees as follows:
  ///   1. init a dummy estimator with mean, compute raw_predictions as mean
  ///   2. for tree id 0 to num-1, init a decision tree, compute the encrypted
  ///      residuals between the original labels and raw_predictions
  ///   3. build a decision tree model by calling the train with encrypted
  ///      labels api in cart_tree.h
  ///   4. update the raw_predictions, using the predicted labels from the
  ///      current tree, and iterate for the next tree, until finished

  // check loss function
  if (loss != "ls") {
    LOG(ERROR) << "The loss function is not supported, need to use the least "
                  "square error loss function.";
    exit(1);
  }
  // step 1
  // init the dummy estimator and compute raw_predictions:
  // for regression, using the mean label
  int sample_size = (int) training_data.size();
  EncodedNumber *raw_predictions = new EncodedNumber[sample_size];
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // init loss function, for regression loss, class num is set to 1
  LeastSquareError least_square_error(gbdt_model.tree_type, 1);
  // get the initial encrypted raw_predictions, all parties obtain raw_predictions
  least_square_error.get_init_raw_predictions(party, raw_predictions, sample_size,
                                              training_data, training_labels);
  // add the dummy mean prediction to the gbdt_model
  gbdt_model.dummy_predictors.emplace_back(least_square_error.dummy_prediction);
  // init the encrypted labels, only the active party can init
  EncodedNumber *encrypted_true_labels = new EncodedNumber[sample_size];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < sample_size; i++) {
      encrypted_true_labels[i].set_double(phe_pub_key->n[0], training_labels[i]);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         encrypted_true_labels[i], encrypted_true_labels[i]);
    }
  }
  // active party broadcasts the encrypted true labels
  party.broadcast_encoded_number_array(encrypted_true_labels, sample_size, ACTIVE_PARTY_ID);

  // iteratively train the regression trees
  LOG(INFO) << "tree_size = " << gbdt_model.tree_size;
  for (int tree_id = 0; tree_id < gbdt_model.tree_size; ++tree_id) {
    LOG(INFO) << "------------- build the " << tree_id << "-th tree -------------";
    // step 2
    // compute the encrypted residual, i.e., negative gradient
    EncodedNumber *residuals = new EncodedNumber[sample_size];
    least_square_error.negative_gradient(party,
                                         encrypted_true_labels,
                                         raw_predictions,
                                         residuals,
                                         sample_size);
    LOG(INFO) << "negative gradient computed finished";
    EncodedNumber *squared_residuals = new EncodedNumber[sample_size];
    square_encrypted_residual(party,
                              residuals,
                              squared_residuals,
                              sample_size,
                              PHE_FIXED_POINT_PRECISION);
    // flatten the residuals and squared_residuals into one vector for calling
    // the train_with_encrypted_labels in cart_tree.h
    EncodedNumber *flatten_residuals = new
        EncodedNumber[REGRESSION_TREE_CLASS_NUM * sample_size];
    for (int i = 0; i < sample_size; i++) {
      flatten_residuals[i] = residuals[i];
      flatten_residuals[sample_size + i] = squared_residuals[i];
    }
    LOG(INFO) << "compute squared residuals finished";
    // step 3
    // init a tree builder and train the model with flatten_residuals
    tree_builders.emplace_back(dt_param,
                               training_data, testing_data,
                               training_labels, testing_labels,
                               training_accuracy, testing_accuracy);
    tree_builders[tree_id].train(party, flatten_residuals);
    LOG(INFO) << "tree builder = " << tree_id << " train finished";
    google::FlushLogFiles(google::INFO);
    // step 4
    // after training the model, update the terminal regions, for least
    // square error, only need to update the raw_predictions
    least_square_error.update_terminal_regions(party, tree_builders[tree_id],
                                               encrypted_true_labels,
                                               residuals, raw_predictions,
                                               sample_size, learning_rate, 0);
    LOG(INFO) << "update terminal regions and raw predictions finished";
    google::FlushLogFiles(google::INFO);
    // save the trained regression tree model to gbdt_model
    gbdt_model.gbdt_trees.emplace_back(tree_builders[tree_id].tree);

    // free temporary variables
    delete [] residuals;
    delete [] squared_residuals;
    delete [] flatten_residuals;
    google::FlushLogFiles(google::INFO);
  }
  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
}

void GbdtBuilder::train_classification_task(Party party) {
  /// For classification, build the trees as follows:
  ///   1. if binary classification: apply the BinomialDeviance loss function,
  ///      where the number of estimator inside the loss function is set to 1,
  ///      first init a dummy estimator with probability = (#event / #total),
  ///      note that the corresponding get_init_raw_predictions = log(odds)
  ///      = log(p/(1-p), which will be updated in the following tree building
  ///      (1.1) then, for tree id 0 to num-1, init a decision tree, compute
  ///      the encrypted residuals between the original labels and raw_predictions
  ///      (1.2) build a regression tree model by calling the train with encrypted
  ///      labels api in cart_tree.h
  ///      (1.3) update the terminal regions, and update the raw_predictions,
  ///      iterate for the next tree, until finished
  ///   2. if multi-class classification: apply the MultinomialDeviance loss
  ///      function, where the number of estimator inside the loss function is
  ///      set to class_num, i.e., for each class, build n_estimator trees,
  ///      first init a dummy estimator with probability = (#event / #total)
  ///      for each class, and compute the raw_predictions
  ///      (2.1) for each class, convert to the one-hot encoding dataset, from
  ///      tree id 0 to num-1, init a decision tree, compute the encrypted
  ///      residuals between the original one-hot labels and raw_predictions
  ///      (2.2) build a regression tree model by calling the train with encrypted
  ///      labels api in cart_tree.h
  ///      (2.3) update the terminal regions, and update the raw_predictions,
  ///      iterate for the next tree, until finished

  // check loss function
  if (loss != "deviance") {
    LOG(ERROR) << "The loss function is not supported, need to use the deviance "
                  "loss function.";
    exit(1);
  }
  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  int sample_size = (int) training_data.size();
  // check class_num, for binary classification, use BinomialDeviance loss
  // for multi-class classification, use MultinomialDeviance loss
  if (dt_param.class_num == 2) {
    // step 1
    // init the dummy estimator and compute raw_predictions:
    // for binary classification, using the log(odds) for the raw_predictions
    EncodedNumber *raw_predictions = new EncodedNumber[sample_size];
    // init loss function, for regression loss, class num is set to 1
    BinomialDeviance binomial_deviance(gbdt_model.tree_type, dt_param.class_num);
    // get the initial encrypted raw_predictions, all parties obtain raw_predictions
    binomial_deviance.get_init_raw_predictions(party, raw_predictions, sample_size,
                                                training_data, training_labels);
    // add the dummy mean prediction to the gbdt_model
    gbdt_model.dummy_predictors.emplace_back(binomial_deviance.dummy_prediction);
    // init the encrypted labels, only the active party can init
    EncodedNumber *encrypted_true_labels = new EncodedNumber[sample_size];
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int i = 0; i < sample_size; i++) {
        encrypted_true_labels[i].set_double(phe_pub_key->n[0], training_labels[i]);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           encrypted_true_labels[i], encrypted_true_labels[i]);
      }
    }
    // active party broadcasts the encrypted true labels
    party.broadcast_encoded_number_array(encrypted_true_labels, sample_size, ACTIVE_PARTY_ID);

    // iteratively train the regression trees
    LOG(INFO) << "tree_size = " << gbdt_model.tree_size;
    for (int tree_id = 0; tree_id < gbdt_model.tree_size; ++tree_id) {
      LOG(INFO) << "------------- build the " << tree_id << "-th tree -------------";
      // step 2
      // compute the encrypted residual, i.e., negative gradient
      EncodedNumber *residuals = new EncodedNumber[sample_size];
      binomial_deviance.negative_gradient(party,
                                          encrypted_true_labels,
                                          raw_predictions,
                                          residuals,
                                          sample_size);
      LOG(INFO) << "negative gradient computed finished";
      EncodedNumber *squared_residuals = new EncodedNumber[sample_size];
      square_encrypted_residual(party,
                                residuals,
                                squared_residuals,
                                sample_size,
                                PHE_FIXED_POINT_PRECISION);
      // flatten the residuals and squared_residuals into one vector for calling
      // the train_with_encrypted_labels in cart_tree.h
      EncodedNumber *flatten_residuals = new
          EncodedNumber[REGRESSION_TREE_CLASS_NUM * sample_size];
      for (int i = 0; i < sample_size; i++) {
        flatten_residuals[i] = residuals[i];
        flatten_residuals[sample_size + i] = squared_residuals[i];
      }
      LOG(INFO) << "compute squared residuals finished";
      // step 3
      // init a tree builder and train the model with flatten_residuals
      // check gbdt task type, note that the gbdt type can be classification, but
      // all the trained decision trees are regression trees
      DecisionTreeParams classification_dt_params = dt_param;
      classification_dt_params.tree_type = "regression";
      classification_dt_params.class_num = REGRESSION_TREE_CLASS_NUM;
      tree_builders.emplace_back(classification_dt_params,
                                 training_data, testing_data,
                                 training_labels, testing_labels,
                                 training_accuracy, testing_accuracy);
      tree_builders[tree_id].train(party, flatten_residuals);
      LOG(INFO) << "tree builder = " << tree_id << " train finished";
      google::FlushLogFiles(google::INFO);
      // step 4
      // after training the model, update the terminal regions, for least
      // square error, only need to update the raw_predictions
      binomial_deviance.update_terminal_regions(party, tree_builders[tree_id],
                                                encrypted_true_labels,
                                                residuals, raw_predictions,
                                                sample_size, learning_rate, 0);
      LOG(INFO) << "update terminal regions and raw predictions finished";
      google::FlushLogFiles(google::INFO);
      // save the trained regression tree model to gbdt_model
      gbdt_model.gbdt_trees.emplace_back(tree_builders[tree_id].tree);

      // free temporary variables
      delete [] residuals;
      delete [] squared_residuals;
      delete [] flatten_residuals;
    }
    delete [] raw_predictions;
    delete [] encrypted_true_labels;
  }

  // multi-class classification
  if (gbdt_model.class_num > 2) {
    // step 1
    // init the dummy estimator and compute raw_predictions: for multi-class
    // classification, using the log(odds) for the raw_predictions of each tree
    EncodedNumber *raw_predictions = new EncodedNumber[sample_size * gbdt_model.class_num];
    // init loss function, for regression loss, class num is set to 1
    MultinomialDeviance multinomial_deviance(gbdt_model.tree_type, gbdt_model.class_num);
    // get the initial encrypted raw_predictions, all parties obtain raw_predictions
    multinomial_deviance.get_init_raw_predictions(party, raw_predictions,
                                                  sample_size * gbdt_model.class_num,
                                               training_data, training_labels);
    // add the dummy mean prediction to the gbdt_model
    for (int i = 0; i < gbdt_model.class_num; i++) {
      gbdt_model.dummy_predictors.emplace_back(multinomial_deviance.dummy_predictions[i]);
    }
    // init the encrypted labels, only the active party can init,
    // for multi-class classification, init one-hot labels for each class
    EncodedNumber *encrypted_true_labels = new EncodedNumber[sample_size * gbdt_model.class_num];
    if (party.party_type == falcon::ACTIVE_PARTY) {
      double positive_class = 1.0;
      double negative_class = 0.0;
      for (int c = 0; c < gbdt_model.class_num; c++) {
        // compute one hot label vector
        std::vector<double> one_hot_labels;
        for (int i = 0; i < sample_size; i++) {
          if ((int) training_labels[i] == c) {
            one_hot_labels.push_back(positive_class);
          } else {
            one_hot_labels.push_back(negative_class);
          }
        }
        for (int i = 0; i < sample_size; i++) {
          int index = c * sample_size + i;
          encrypted_true_labels[index].set_double(phe_pub_key->n[0], one_hot_labels[i]);
          djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                             encrypted_true_labels[index],
                             encrypted_true_labels[index]);
        }
      }
    }
    // active party broadcasts the encrypted true labels
    party.broadcast_encoded_number_array(encrypted_true_labels,
                                         sample_size * gbdt_model.class_num, ACTIVE_PARTY_ID);

    // iteratively train the regression trees, tree_size = n_estimator * class_num
    LOG(INFO) << "tree_size = " << gbdt_model.tree_size;
    LOG(INFO) << "n_estimator = " << gbdt_model.n_estimator;
    LOG(INFO) << "class_num = " << gbdt_model.class_num;
    for (int tree_id = 0; tree_id < gbdt_model.n_estimator; tree_id++) {
      LOG(INFO) << "------------- build the " << tree_id << "-th tree -------------";
      // step 2
      // compute the encrypted residual, i.e., negative gradient
      EncodedNumber *residuals = new EncodedNumber[sample_size * gbdt_model.class_num];
      multinomial_deviance.negative_gradient(party,
                                             encrypted_true_labels,
                                             raw_predictions,
                                             residuals,
                                             sample_size * gbdt_model.class_num);
      LOG(INFO) << "negative gradient computed finished";
      EncodedNumber *squared_residuals = new EncodedNumber[sample_size * gbdt_model.class_num];
      square_encrypted_residual(party,
                                residuals,
                                squared_residuals,
                                sample_size * gbdt_model.class_num,
                                PHE_FIXED_POINT_PRECISION);

      // build a tree for each class
      for (int c = 0; c < gbdt_model.class_num; c++) {
        LOG(INFO) << "----- build the " << tree_id << "-th tree ----- with class " << c;
        // this is the tree id in the gbdt_model.trees
        int read_tree_id = tree_id * gbdt_model.class_num + c;
        // flatten the residuals and squared_residuals into one vector for calling
        // the train_with_encrypted_labels in cart_tree.h
        EncodedNumber *flatten_residuals = new
            EncodedNumber[REGRESSION_TREE_CLASS_NUM * sample_size];
        for (int i = 0; i < sample_size; i++) {
          int real_sample_id = c * sample_size + i;
          flatten_residuals[i] = residuals[real_sample_id];
          flatten_residuals[sample_size + i] = squared_residuals[real_sample_id];
        }
        LOG(INFO) << "compute squared residuals finished";

        // step 3
        // init a tree builder and train the model with flatten_residuals
        // check gbdt task type, note that the gbdt type can be classification, but
        // all the trained decision trees are regression trees
        DecisionTreeParams classification_dt_params = dt_param;
        classification_dt_params.tree_type = "regression";
        classification_dt_params.class_num = REGRESSION_TREE_CLASS_NUM;
        tree_builders.emplace_back(classification_dt_params,
                                   training_data, testing_data,
                                   training_labels, testing_labels,
                                   training_accuracy, testing_accuracy);
        tree_builders[read_tree_id].train(party, flatten_residuals);
        LOG(INFO) << "tree builder = " << read_tree_id << " train finished";
        google::FlushLogFiles(google::INFO);

        // step 4
        // after training the model, update the terminal regions,
        // note that we need to copy the read_tree_raw_predictions and
        // read_tree_encrypted_truth_labels for this real_tree_id from the
        // original raw_predictions and encrypted_truth_labels before update terminal regions
        EncodedNumber *real_tree_encrypted_truth_labels = new EncodedNumber[sample_size];
        EncodedNumber *real_tree_raw_predictions = new EncodedNumber[sample_size];
        EncodedNumber *real_tree_residuals = new EncodedNumber[sample_size];
        for (int i = 0; i < sample_size; i++) {
          int real_sample_id = c * sample_size + i;
          real_tree_encrypted_truth_labels[i] = encrypted_true_labels[real_sample_id];
          real_tree_raw_predictions[i] = raw_predictions[real_sample_id];
          real_tree_residuals[i] = residuals[real_sample_id];
        }
        multinomial_deviance.update_terminal_regions(party, tree_builders[read_tree_id],
                                                     real_tree_encrypted_truth_labels,
                                                     real_tree_residuals, real_tree_raw_predictions,
                                                     sample_size, learning_rate, 0);
        // after update terminal regions, also update the raw_predictions
        // for the next iteration usage, the other two vectors can be discarded
        for (int i = 0; i < sample_size; i++) {
          int real_sample_id = c * sample_size + i;
          raw_predictions[real_sample_id] = real_tree_raw_predictions[i];
        }
        LOG(INFO) << "update terminal regions and raw predictions finished";
        google::FlushLogFiles(google::INFO);
        // save the trained regression tree model to gbdt_model
        gbdt_model.gbdt_trees.emplace_back(tree_builders[read_tree_id].tree);
        delete [] flatten_residuals;
        delete [] real_tree_encrypted_truth_labels;
        delete [] real_tree_raw_predictions;
        delete [] real_tree_residuals;
      }
      delete [] residuals;
      delete [] squared_residuals;
    }
    delete [] raw_predictions;
    delete [] encrypted_true_labels;
  }

  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
}

void GbdtBuilder::square_encrypted_residual(Party party,
                                            EncodedNumber *residuals,
                                            EncodedNumber *squared_residuals,
                                            int size,
                                            int phe_precision) {
  // given the encrypted residuals, need to compute the encrypted squared
  // labels, which is needed by the train_with_encrypted_labels api
  std::vector<int> public_values;
  std::vector<double> private_values;
  int class_num_for_regression = 1;
  public_values.push_back(size);
  public_values.push_back(class_num_for_regression);
  std::cout << "size = " << size << std::endl;
  std::cout << "class_num = " << class_num_for_regression << std::endl;
  LOG(INFO) << "size = " << size;
  LOG(INFO) << "class_num = " << class_num_for_regression;
  // convert the encrypted residuals into secret shares
  // the structure is one-dimensional vector, [tree_1 * sample_size] ... [tree_n * sample_size]
  std::vector<double> residuals_shares;
  party.ciphers_to_secret_shares(residuals,
                                 residuals_shares,
                                 size,
                                 ACTIVE_PARTY_ID,
                                 phe_precision);
  // pack to private values for sending to mpc parties
  for (int i = 0; i < size; i++) {
    private_values.push_back(residuals_shares[i]);
  }

  falcon::SpdzTreeCompType comp_type = falcon::GBDT_SQUARE_LABEL;

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
  std::vector<double> squared_residuals_shares = future_values.get();
  spdz_pruning_check_thread.join();

  // convert the secret shares to ciphertext, which is encrypted square residuals
  party.secret_shares_to_ciphers(squared_residuals,
                                 squared_residuals_shares,
                                 size,
                                 ACTIVE_PARTY_ID,
                                 phe_precision);
}

void GbdtBuilder::eval(Party party, falcon::DatasetType eval_type, const string &report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Start *************";
  const clock_t testing_start_time = clock();

  // init test data
  int dataset_size = (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector< std::vector<double> > cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  // compute predictions
  // now the predicted labels are computed by mpc, thus it is already the final label
  EncodedNumber* predicted_labels = new EncodedNumber[dataset_size];
  gbdt_model.predict(party, cur_test_dataset, dataset_size, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  EncodedNumber* decrypted_labels = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(predicted_labels,
                              decrypted_labels,
                              dataset_size,
                              ACTIVE_PARTY_ID);

  // calculate accuracy by the active party
  std::vector<double> predictions;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // decode decrypted predicted labels
    for (int i = 0; i < dataset_size; i++) {
      double x;
      decrypted_labels[i].decode(x);
      predictions.push_back(x);
    }

    // compute accuracy
    if (gbdt_model.tree_type == falcon::CLASSIFICATION) {
      int correct_num = 0;
      if (gbdt_model.class_num == 2) {
        // binary classification
        for (int i = 0; i < dataset_size; i++) {
          LOG(INFO) << "before predictions[" << i << "] = " << predictions[i];
          predictions[i] = (predictions[i] > LOGREG_THRES) ? 1.0 : 0.0;
          LOG(INFO) << "after predictions[" << i << "] = " << predictions[i];
          if (predictions[i] == cur_test_dataset_labels[i]) {
            correct_num += 1;
          }
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
        training_accuracy = mean_squared_error(predictions, cur_test_dataset_labels);
        LOG(INFO) << "Training accuracy = " << training_accuracy;
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = mean_squared_error(predictions, cur_test_dataset_labels);
        LOG(INFO) << "Testing accuracy = " << testing_accuracy;
      }
    }
  }

  // free memory
  delete [] predicted_labels;
  delete [] decrypted_labels;

  const clock_t testing_finish_time = clock();
  double testing_consumed_time = double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Evaluation time = " << testing_consumed_time;
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Finished *************";
  google::FlushLogFiles(google::INFO);
}

void train_gbdt(Party party, const std::string& params_str,
                const std::string& model_save_file, const std::string& model_report_file) {
  LOG(INFO) << "Run the example gbdt model train";
  std::cout << "Run the example gbdt model train" << std::endl;

  GbdtParams params;
  // currently for testing
  params.n_estimator = 8;
  params.learning_rate = 0.1;
  params.subsample = 1.0;
  params.loss = "deviance";
  params.dt_param.tree_type = "classification";
  params.dt_param.criterion = "gini";
  params.dt_param.split_strategy = "best";
  params.dt_param.class_num = 2;
  params.dt_param.max_depth = 3;
  params.dt_param.max_bins = 8;
  params.dt_param.min_samples_split = 5;
  params.dt_param.min_samples_leaf = 5;
  params.dt_param.max_leaf_nodes = 16;
  params.dt_param.min_impurity_decrease = 0.01;
  params.dt_param.min_impurity_split = 0.001;
  params.dt_param.dp_budget = 0.1;
  //  deserialize_gbdt_params(params, params_str);
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

  LOG(INFO) << "Init gbdt model builder";
  LOG(INFO) << "params.n_estimator = " << params.n_estimator;
  LOG(INFO) << "params.loss = " << params.loss;
  LOG(INFO) << "params.learning_rate = " << params.learning_rate;
  LOG(INFO) << "params.subsample = " << params.subsample;
  LOG(INFO) << "params.dt_param.tree_type = " << params.dt_param.tree_type;
  LOG(INFO) << "params.dt_param.criterion = " << params.dt_param.criterion;
  LOG(INFO) << "params.dt_param.split_strategy = " << params.dt_param.split_strategy;
  LOG(INFO) << "params.dt_param.class_num = " << params.dt_param.class_num;
  LOG(INFO) << "params.dt_param.max_depth = " << params.dt_param.max_depth;
  LOG(INFO) << "params.dt_param.max_bins = " << params.dt_param.max_bins;
  LOG(INFO) << "params.dt_param.min_samples_split = " << params.dt_param.min_samples_split;
  LOG(INFO) << "params.dt_param.min_samples_leaf = " << params.dt_param.min_samples_leaf;
  LOG(INFO) << "params.dt_param.max_leaf_nodes = " << params.dt_param.max_leaf_nodes;
  LOG(INFO) << "params.dt_param.min_impurity_decrease = " << params.dt_param.min_impurity_decrease;
  LOG(INFO) << "params.dt_param.min_impurity_split = " << params.dt_param.min_impurity_split;
  LOG(INFO) << "params.dt_param.dp_budget = " << params.dt_param.dp_budget;

  GbdtBuilder gbdt_builder(params,
      training_data,
      testing_data,
      training_labels,
      testing_labels,
      training_accuracy,
      testing_accuracy);

  LOG(INFO) << "Init gbdt model finished";
  std::cout << "Init gbdt model finished" << std::endl;
  google::FlushLogFiles(google::INFO);

  gbdt_builder.train(party);
  gbdt_builder.eval(party, falcon::TRAIN);
  gbdt_builder.eval(party, falcon::TEST);

  std::string pb_gbdt_model_string;
  serialize_gbdt_model(gbdt_builder.gbdt_model, pb_gbdt_model_string);
  save_pb_model_string(pb_gbdt_model_string, model_save_file);
  save_training_report(gbdt_builder.getter_training_accuracy(),
                       gbdt_builder.getter_testing_accuracy(),
                       model_report_file);

  LOG(INFO) << "Trained model and report saved";
  std::cout << "Trained model and report saved" << std::endl;
  google::FlushLogFiles(google::INFO);
}
