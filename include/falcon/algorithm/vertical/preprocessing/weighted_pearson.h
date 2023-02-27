//
// Created by naili on 21/8/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_H_

#include "falcon/algorithm/vertical/preprocessing/weighted_pearson_ps.h"
#include <Networking/ssl_sockets.h>
#include <falcon/common.h>
#include <falcon/distributed/worker.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/party/info_exchange.h>
#include <falcon/party/party.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <future>
#include <openssl/ssl.h>
#include <random>
#include <string>
#include <thread>
#include <vector>

/**
 * Convert cipher into negative cipher
 * @param phe_pub_key: user public key
 * @param cipher_value: cipher to be converted
 * @param result: converted result
 */
void convert_cipher_to_negative(djcs_t_public_key *phe_pub_key,
                                const EncodedNumber &cipher_value,
                                EncodedNumber &result);

/**
 * active party gather all parties feature number and broadcast to each party
 *
 * @param party: the participating party
 * @return global feature number over each partyID, eg,. if 1st is N, N is
 * number of features of party 0
 */
std::vector<int> sync_global_feature_number(const Party &party);

/**
 * This function return importance of features with encrypted predictions,
 * and encrypted sample_weights.
 *
 * @param party: the participating party
 * @param num_explained_features: top K features
 * @param train_data: the plaintext train data
 * @param predictions: the encrypted model predictions
 * @param sss_sample_weights: the sss sample weights
 * @param ps_network_str: the parameters of ps network string,
 * @param is_distributed: whether use distributed interpretable model training
 * @param distributed_role: if is_distributed = 1, meaningful; if 0, ps, else:
 * worker
 * @param worker_id: if is_distributed = 1 and distributed_role = 1
 * @return
 */
std::vector<int> wpcc_feature_selection(
    Party party, int num_explained_features,
    const std::vector<std::vector<double>> &train_data,
    EncodedNumber *predictions, const std::vector<double> &sss_sample_weights,
    const std::string &ps_network_str = std::string(), int is_distributed = 0,
    int distributed_role = 0, int worker_id = 0);

/**
 * get the correlation
 *
 * @param party: the participating party
 * @param train_data: the plaintext train data
 * @param predictions: the encrypted model predictions
 * @param sss_sample_weights: the sss sample weights
 * @return correlation
 */
void get_local_features_correlations(
    const Party &party, const std::vector<int> &party_feature_nums,
    const vector<std::vector<double>> &train_data, EncodedNumber *predictions,
    const vector<double> &sss_sample_weights_share,
    std::vector<double> &wpcc_vec, std::vector<int> &party_id_loop_ups,
    std::vector<int> &party_feature_id_look_ups);

/**
 * get the top K features from MPC
 *
 * @param party: the participating party
 * @param feature_cor_shares: the importance share for each feature
 * @param num_explained_features: the top K features
 * @return correlation
 */
std::vector<int>
jointly_get_top_k_features(const Party &party,
                           const std::vector<int> &party_feature_nums,
                           const std::vector<double> &feature_cor_shares,
                           const std::vector<int> &party_id_loop_ups,
                           const std::vector<int> &party_feature_id_look_ups,
                           int num_explained_features);

/**
 * weighted mean
 *
 * @param party: the participating party
 * @param weight_sum_share: the sum of weights
 * @param tmp_numerator_cipher: 1*1 array, with one
 * @param weighted_mean_cipher: result cipher
 * @param req_party_id: request party's id
 * @return weighted_mean_vector: returned result in terms of shares
 */
std::vector<double>
WeightedMean(const Party &party,
             const double &weight_sum_share, // sum of weight share
             EncodedNumber *tmp_numerator_cipher,
             EncodedNumber *weighted_mean_cipher, int req_party_id);

/**
 * compute wpcc equation using MPC
 *
 * @param party: the participating party
 * @param p_shares: p_shares
 * @param q1_shares: q1_shares
 * @param q2_shares: q2_shares
 * @return wpcc_shares: wpcc_shares
 */
std::vector<double> compute_wpcc(const Party &party,
                                 const std::vector<double> &p_shares_vec,
                                 const std::vector<double> &q1_shares_vec,
                                 double q2_shares);

