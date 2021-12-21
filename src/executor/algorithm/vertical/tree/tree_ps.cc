//
// Created by nai li xing on 08/12/21.
//



#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/tree/tree_ps.h>
#include <falcon/utils/io_util.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <iostream>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/model/model_io.h>


DTParameterServer::DTParameterServer(const DTParameterServer &obj) {}

DTParameterServer::~DTParameterServer() = default;

void DTParameterServer::broadcast_train_test_data(
    const std::vector<std::vector<double>> &training_data,
    const std::vector<std::vector<double>> &testing_data,
    const std::vector<double> &training_labels,
    const std::vector<double> &testing_labels){

  // ps splits the data from vertical dimention, and sends the splitted part to workers
  std::string training_data_str, testing_data_str, training_labels_str, testing_labels_str;

  assert(training_data[0].size() > this->worker_channels.size());

  int each_worker_features_num = training_data[0].size()/this->worker_channels.size();

  falcon_print(std::cout,  "[PS.broadcast_train_test_data]:",
      "worker num",
      this->worker_channels.size(),
      "data size",
      training_data[0].size(),
      "each_worker_features_num is", each_worker_features_num);

  int train_data_prev_index = 0;
  int train_data_last_index;

  int test_data_prev_index = 0;
  int test_data_last_index;

  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {

    // get the last index of training_data[0].size()
    std::vector<std::vector<double>> tmp_training_data(training_data.size());
    std::vector<std::vector<double>> tmp_testing_data(testing_data.size());

    if (wk_index == this->worker_channels.size()-1){
      train_data_last_index = training_data[0].size();
      test_data_last_index = testing_data[0].size();
    }else{
      train_data_last_index = train_data_prev_index+each_worker_features_num; //4
      test_data_last_index = test_data_prev_index+each_worker_features_num; //4
    }

    // assign
    for(int j = 0; j < training_data.size(); j++){
      tmp_training_data[j].insert(tmp_training_data[j].begin(),
          training_data[j].begin()+train_data_prev_index,
          training_data[j].begin()+train_data_last_index);
    }

    for(int j = 0; j < testing_data.size(); j++){
      tmp_testing_data[j].insert(tmp_testing_data[j].begin(),
          testing_data[j].begin()+test_data_prev_index,
          testing_data[j].begin()+test_data_last_index);
    }

    serialize_double_matrix(tmp_training_data, training_data_str);
    serialize_double_matrix(tmp_testing_data, testing_data_str);
    this->send_long_message_to_worker(wk_index, training_data_str);
    this->send_long_message_to_worker(wk_index, testing_data_str);

    falcon_print(std::cout,  "[PS.broadcast_train_test_data]:",
                 "assign",train_data_prev_index, "-", train_data_last_index,
                 "to worker", wk_index);

    train_data_prev_index = train_data_last_index;
    test_data_prev_index = test_data_last_index;
  }

  // only active ps sends the training labels and testing labels to workers
  if (party.party_type == falcon::ACTIVE_PARTY) {
    serialize_double_array(training_labels, training_labels_str);
    serialize_double_array(testing_labels, testing_labels_str);
    for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
      this->send_long_message_to_worker(wk_index, training_labels_str);
      this->send_long_message_to_worker(wk_index, testing_labels_str);
    }
  }

}


std::vector<int> DTParameterServer::partition_examples(std::vector<int> batch_indexes){

  int mini_batch_size = int(batch_indexes.size()/this->worker_channels.size());

  log_info("ps worker size = " + std::to_string(this->worker_channels.size()));
  log_info("mini batch size = " + std::to_string(mini_batch_size));

  std::vector<int> message_sizes;
  // deterministic partition given the batch indexes
  int index = 0;
  for(int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++){

    // generate mini-batch for this worker
    std::vector<int>::const_iterator first1 = batch_indexes.begin() + index;
    std::vector<int>::const_iterator last1  = batch_indexes.begin() + index + mini_batch_size;

    if (wk_index == this->worker_channels.size() - 1){
      last1  = batch_indexes.end();
    }
    std::vector<int> mini_batch_indexes(first1, last1);

    // serialize mini_batch_indexes to str
    std::string mini_batch_indexes_str;
    serialize_int_array(mini_batch_indexes, mini_batch_indexes_str);

    // record size of mini_batch_indexes, used in deserialization process
    message_sizes.push_back((int) mini_batch_indexes.size());

    // send to worker
    this->send_long_message_to_worker(wk_index, mini_batch_indexes_str);
    // update index
    index += mini_batch_size;
  }

  for (int i = 0; i < message_sizes.size(); i++) {
    log_info("message_sizes[" + std::to_string(i) + "] = " + std::to_string(message_sizes[i]));
  }
  log_info("Broadcast client's batch requests to other workers");

  return message_sizes;

}

