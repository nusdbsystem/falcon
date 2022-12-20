//
// Created by root on 20/12/22.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_PS_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_PS_H_

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/pb_converter/common_converter.h>

class WeightedPearsonPS : public ParameterServer {

 public:
  Party party;
 public:
  WeightedPearsonPS() = default;
  WeightedPearsonPS(const Party &m_party, const std::string &ps_network_config_pb_str);
  WeightedPearsonPS(const WeightedPearsonPS &obj);
  ~WeightedPearsonPS();

 public:

  void distributed_train() override;

  void distributed_eval(falcon::DatasetType eval_type, const std::string &report_save_path) override;

  void distributed_predict(const std::vector<int> &cur_test_data_indexes, EncodedNumber *predicted_labels) override;

  void save_model(const std::string &model_save_file) override;

 public:

  std::vector<int> partition_examples(std::vector<int> batch_indexes);

};

#endif //FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_PS_H_
