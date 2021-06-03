//
// Created by wuyuncheng on 2/2/21.
//

#ifndef FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H
#define FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H

#include <falcon/algorithm/vertical/linear_model/logistic_regression.h>

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
 * save the lr training report
 *
 * @param lr_training_accuracy: model training accuracy
 * @param lr_testing_accuracy: model testing accuracy
 * @param report_save_path: report file to be saved
 */
void save_lr_report(double lr_training_accuracy,
    double lr_testing_accuracy,
    const std::string& report_save_path);


#endif //FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H