void DTParameterServer::broadcast_phe_keys(){
  std::string phe_keys_str = party.export_phe_key_string();
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
    this->send_long_message_to_worker(wk_index, phe_keys_str);
  }
}


void DTParameterServer::distributed_train(){

  log_info("************* [Ps.distributed_train] Training started *************");
  const clock_t training_start_time = clock();

  /// 1. train the tree model
  build_tree();

  /// 2. clear and log
  const clock_t training_finish_time = clock();
  double training_consumed_time = double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("[Ps.distributed_train]: tree capacity = " + to_string(alg_builder.tree.capacity));
  log_info("[Ps.distributed_train]: Training time = " + to_string(training_consumed_time));
  log_info("************* [Ps.distributed_train] Training Finished *************");

}


void DTParameterServer::build_tree(){

  /// 0. init depth and impurity for root node
  alg_builder.tree.nodes[0].depth = 0;

  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  EncodedNumber max_impurity;
  max_impurity.set_double(phe_pub_key->n[0], MAX_IMPURITY, PHE_FIXED_POINT_PRECISION);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                     alg_builder.tree.nodes[0].impurity, max_impurity);
  djcs_t_free_public_key(phe_pub_key);

  /// 1. create dynamic stacks and init the value
  std::vector<int> node_index_stack;
  // store the index of left and right child
  node_index_stack.push_back(0);
  // store the mask array or label of left and right child
  std::vector< EncodedNumber* > node_mask_stack;
  std::vector< EncodedNumber* > node_label_stack;

  /// 2. start training
  while (!node_index_stack.empty()){

    /// 2.1 get the current node index
    // get the last element of the stack
    int node_index = node_index_stack.back();
    node_index_stack.pop_back();

    log_info("[Ps.build_tree]: step 2.1, train node with index = " + to_string(node_index));

    // calculate the child index
    int left_child_index = 2 * node_index + 1;
    int right_child_index = 2 * node_index + 2;

    /// 2.2 check if stopping the training
    if (node_index >= alg_builder.tree.capacity) {
      log_info("[Ps.build_tree]: step 2.2, Node index exceeds the maximum tree depth, boardcast stop to workers");
      for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
        this->send_long_message_to_worker(wk_index, "stop");
      }
      break;
    }
//    // check if stopping the training
//    if (node_index > 0){
//
//      log_info("[Ps.build_tree]: step 2.2, check if stopping the training");
//
//      EncodedNumber* sample_mask_iv = node_mask_stack.back();
//      node_mask_stack.pop_back();
//      EncodedNumber* encrypted_labels = node_label_stack.back();
//      node_label_stack.pop_back();
//
//      // print sample_mask_iv[sample_num-1] info for debug
//      int sample_num = alg_builder.train_data_size;
//      log_info("[Ps.build_tree]: step 2.2, sample_num = " + to_string(sample_num));
//      log_info("[Ps.build_tree]: step 2.2, sample_mask_iv[sample_num-1].exponent = " + to_string(sample_mask_iv[sample_num-1].getter_exponent()));
//      mpz_t t;
//      mpz_init(t);
//      sample_mask_iv[sample_num-1].getter_n(t);
//      gmp_printf("[Ps.build_tree]: step 2.2, sample_mask_iv[sample_num-1].n = %Zd", t);
//      mpz_clear(t);
//
//      /// step 1: check pruning condition via spdz computation
//      bool is_satisfied = alg_builder.check_pruning_conditions(
//          party, node_index, sample_mask_iv);
//
//      log_info("[Ps.build_tree]: step 2.2, is_satisfied = " + to_string(is_satisfied));
//
//      /// step 2: if satisfied, compute label via spdz computation
//      if (is_satisfied) {
//        alg_builder.compute_leaf_statistics(party, node_index, sample_mask_iv, encrypted_labels);
//        log_info("[Ps.build_tree]: step 2.2, Node is satisfied, boardcast stop to workers");
//
//        for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
//          this->send_long_message_to_worker(wk_index, "stop");
//        }
//        break;
//      }else{
//        log_info("[Ps.build_tree]: step 2.2, Node is not satisfied, continue training");
//      }
//    }

    /// 2.3 send node index to worker
    log_info("[Ps.build_tree]: step 2.3, ps boardcast node index = " + to_string(node_index) + "to workers");
    std::string node_index_str = to_string(node_index);
    for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
      this->send_long_message_to_worker(wk_index, node_index_str);
    }

    /// 2.4 : wait worker finish execution, get all local best split
    log_info("[Ps.build_tree]: step 2.4, ps wait_worker_complete");
    std::vector<string> encoded_message = this->wait_worker_complete();

    /// 2.5 : ps compare the local best split and get the global best split
    log_info("[Ps.build_tree]: step 2.5, ps begin to compare and get the best split");
    vector<double> final_decision_res = retrieve_global_best_split(encoded_message);
    log_info("[Ps.build_tree]: step 2.5, size of final_decision_res " +  to_string(final_decision_res.size()));
