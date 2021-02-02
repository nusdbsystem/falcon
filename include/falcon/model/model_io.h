//
// Created by wuyuncheng on 2/2/21.
//

#ifndef FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H
#define FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H

#include <falcon/algorithm/vertical/linear_model/logistic_regression.h>

#include <glog/logging.h>

void save_lr_model(LogisticRegression lr_model, const std::string& model_save_path);

void save_lr_report(LogisticRegression lr_model, const std::string& report_save_path);


#endif //FALCON_INCLUDE_FALCON_MODEL_MODEL_IO_H
