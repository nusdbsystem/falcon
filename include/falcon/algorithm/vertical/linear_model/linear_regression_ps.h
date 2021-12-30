//
// Created by root on 11/17/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_PS_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_PS_H_

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_ps.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
#include <falcon/party/party.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/model/model_io.h>
#include <falcon/common.h>

using namespace std;


class LinearRegParameterServer : public LinearParameterServer{
 public:
  // LR model builder
  LinearRegressionBuilder alg_builder;
//  // party
//  Party party;
 public:
  LinearRegParameterServer(const LinearRegressionBuilder& m_alg_builder,
                          const Party& m_party,
                          const std::string& ps_network_config_pb_str);

  LinearRegParameterServer(const Party& m_party,
                          const std::string& ps_network_config_pb_str);

  /**
   * copy constructor
   * @param linear regression parameter server
   */
  LinearRegParameterServer(const LinearRegParameterServer& obj);

  ~LinearRegParameterServer();

 public:

  /**
   * distributed training process, partition data, collect result, update weights
   */
  void distributed_train() override;

  /**
   * distributed lime train linear regression
   *
   * @param use_encrypted_labels: whether use encrypted labels during training
   * @param encrypted_true_labels: encrypted labels used
   * @param use_sample_weights: whether use encrypted sample weights
   * @param encrypted_sample_weights: encrypted sample weights
   */
  void distributed_lime_train(bool use_encrypted_labels,
                              EncodedNumber* encrypted_true_labels,
                              bool use_sample_weights,
                              EncodedNumber* encrypted_sample_weights);

  /**
   * distributed evaluation, partition data, collect result, compute evaluation matrix
   *
   * @param eval_type: train data or test data
   * @param report_save_path: the path to save evaluation matrix
   */
  void distributed_eval(falcon::DatasetType eval_type,
                        const std::string& report_save_path) override;

  /**
   * save the trained model
   *
   * @param model_save_file: vector of index
   */
  void save_model(const std::string& model_save_file) override;
};

/**
 * run a master to help to train linear regression
 *
 * @param party: init
 * @param params_str: LogisticRegressionParams serialized string
 * @param worker_address_str: worker's address 'ip+port'
 * @param model_save_file: the path for saving the trained model
 * @param model_report_file: the path for saving the training report
 */
void launch_linear_reg_parameter_server(
    Party* party,
    const std::string& params_pb_str,
    const std::string& ps_network_config_pb_str,
    const std::string& model_save_file,
    const std::string& model_report_file);


#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_PS_H_
