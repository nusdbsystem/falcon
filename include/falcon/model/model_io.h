//
// Created by wuyuncheng on 2/2/21.
//

#ifndef FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H
#define FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H

#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/algorithm/vertical/tree/node.h>

#include <glog/logging.h>
#include <falcon/algorithm/vertical/tree/forest_model.h>

/**
 * save the serialized model to the specified path
 * @param pb_model_string: the serialized proto message of the model
 * @param model_save_path: the path to save the model
 */
void save_pb_model_string(const std::string& pb_model_string,
    const std::string& model_save_path);

/**
 * load the serialized model from the specified path
 * @param saved_pb_model_string: the serialized proto message of the model
 * @param model_save_path: the path to save the model
 */
void load_pb_model_string(std::string& saved_pb_model_string,
    const std::string& model_save_path);

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
