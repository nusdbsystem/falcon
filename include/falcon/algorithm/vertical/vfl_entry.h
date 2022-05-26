//
// Created by root on 3/12/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_VFL_ENTRY_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_VFL_ENTRY_H_

#include <falcon/common.h>
#include <falcon/party/party.h>
#include <falcon/distributed/worker.h>

/**
 * train a linear regression model
 *
 * @param party: initialized party object
 * @param params_str: LinearRegressionParams serialized string
 * @param model_save_file: saved model file
 * @param model_report_file: saved report file
 * @param is_distributed_train: 1: use distributed train
 * @param worker: worker instance, used when is_distributed_train=1
 */
void train_linear_regression(
    Party* party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train=0, Worker* worker=nullptr);

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

/**
 * train a logistic regression model
 *
 * @param party: initialized party object
 * @param params_str: LogisticRegressionParams serialized string
 * @param model_save_file: saved model file
 * @param model_report_file: saved report file
 * @param is_distributed_train: 1: use distributed train
 * @param worker: worker instance, used when is_distributed_train=1
 */
void train_logistic_regression(
    Party* party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train=0, Worker* worker=nullptr);

/**
 * run a master to help to train logistic regression
 *
 * @param party: init
 * @param params_str: LogisticRegressionParams serialized string
 * @param worker_address_str: worker's address 'ip+port'
 * @param model_save_file: the path for saving the trained model
 * @param model_report_file: the path for saving the training report
 */
void launch_log_reg_parameter_server(
    Party* party,
    const std::string& params_pb_str,
    const std::string& ps_network_config_pb_str,
    const std::string& model_save_file,
    const std::string& model_report_file);

/**
 * train a decision tree model
 * @param party: initialized party object
 * @param params: DecisionTreeBuilderParam serialized string
 * @param model_save_file: saved model file
 * @param model_report_filef: saved report file
 * @param is_distributed_train: 1: use distributed train
 * @param worker: worker instance, used when is_distributed_train=1
 */
void train_decision_tree(
    Party *party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train=0, Worker* worker=nullptr);

/**
 * run a master to help to train logistic regression
 *
 * @param party: init
 * @param params_str: LogisticRegressionParams serialized string
 * @param worker_address_str: worker's address 'ip+port'
 * @param model_save_file: the path for saving the trained model
 * @param model_report_file: the path for saving the training report
 */
void launch_dt_parameter_server(
    Party* party,
    const std::string& params_pb_str,
    const std::string& ps_network_config_pb_str,
    const std::string& model_save_file,
    const std::string& model_report_file);

/**
 * train a random forest model
 * @param party: initialized party object
 * @param params: RandomForestParam serialized string
 * @param model_save_file: saved model file
 * @param model_report_file: saved report file
 */
void train_random_forest(Party party, const std::string& params_str,
                         const std::string& model_save_file, const std::string& model_report_file);

/**
 * train a gbdt model
 * @param party: initialized party object
 * @param params: GbdtParams serialized string
 * @param model_save_file: saved model file
 * @param model_report_file: saved report file
 */
void train_gbdt(Party party, const std::string& params_str,
                const std::string& model_save_file, const std::string& model_report_file);

/**
 * train an mlp model
 *
 * @param party: initialized party object
 * @param params_str: LinearRegressionParams serialized string
 * @param model_save_file: saved model file
 * @param model_report_file: saved report file
 * @param is_distributed_train: 1: use distributed train
 * @param worker: worker instance, used when is_distributed_train=1
 */
void train_mlp(
    Party* party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train=0, Worker* worker=nullptr);

/**
 * run a master to help to train mlp
 *
 * @param party: init
 * @param params_str: LogisticRegressionParams serialized string
 * @param worker_address_str: worker's address 'ip+port'
 * @param model_save_file: the path for saving the trained model
 * @param model_report_file: the path for saving the training report
 */
void launch_mlp_parameter_server(
    Party* party,
    const std::string& params_pb_str,
    const std::string& ps_network_config_pb_str,
    const std::string& model_save_file,
    const std::string& model_report_file);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_VFL_ENTRY_H_