/**
 * spdz computation with thread,
 * the spdz_lime_computation will do the sqrt(dist), kernel
 * exponential, and pearson coefficient operation
 *
 * @param party_num: the participating party number
 * @param party_id: the participating party id
 * @param mpc_port_bases: the mpc ports
 * @param party_host_names: the mpc hosts
 * @param public_value_size: public value size
 * @param public_values: public value known to all parties
 * @param private_value_size: private value size
 * @param private_values: private value only stored at each party, eg, local
 * share
 * @param lime_comp_type: which function in MPC is executed
 * @param res: res, in form of share
 */
void spdz_lime_computation(int party_num, int party_id,
                           std::vector<int> mpc_port_bases,
                           std::vector<std::string> party_host_names,
                           int public_value_size,
                           const std::vector<int> &public_values,
                           int private_value_size,
                           const std::vector<double> &private_values,
                           falcon::SpdzLimeCompType lime_comp_type,
                           std::promise<std::vector<double>> *res);

/**
 * PS calculate [w], [w_sum], <r>, <q2>
 * @param party
 * @param train_data
 * @param instance_partition_vec
 * @param predictions
 * @param sss_sample_weights_share
 * @param sum_sss_weight_share
 * @param two_d_e_share_vec
 * @param two_d_sss_weights_share
 */
void ps_get_wpcc_pre_info(
    const WeightedPearsonPS &ps, const vector<std::vector<double>> &train_data,
    const std::vector<std::vector<int>> &instance_partition_vec,
    EncodedNumber *predictions, const vector<double> &sss_sample_weights_share,
    std::vector<double> &sum_sss_weight_share,
    std::vector<vector<double>> &two_d_e_share_vec,
    std::vector<std::vector<double>> &two_d_sss_weights_share,
    std::vector<double> &q2_shares);

/**
 * worker calculate wpcc pre info distributedly
 * @param party
 * @param sum_sss_weight_share
 * @param instance_partition_vec
 * @param predictions
 * @param mean_y_cipher
 * @param partial_e_share_vec
 */
void worker_calculate_wpcc_batch_pre_info(
    const Party &party, int wk_index,
    const std::vector<double> &sum_sss_weight_share,
    const std::vector<std::vector<int>> &instance_partition_vec,
    EncodedNumber *predictions, EncodedNumber *mean_y_cipher,
    std::vector<double> &partial_e_share_vec);

/**
 * Worker pair calculate WPCC for all those pair's local feature.
 * @param party
 * @param train_data
 * @param party_local_tmp_wf
 * @param sum_sss_weight_share
 * @param local_encrypted_feature
 * @param two_d_e_share_vec
 * @param two_d_sss_weights_share
 * @param wpcc_vec
 * @param party_id_loop_ups
 * @param party_feature_id_look_ups
 */
void worker_calculate_wpcc_per_feature(
    const Party &party, const vector<std::vector<double>> &train_data,
    const vector<double> &sss_sample_weight_share,
    const std::vector<double> &sum_sss_weight_share,
    const std::vector<vector<double>> &two_d_e_share_vec,
    const std::vector<std::vector<double>> &two_d_sss_weights_share,
    const std::vector<double> &q2_shares, std::vector<double> &wpcc_vec,
    const std::vector<int> &global_feature_partition_ids,
    const std::vector<int> &global_partyid_look_up_vec,
    const std::vector<int> &global_party_local_feature_id_look_up_vec);

/**
 * get the correlation plaintext for verification
 *
 * @param party: the participating party
 * @param train_data: the plaintext train data
 * @param predictions: the encrypted model predictions
 * @param sss_sample_weights: the sss sample weights
 * @return correlation
 */
std::vector<double> get_local_features_correlations_plaintext(
    const Party &party, const std::vector<int> &party_feature_nums,
    const vector<std::vector<double>> &train_data, EncodedNumber *predictions,
    const vector<double> &sss_sample_weights_share,
    std::vector<double> &wpcc_vec);

/**
 * get the top K features from MPC
 *
 * @param party: the participating party
 * @param feature_cor_shares: the importance share for each feature
 * @param num_explained_features: the top K features
 * @return correlation
 */
std::vector<double> jointly_get_top_k_features_plaintext(
    const Party &party, const std::vector<int> &party_feature_nums,
    const std::vector<double> &feature_cor_shares,
    const std::vector<int> &party_id_loop_ups,
    const std::vector<int> &party_feature_id_look_ups,
    int num_explained_features);

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_H_
