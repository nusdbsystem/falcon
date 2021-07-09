//
// Created by wuyuncheng on 2/2/21.
//

#ifndef FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H
#define FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H

#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/algorithm/vertical/tree/tree.h>
#include <falcon/algorithm/vertical/tree/node.h>

#include <glog/logging.h>

/**
 * save the lr model weights after training
 *
 * @param model_weights: trained lr model weights
 * @param weight_size: size of model
 * @param model_save_path: model file to be saved
 */
void save_lr_model(EncodedNumber* model_weights, int weight_size, const std::string& model_save_path);

/**
 * load the saved lr model
 *
 * @param model_save_path: model file saved
 * @param weight_size: number of weights
 * @param saved_weights: encoded weights
 */
void load_lr_model(const std::string& model_save_path, int& weight_size, EncodedNumber* saved_weights);

/**
 * save the dt model after training
 *
 * @param tree: trained tree model
 * @param model_save_path: model file to be saved
 */
void save_dt_model(Tree tree, const std::string& model_save_path);

/**
 * load the saved lr model
 *
 * @param model_save_path: model file saved
 * @param saved_tree: saved tree model
 */
void load_dt_model(const std::string& model_save_path, Tree& saved_tree);

/**
 * save the rf model after training
 *
 * @param trees
 * @param n_estimator
 * @param model_save_path
 */
void save_rf_model(std::vector<Tree> trees, int n_estimator, const std::string& model_save_path);

/**
 * load the saved rf model
 *
 * @param model_save_path
 * @param saved_trees
 * @param n_estimator
 */
void load_rf_model(const std::string& model_save_path, std::vector<Tree>& saved_trees, int& n_estimator);

/**
 * save the training report
 *
 * @param training_accuracy: model training accuracy
 * @param testing_accuracy: model testing accuracy
 * @param report_save_path: report file to be saved
 */
void save_training_report(double training_accuracy,
    double testing_accuracy,
    const std::string& report_save_path);


#endif //FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H
