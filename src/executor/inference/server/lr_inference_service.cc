//
// Created by wuyuncheng on 4/6/21.
//

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <served/served.hpp>

#include <falcon/common.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/model/model_io.h>
#include <falcon/inference/server/lr_inference_service.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/io_util.h>

#include <glog/logging.h>
#include <falcon/operator/conversion/op_conv.h>

static int IS_MODEL_LOADED = 0;
static LogisticRegressionModel saved_lr_model;

void run_active_server_lr(const Party& party,
    const std::string& saved_model_file,
    const std::vector<int> & batch_indexes,
    EncodedNumber* decrypted_labels) {

  // step 1. load trained model if not loaded
  if (IS_MODEL_LOADED == 0){
    std::string saved_model_string;
    load_pb_model_string(saved_model_string, saved_model_file);
    deserialize_lr_model(saved_lr_model, saved_model_string);
    IS_MODEL_LOADED = 1;
  }

  // step 2. send batch indexes to passive parties
  std::string batch_indexes_str;
  serialize_int_array(batch_indexes, batch_indexes_str);
  for (int i = 0; i < party.party_num; i++) {
    if (i != party.party_id) {
      party.send_long_message(i, batch_indexes_str);
    }
  }

  // step 3: active party computes the logistic function and compare the accuracy
  int sample_num = batch_indexes.size();
  lr_inference_logic(batch_indexes, party, decrypted_labels);

  delete [] decrypted_labels;
  std::cout << "Broadcast client's batch requests to other parties" << std::endl;
  LOG(INFO) << "Broadcast client's batch requests to other parties";
  google::FlushLogFiles(google::INFO);

}

void run_passive_server_lr(const std::string& saved_model_file,
                           const Party& party) {

  std::string saved_model_string;
  load_pb_model_string(saved_model_string, saved_model_file);
  deserialize_lr_model(saved_lr_model, saved_model_string);

  // keep listening requests from the active party
  while (true) {
    std::cout << "listen active party's request" << std::endl;

    // 1. receive sample id from active party
    std::string recv_batch_indexes_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_batch_indexes_str);
    std::vector<int> batch_indexes;
    deserialize_int_array(batch_indexes, recv_batch_indexes_str);

    std::cout << "Received active party's batch indexes" << std::endl;
    LOG(INFO) << "Received active party's batch indexes";

    // step 2: passive party computes the logistic function
    auto* decrypted_labels = new EncodedNumber[batch_indexes.size()];
    lr_inference_logic(batch_indexes, party, decrypted_labels);
    delete [] decrypted_labels;

    google::FlushLogFiles(google::INFO);
  }
}


void lr_inference_logic(
    const std::vector<int> & batch_indexes,
    const Party& party,
    EncodedNumber* decrypted_labels){

  int sample_num = batch_indexes.size();
  // retrieve batch samples and encode (notice to use cur_batch_size
  // instead of default batch size to avoid unexpected batch)
  auto* predicted_labels = new EncodedNumber[sample_num];
  std::vector< std::vector<double> > batch_samples;
  for (int i = 0; i < sample_num; i++) {
    batch_samples.push_back(party.getter_local_data()[batch_indexes[i]]);
  }
  saved_lr_model.predict(const_cast<Party &>(party), batch_samples, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  collaborative_decrypt(party, predicted_labels, decrypted_labels, sample_num, ACTIVE_PARTY_ID);

  std::cout << "Collaboratively decryption finished" << std::endl;
  LOG(INFO) << "Collaboratively decryption finished";

  delete [] predicted_labels;
}