//    assert(final_decision_res.size() == 4);

    int global_best_split_worker_id = final_decision_res[3];
    //int global_best_split_party_id = final_decision_res[4];

    /// 2.6 : send the best split to workers
    log_info("[Ps.build_tree]: step 2.6, ps boardcast global best to other workers");
    std::string final_decision_str;
    serialize_double_array(final_decision_res, final_decision_str);
    for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
      this->send_long_message_to_worker(wk_index, final_decision_str);
    }

    /// 2.7 : receive from worker, get party_id, each worker should get the same party_id
    log_info("[Ps.build_tree]: step 2.7, ps receive party-id");
    int best_party_id;
    for(int wk_index=0; wk_index< this->worker_channels.size(); wk_index++){
      std::string party_id_str;
      this->recv_long_message_from_worker(wk_index, party_id_str);
      best_party_id = std::stoi(party_id_str);
    }

    /// 2.8 : best party's ps received the updated masks and labels and then it sent them to other workers
    log_info("[Ps.build_tree]: step 2.8, best party's ps received the updated masks and labels and then it sent them to other workers");
    std::string recv_update_str_sample_iv, recv_update_str_encrypted_labels_left, recv_update_str_encrypted_labels_right;

    if (party.party_id == best_party_id){
      log_info("[Ps.build_tree]: step 2.8, party id is matched, receive updated info from ps and send them to other worker");

      // only receive from matched worker
      for(int wk_index=0; wk_index< this->worker_channels.size(); wk_index++){
        if (wk_index == global_best_split_worker_id){

          recv_long_message_from_worker(wk_index,  recv_update_str_sample_iv);
          recv_long_message_from_worker(wk_index,  recv_update_str_encrypted_labels_left);
          recv_long_message_from_worker(wk_index,  recv_update_str_encrypted_labels_right);
          break;
        }
      }

      log_info("[Ps.build_tree]: step 2.8, ps receive from match worker recv_update_str_sample_iv size: " + to_string(recv_update_str_sample_iv.size() ));
      log_info("[Ps.build_tree]: step 2.8, ps receive from match worker recv_update_str_encrypted_labels_left size: " + to_string(recv_update_str_encrypted_labels_left.size() ));
      log_info("[Ps.build_tree]: step 2.8, ps receive from match worker recv_update_str_encrypted_labels_right size: " + to_string(recv_update_str_encrypted_labels_right.size() ));

      // send received masks and labels to other workers at this party
      for(int wk_index=0; wk_index< this->worker_channels.size(); wk_index++){
        if (wk_index != global_best_split_worker_id){
          this->send_long_message_to_worker(wk_index, recv_update_str_sample_iv);
          this->send_long_message_to_worker(wk_index, recv_update_str_encrypted_labels_left);
          this->send_long_message_to_worker(wk_index, recv_update_str_encrypted_labels_right);
        }
      }
    }

    else {
      log_info("[Ps.build_tree]: step 2.8, party id is not matched, receive updated info from other party's worker");

      for(int wk_index=0; wk_index< this->worker_channels.size(); wk_index++){
        recv_long_message_from_worker(wk_index,  recv_update_str_sample_iv);
        recv_long_message_from_worker(wk_index,  recv_update_str_encrypted_labels_left);
        recv_long_message_from_worker(wk_index,  recv_update_str_encrypted_labels_right);

        log_info("[Ps.build_tree]: step 2.8, other party's ps receive from worker index: "+ to_string(wk_index));
        log_info("[Ps.build_tree]: step 2.8, other party's ps receive from worker recv_update_str_sample_iv size: " + to_string(recv_update_str_sample_iv.size() ));
        log_info("[Ps.build_tree]: step 2.8, other party's ps receive from worker recv_update_str_encrypted_labels_left size: " + to_string(recv_update_str_encrypted_labels_left.size() ));
        log_info("[Ps.build_tree]: step 2.8, other party's ps receive from worker recv_update_str_encrypted_labels_right size: " + to_string(recv_update_str_encrypted_labels_right.size() ));
      }
    }

    /// 2.9 : deserialize the updated sample iv and label
    log_info("[Ps.build_tree]: step 2.9, deserialize the updated sample iv and label");

    auto *sample_mask_iv_left = new EncodedNumber[alg_builder.train_data_size];
    auto *sample_mask_iv_right = new EncodedNumber[alg_builder.train_data_size];
    auto *encrypted_labels_left = new EncodedNumber[alg_builder.train_data_size * alg_builder.class_num];
    auto *encrypted_labels_right = new EncodedNumber[alg_builder.train_data_size * alg_builder.class_num];

    int recv_source_party_id, recv_best_party_id, recv_best_feature_id, recv_best_split_id;
    EncodedNumber recv_left_impurity, recv_right_impurity;
    deserialize_update_info(recv_source_party_id, recv_best_party_id,
                            recv_best_feature_id, recv_best_split_id,
                            recv_left_impurity, recv_right_impurity,
                            sample_mask_iv_left, sample_mask_iv_right,
                            recv_update_str_sample_iv);

    log_info("[Ps.build_tree]: step 2.9, class num: " + to_string(alg_builder.class_num ));
    log_info("[Ps.build_tree]: step 2.9, sample_num: " + to_string(alg_builder.train_data_size ));
    log_info("[Ps.build_tree]: step 2.9, size of encrypted_labels_right: " + to_string(alg_builder.class_num * alg_builder.train_data_size));

    deserialize_encoded_number_array(encrypted_labels_left, alg_builder.class_num * alg_builder.train_data_size,
                                     recv_update_str_encrypted_labels_left);
    deserialize_encoded_number_array(encrypted_labels_right, alg_builder.class_num * alg_builder.train_data_size,
                                     recv_update_str_encrypted_labels_right);


    /// 2.10 : update current node index for prediction
    alg_builder.tree.nodes[node_index].node_type = falcon::INTERNAL;
    alg_builder.tree.nodes[node_index].is_self_feature = 0;
    alg_builder.tree.nodes[node_index].best_party_id = recv_best_party_id;
    alg_builder.tree.nodes[node_index].best_feature_id = recv_best_feature_id;
    alg_builder.tree.nodes[node_index].best_split_id = recv_best_split_id;

    alg_builder.tree.nodes[left_child_index].depth = alg_builder.tree.nodes[node_index].depth + 1;
    alg_builder.tree.nodes[left_child_index].impurity = recv_left_impurity;
    alg_builder.tree.nodes[right_child_index].depth = alg_builder.tree.nodes[node_index].depth + 1;
    alg_builder.tree.nodes[right_child_index].impurity = recv_right_impurity;

    alg_builder.tree.internal_node_num += 1;
    alg_builder.tree.total_node_num += 1;

    log_info("[DT_train_worker.distributed_train]: step 2.10, ps update tree, "
             "node_index:" + to_string(node_index) +
        "best_party_id:" + to_string(recv_best_party_id) +
        "recv_best_feature_id) :" + to_string(recv_best_feature_id) +
        "best_split_id:" + to_string(recv_best_split_id));

    // update index stack
    node_index_stack.push_back(left_child_index);
    node_index_stack.push_back(right_child_index);

    int sample_num = alg_builder.train_data_size;
    log_info("[DT_train_worker.distributed_train]: step 2.10, sample_mask_iv_right[sample_num-1].exponent = " + to_string(sample_mask_iv_right[sample_num-1].getter_exponent()));
    mpz_t t;
    mpz_init(t);
    sample_mask_iv_right[sample_num-1].getter_n(t);
    gmp_printf("[DT_train_worker.distributed_train]: step 2.10, sample_mask_iv_right[sample_num-1].n = %Zd", t);
    mpz_clear(t);

    // update mask stack
    node_mask_stack.push_back(sample_mask_iv_left);
    node_mask_stack.push_back(sample_mask_iv_right);

    EncodedNumber* test1 = node_mask_stack.back();
    EncodedNumber* test2 = node_mask_stack.back();
    log_info("[DT_train_worker.distributed_train]: step 2.10, test1[sample_num-1].exponent = " + to_string(test1[sample_num-1].getter_exponent()));
    mpz_t t1;
    mpz_init(t1);
    test1[sample_num-1].getter_n(t1);
    gmp_printf("[DT_train_worker.distributed_train]: step 2.10, test1[sample_num-1].n = %Zd", t1);
    mpz_clear(t1);

    log_info("[DT_train_worker.distributed_train]: step 2.10, test2[sample_num-1].exponent = " + to_string(test2[sample_num-1].getter_exponent()));
    mpz_t t2;
    mpz_init(t2);
    test2[sample_num-1].getter_n(t2);
    gmp_printf("[DT_train_worker.distributed_train]: step 2.10, test2[sample_num-1].n = %Zd", t2);
    mpz_clear(t2);

    // update label stack
    node_label_stack.push_back(encrypted_labels_left);
    node_label_stack.push_back(encrypted_labels_right);

