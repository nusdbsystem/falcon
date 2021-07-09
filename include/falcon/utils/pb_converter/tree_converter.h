//
// Created by wuyuncheng on 14/5/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_TREE_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_TREE_CONVERTER_H_

#include <vector>
#include <string>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/algorithm/vertical/tree/node.h>

/**
 * pb serialize encrypted statistics
 *
 * @param client_id
 * @param node_index
 * @param split_num
 * @param classes_num
 * @param left_sample_nums
 * @param right_sample_nums
 * @param encrypted_statistics
 * @param output_str
 */
void serialize_encrypted_statistics(int client_id,
    int node_index,
    int split_num,
    int classes_num,
    EncodedNumber* left_sample_nums,
    EncodedNumber* right_sample_nums,
    EncodedNumber ** encrypted_statistics,
    std::string & output_str);

/**
 * pb deserialize encrypted statistics
 *
 * @param client_id
 * @param node_index
 * @param split_num
 * @param classes_num
 * @param left_sample_nums
 * @param right_sample_nums
 * @param encrypted_statistics
 * @param input_str
 */
void deserialize_encrypted_statistics(int & client_id,
    int & node_index,
    int & split_num,
    int & classes_num,
    EncodedNumber * & left_sample_nums,
    EncodedNumber * & right_sample_nums,
    EncodedNumber ** & encrypted_statistics,
    std::string input_str);

/**
 * serialize update information
 *
 * @param source_party_id
 * @param best_party_id
 * @param best_feature_id
 * @param best_split_id
 * @param left_branch_impurity
 * @param right_branch_impurity
 * @param left_branch_sample_iv
 * @param right_branch_sample_iv
 * @param output_str
 */
void serialize_update_info(int source_party_id,
    int best_party_id,
    int best_feature_id,
    int best_split_id,
    EncodedNumber left_branch_impurity,
    EncodedNumber right_branch_impurity,
    EncodedNumber* left_branch_sample_iv,
    EncodedNumber *right_branch_sample_iv,
    int sample_size,
    std::string & output_str);

/**
 * deserialize update information
 *
 * @param source_client_id
 * @param best_client_id
 * @param best_feature_id
 * @param best_split_id
 * @param left_branch_impurity
 * @param right_branch_impurity
 * @param left_branch_sample_iv
 * @param right_branch_sample_iv
 * @param input_str
 */
void deserialize_update_info(int & source_client_id,
    int & best_client_id,
    int & best_feature_id,
    int & best_split_id,
    EncodedNumber & left_branch_impurity,
    EncodedNumber & right_branch_impurity,
    EncodedNumber* & left_branch_sample_iv,
    EncodedNumber* & right_branch_sample_iv,
    std::string input_str);

/**
 * serialize split nums
 *
 * @param global_split_num
 * @param client_split_nums
 * @param output_str
 */
void serialize_split_info(int global_split_num,
    std::vector<int> client_split_nums,
    std::string & output_str);

/**
 * deserialize split nums
 *
 * @param global_split_num
 * @param client_split_nums
 * @param input_str
 */
void deserialize_split_info(int & global_split_num,
    std::vector<int> & client_split_nums,
    std::string input_str);

/**
 * serialize a tree model to string
 *
 * @param tree
 * @param output_str
 */
void serialize_tree_model(TreeModel tree, std::string & output_str);

/**
 * deserialize an input string to a tree model
 *
 * @param tree
 * @param input_str
 */
void deserialize_tree_model(TreeModel& tree, std::string input_str);

/**
 * serialize a random forest model to string
 *
 * @param trees
 * @param n_estimator
 * @param output_str
 */
void serialize_random_forest_model(std::vector<TreeModel> trees, int n_estimator, std::string & output_str);

/**
 * deserialize an input string to a random forest model
 *
 * @param trees
 * @param n_estimator
 * @param input_str
 */
void deserialize_random_forest_model(std::vector<TreeModel>& trees, int& n_estimator, std::string input_str);

#endif //FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_TREE_CONVERTER_H_
