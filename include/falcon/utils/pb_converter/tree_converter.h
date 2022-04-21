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
#include <falcon/algorithm/vertical/tree/forest_model.h>
#include <falcon/algorithm/vertical/tree/gbdt_model.h>

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
    const std::string& input_str);

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
 * @param tree_model
 * @param output_str
 */
void serialize_tree_model(TreeModel tree_model, std::string & output_str);

/**
 * deserialize an input string to a tree model
 *
 * @param tree_model
 * @param input_str
 */
void deserialize_tree_model(TreeModel& tree_model, std::string input_str);

/**
 * serialize a random forest model to string
 *
 * @param forest_model
 * @param output_str
 */
void serialize_random_forest_model(ForestModel forest_model, std::string & output_str);

/**
 * deserialize an input string to a random forest model
 *
 * @param forest_model
 * @param input_str
 */
void deserialize_random_forest_model(ForestModel& forest_model, std::string input_str);

/**
 * serialize a gbdt model to string
 *
 * @param gbdt_model
 * @param output_str
 */
void serialize_gbdt_model(GbdtModel gbdt_model, std::string & output_str);

/**
 * deserialize an input string to a gbdt model
 *
 * @param gbdt_model
 * @param input_str
 */
void deserialize_gbdt_model(GbdtModel& gbdt_model, std::string input_str);

#endif //FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_TREE_CONVERTER_H_
