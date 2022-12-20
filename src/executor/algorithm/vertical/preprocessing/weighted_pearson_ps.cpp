//
// Created by root on 20/12/22.
//

#include "falcon/algorithm/vertical/preprocessing/weighted_pearson_ps.h"

WeightedPearsonPS::WeightedPearsonPS(const WeightedPearsonPS &obj) : party(obj.party) {}

WeightedPearsonPS::WeightedPearsonPS(
    const Party &m_party, const string &ps_network_config_pb_str) :
    ParameterServer(ps_network_config_pb_str), party(m_party) {
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  log_info("[WeightedPearsonPS::initialized]: okay.");
  djcs_t_free_public_key(phe_pub_key);
}

void WeightedPearsonPS::distributed_train() {

}
void WeightedPearsonPS::distributed_eval(falcon::DatasetType eval_type, const string &report_save_path) {

}
void WeightedPearsonPS::save_model(const string &model_save_file) {

}
void WeightedPearsonPS::distributed_predict(const vector<int> &cur_test_data_indexes, EncodedNumber *predicted_labels) {

}