//    delete [] sample_mask_iv_left;
//    delete [] sample_mask_iv_right;
//    delete [] encrypted_labels_left;
//    delete [] encrypted_labels_right;

  }

  /// 3 : send "stop" to worker to stop training
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
    this->send_long_message_to_worker(wk_index, "stop");
  }

}

void DTParameterServer::distributed_eval(
    falcon::DatasetType eval_type,
    const std::string& report_save_path) {

  // step 1: init test data
  int dataset_size =
      (eval_type == falcon::TRAIN) ?
      (int) this->alg_builder.getter_training_data().size() :
      (int) this->alg_builder.getter_testing_data().size();

  std::vector<int> cur_test_data_indexes;
  cur_test_data_indexes.reserve(dataset_size);
  for (int i = 0; i < dataset_size; i++) {
    cur_test_data_indexes.push_back(i);
  }

  // step 2: do the prediction
  auto* decrypted_labels = new EncodedNumber[dataset_size];
  distributed_predict(cur_test_data_indexes, decrypted_labels);

  // step 3: compute and save matrix
  if (party.party_type == falcon::ACTIVE_PARTY) {
    save_model(report_save_path);
  }
  delete [] decrypted_labels;

}


void DTParameterServer::distributed_predict(
    const std::vector<int>& cur_test_data_indexes,
    EncodedNumber* predicted_labels){

  log_info("current channel size = " + std::to_string(this->worker_channels.size()));

  // step 1: partition sample ids, every ps partition in the same way
  std::vector<int> message_sizes = this->partition_examples(cur_test_data_indexes);

  log_info("cur_test_data_indexes.size = " + std::to_string(cur_test_data_indexes.size()));
  for (int i = 0; i < message_sizes.size(); i++) {
    log_info("message_sizes[" + std::to_string(i) + "] = " + std::to_string(message_sizes[i]));
  }

  double cur_accuracy;
  // step 2: if active party, wait worker finish execution
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::vector< string > encoded_messages = this->wait_worker_complete();

    // deserialize encrypted predicted labels
    for (int i=0; i < encoded_messages.size(); i++){
      cur_accuracy += std::stoi(encoded_messages[i]);
    }

    log_info("training accuracy = " + std::to_string(cur_accuracy));
  }
}


