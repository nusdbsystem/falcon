//
// Created by naili on 21/6/22.
//

#include "falcon/algorithm/vertical/preprocessing/pre_feature_selection.h"
#include "falcon/algorithm/vertical/preprocessing/pearson_correlation.h"

#include <string>
#include <glog/logging.h>
#include <falcon/utils/io_util.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/base64.h>
#include <falcon/utils/pb_converter/preprocessing_converter.h>
#include <falcon/common.h>
#include <falcon/utils/pb_converter/common_converter.h>


void FeatSel::select_features(Party party,
                                    int num_samples,
                                    const std::string &feature_selection,
                                    const std::string &selected_features_file,
                                    const std::string& ps_network_str,
                                    int is_distributed,
                                    int distributed_role,
                                    int worker_id){

  // 1. read the selected sample file
  std::vector<std::vector<double> > dataset = party.getter_local_data();

  int selected_sample_size = dataset.size();
  log_info("Read the generated samples finished");

  if (feature_selection != "pearson_correlation"){
    log_error("only support pearson_correlation in pre-processing stage");
    exit(EXIT_FAILURE);
  }

  // 0. retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // 1. Active party sends local labels to other party, while passive party receive and save locally.
  auto* selected_sample_true_labels = new EncodedNumber[selected_sample_size];

  // 1.1 encrypt local label
  if (party.party_type == falcon.ACTIVE_PARTY){

    std::vector<double> labels = party.getter_labels();

    for (int i = 0; i < selected_sample_size; i++){
      selected_sample_true_labels[i].set_double(
          phe_pub_key->n[0],
          labels[i],
          PHE_FIXED_POINT_PRECISION);

      // encrypted label
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         selected_sample_true_labels[i],
                         selected_sample_true_labels[i]);
    }
    // serialize encrypted label and send out
    std::string enc_label_str;
    serialize_encoded_number_array(selected_sample_true_labels, selected_sample_size, enc_label_str);
    for (int j = 0; j < party.party_num; j++){
      if (j != party.party_id){
        party.send_long_message(j, enc_label_str);
      }
    }
  }

  // passive party receive labels.
  if (party.party_type == falcon.PASSIVE_PARTY){
    std::string recv_enc_label_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_enc_label_str);
    deserialize_encoded_number_array(selected_sample_true_labels, selected_sample_size, recv_enc_label_str);
  }

  // 2. each party calculate the score
  PearsonCorrelation pearson_cor;
  EncodedNumber* local_score = new EncodedNumber[dataset[0].size()];

  // for each feature
  for ( int fid = 0; fid < dataset[0].size(); fid++ ){
    std::vector< double > selected_feature_vec;
    // generate the feature vector across all examples.
    for ( int row = 0; row < dataset.size(); row ++){
      selected_feature_vec.push_back(dataset[row][fid]);
    }

    // calculate the score.
    EncodedNumber score = pearson_cor.calculate_score(party,
                                               selected_feature_vec,
                                               selected_sample_true_labels,
                                               selected_feature_vec.size());

    local_score[fid] = score;
  }

  // 3. all party send score to active party.
  std::vector<int> selected_feat_idx;
  std::vector<int> party_score_size;
  if ( party.party_type == falcon.ACTIVE_PARTY){
    // store score for all features across parties in sequence of partyID.
    std::vector< EncodedNumber* > global_score;
    // 1.1 add local score into global score list
    party_score_size.push_back(dataset[0].size());
    global_score.push_back(local_score);

    // 1.2 receive and add other party's score into global scores list.
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {

        // receive size and add size back to queue
        std::string recv_score_size_str;
        party.recv_long_message(id, recv_score_size_str);
        int recv_score_size = stoi(recv_score_size_str);
        party_score_size.push_back(recv_score_size);

        // receive score array
        std::string recv_score_str;
        party.recv_long_message(id, recv_score_str);
        EncodedNumber* recv_score;
        deserialize_encoded_number_array(recv_score, recv_score_size, recv_score_str);
        global_score.push_back(recv_score);
      }
    }

    // 1.3 find top-k score and match to each party's local index
    std::vector<std::vector<int>> party_selected_feat_idx = find_party_feat_index(global_score, 10);

    // 1.4 send selected features index to other party.
    selected_feat_idx = party_selected_feat_idx[0];
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string selected_feat_idx_str;
        serialize_int_array(party_selected_feat_idx[id], selected_feat_idx_str);
        party.send_long_message(id, selected_feat_idx_str);
      }
    }
  }

  // passive perform following
  if (party.party_type == falcon.PASSIVE_PARTY){

    // 1.send to active party the size of the score array for deserialize
    party.send_long_message(ACTIVE_PARTY_ID, to_string(dataset[0].size()));

    // 1.send to active party the score array
    std::string local_score_str;
    serialize_encoded_number_array(local_score, dataset[0].size(), local_score_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_score_str);

    // receive selected_feat_idx from active party
    std::string recv_selected_idx_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_selected_idx_str);
    deserialize_int_array(selected_feat_idx, recv_selected_idx_str);
  }

  // each party begin to select features
  log_info("begin to select features and write dataset");
  std::vector<std::vector<double>> selected_feat_samples;

  for (int i = 0; i < selected_sample_size; i++) {
    std::vector<double> sample;
    for (int j = 0; j < selected_feat_idx.size(); j++) {
      int idx = selected_feat_idx[j];
      sample.push_back(dataset[i][idx]);
    }
    selected_feat_samples.push_back(sample);
  }

  write_dataset_to_file(selected_feat_samples, ',', selected_features_file);

  delete [] selected_sample_true_labels;
  delete []local_score;

}


void pre_feat_sel(Party party, const std::string& params_str,
                      const std::string& output_path_prefix,
                      const std::string& ps_network_str,
                      int is_distributed,
                      int distributed_role,
                      int worker_id){

  log_info("Begin to select features");
  // deserialize the Params

  FeatSelParams feat_sel_params;
  log_info("[lime_feat_sel]: params_str = " + params_str);
  std::string feat_sel_params_str = base64_decode_to_pb_string(params_str);
  deserialize_feat_sel_params(feat_sel_params, feat_sel_params_str);
  log_info("Deserialize the lime feature_selection params");
  log_info("[lime_feat_sel] num_samples = " + std::to_string(feat_sel_params.num_samples));
  log_info("[lime_feat_sel] feature_selection = " + feat_sel_params.feature_selection);
  // std::string path_prefix = "/opt/falcon/exps/breast_cancer/client" + std::to_string(party.party_id);
  feat_sel_params.selected_features_file = output_path_prefix + feat_sel_params.selected_features_file;

  FeatSel feat_sel;
  feat_sel.select_features(
      party,
      feat_sel_params.num_samples,
      feat_sel_params.feature_selection,
      feat_sel_params.selected_features_file,
      ps_network_str,
      is_distributed,
      distributed_role,
      worker_id);
}


// test
std::vector<std::vector<int>> find_party_feat_index(
    std::vector< EncodedNumber* > global_score,
    int topK){

  // construct result
  std::vector<std::vector<int>> party_selected_feat_idx;

  for (int i = 0; i < global_score.size(); i++) {
    std::vector<int> feat_idx;
    for (int j = 0; j < global_score[i].size(); j++ ){
      feat_idx.push_back(j);
    }
    party_selected_feat_idx.push_back(feat_idx);
  }

  return party_selected_feat_idx;
}
