//
// Created by nailixing on 03/07/21.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_PS_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_PS_H_


#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_ps.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/party/party.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/model/model_io.h>
#include <falcon/common.h>

using namespace std;

class LogRegParameterServer : public LinearParameterServer{
 private:
  // LR model builder
  LogisticRegressionBuilder alg_builder;
//  // party
//  Party party;
 public:
  LogRegParameterServer(const LogisticRegressionBuilder& m_alg_builder,
                        const Party& m_party,
                        const std::string& ps_network_config_pb_str);

  LogRegParameterServer(const Party& m_party,
                        const std::string& ps_network_config_pb_str);

  /**
   * copy constructor
   * @param log_reg_model
   */
  LogRegParameterServer(const LogRegParameterServer& obj);

  ~LogRegParameterServer();

 public:

  /**
   * distributed training process, partition data, collect result, update weights
   */
  void distributed_train() override;

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

#endif //FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_PS_H_