void DTParameterServer::save_model(const std::string& model_save_file){

  std::string pb_dt_model_string;
  serialize_tree_model(alg_builder.tree, pb_dt_model_string);
  save_pb_model_string(pb_dt_model_string, model_save_file);
  save_training_report(alg_builder.getter_training_accuracy(),
                       alg_builder.getter_testing_accuracy(),
                       model_save_file);

}


std::vector<double> DTParameterServer::retrieve_global_best_split(const std::vector<string>& encoded_msg){
  log_info("[Ps.retrieve_global_best_split]: send encoded msgs to worker to compare and get global best splits");

  // currently, ps dont have mpc, so send it to worker 0 to compute
  int received_worker_id = 0;
  // send number of workers to worker 0
  this->send_long_message_to_worker(received_worker_id, to_string(encoded_msg.size()));

  for (const auto& msg: encoded_msg){
    this->send_long_message_to_worker(received_worker_id, msg);
  }

  std::string received_str;
  std::vector<double> global_best_split;
  this->recv_long_message_from_worker(received_worker_id, received_str);
  deserialize_double_array(global_best_split, received_str);

  int best_split_index = (int) global_best_split[0];
  double left_impurity = global_best_split[1];
  double right_impurity = global_best_split[2];
  int best_split_worker_id = global_best_split[3];


  log_info("[Ps.retrieve_global_best_split]: received the best from worker: "
      "\nbest_split_index is " + to_string(best_split_index) +
      "\nleft_impurity is " + to_string(left_impurity) +
      "\nright_impurity is " + to_string(right_impurity) +
      "\nbest_split_worker_id is " + to_string(best_split_worker_id) );

  return global_best_split;
}


std::vector<string> DTParameterServer::wait_worker_complete(){
  std::vector<string> encoded_messages;
  // wait until all received str has been stored to encrypted_gradient_strs
  while (this->worker_channels.size() != encoded_messages.size()){
    for(int wk_index=0; wk_index< this->worker_channels.size(); wk_index++){
      std::string message;
      this->recv_long_message_from_worker(wk_index, message);
      encoded_messages.push_back(message);
    }
  }
  return encoded_messages;
}


void launch_dt_parameter_server(
    Party* party,
    const std::string& params_str,
    const std::string& ps_network_config_pb_str,
    const std::string& model_save_file,
    const std::string& model_report_file
    ){

  log_info("Run the example decision tree train");
  falcon_print(std::cout, "Run the example decision tree train");

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
  deserialize_dt_params(params, params_str);
  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;

  std::vector< std::vector<double> > training_data;
  std::vector< std::vector<double> > testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;

  log_info("[DTParameterServer]: split_train_test_data");

  party->split_train_test_data(split_percentage,
                              training_data,
                              testing_data,
                              training_labels,
                              testing_labels);

  log_info("Init decision tree model builder");
  log_info("params.tree_type = " + params.tree_type);
  log_info("params.criterion = " + params.criterion);
  log_info("params.split_strategy = " + params.split_strategy);
  log_info("params.class_num = " + to_string(params.class_num));
  log_info("params.max_depth = " + to_string(params.max_depth));
  log_info("params.max_bins = " + to_string(params.max_bins));
  log_info("params.min_samples_split = " + to_string(params.min_samples_split));
  log_info("params.min_samples_leaf = " + to_string(params.min_samples_leaf));
  log_info("params.max_leaf_nodes = " + to_string(params.max_leaf_nodes));
  log_info("params.min_impurity_decrease = " + to_string(params.min_impurity_decrease));
  log_info("params.min_impurity_split = " + to_string(params.min_impurity_split));
  log_info("params.dp_budget = " + to_string(params.dp_budget));

  falcon_print(std::cout,  "Init decision tree model");

  log_info("[DTParameterServer]: Init decision tree model");

  auto decision_tree_builder = new DecisionTreeBuilder (params,
                                            training_data,
                                            testing_data,
                                            training_labels,
                                            testing_labels,
                                            training_accuracy,
                                            testing_accuracy);

  log_info("[DTParameterServer]: Init decision tree model finished");

  falcon_print(std::cout,  "train labels size are", training_labels.size());
  falcon_print(std::cout,  "Init decision tree model finished");

  auto ps = new DTParameterServer(*decision_tree_builder, *party, ps_network_config_pb_str);

  // here need to send the train/test data/labels to workers
  // also, need to send the phe keys to workers
  // accordingly, workers have to receive and deserialize these

  log_info("[DTParameterServer]: begin to broadcast_train_test_data and keys");

  ps->broadcast_train_test_data(training_data, testing_data, training_labels, testing_labels);
  ps->broadcast_phe_keys();

  log_info("[DTParameterServer]: start to train the task in a distributed way");
  ps->distributed_train();

  log_info("[DTParameterServer]: start to eval the task in a distributed way");
  // evaluate the model on the training and testing datasets
  log_info("[Ps.distributed_eval]: Evaluation on train dataset Start");
  ps->distributed_eval(falcon::TRAIN, model_report_file);
  log_info("[Ps.distributed_eval]: Evaluation on test dataset Start");
  ps->distributed_eval(falcon::TEST, model_report_file);

  log_info("[DTParameterServer]: start to save model");
  ps->save_model(model_save_file);

  delete decision_tree_builder;
  delete ps;

}

