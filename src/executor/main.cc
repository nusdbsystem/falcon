/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by wuyuncheng on 13/8/20.
//

#include "falcon/algorithm/vertical/linear_model/logistic_regression_ps.h"
#include "falcon/distributed/worker.h"
#include "falcon/inference/server/inference_server.h"
#include "falcon/party/party.h"
#include "falcon/utils/base64.h"
#include <boost/program_options.hpp>
#include <chrono>
#include <exception>
#include <falcon/algorithm/vertical/preprocessing/pre_feature_selection.h>
#include <falcon/algorithm/vertical/vfl_entry.h>
#include <falcon/inference/interpretability/lime/lime.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/parser.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include <vector>

using namespace boost;
using namespace std::chrono;

void print_arguments(const boost::program_options::variables_map &vm);
void handle_eptr(std::exception_ptr eptr);

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  std::cout << "------------- Entered executor -------------" << std::endl;
  // basic arguments
  int party_id, party_num, party_type, fl_setting, use_existing_key;
  std::string network_file, log_file, data_input_file, data_output_file,
      key_file, model_save_file, model_report_file;
  std::string algorithm_name, algorithm_params;
  // for distributed training
  int is_distributed = 0;
  std::string distributed_network_file;
  int worker_id;
  // parameter server:0, worker:1
  int distributed_role = 0;
  // for serving params
  int is_inference = 0;
  std::string inference_endpoint;

  // parse the arguments
  try {
    namespace po = boost::program_options;
    po::options_description description("Usage:");
    description.add_options()("help,h", "display this help message")(
        "version,v", "display the version number")(
        "party-id", po::value<int>(&party_id), "current party id")(
        "party-num", po::value<int>(&party_num),
        "total party num")("party-type", po::value<int>(&party_type),
                           "type of this party, active or passive")(
        "fl-setting", po::value<int>(&fl_setting),
        "federated learning setting, horizontal or vertical")(
        "network-file", po::value<std::string>(&network_file),
        "file name of network configurations")(
        "log-file", po::value<std::string>(&log_file),
        "file name of log destination")(
        "data-input-file", po::value<std::string>(&data_input_file),
        "input file name of dataset")("data-output-file",
                                      po::value<std::string>(&data_output_file),
                                      "output file name of dataset")(
        "existing-key", po::value<int>(&use_existing_key),
        "whether use existing phe keys")(
        "key-file", po::value<std::string>(&key_file), "file name of phe keys")(
        "algorithm-name", po::value<std::string>(&algorithm_name),
        "algorithm to be run")
        // algorithm-params is not needed in inference stage
        ("algorithm-params", po::value<std::string>(&algorithm_params),
         "parameters for the algorithm")(
            "model-save-file", po::value<std::string>(&model_save_file),
            "model save file name")
        // model-report-file is not needed in inference stage
        ("model-report-file", po::value<std::string>(&model_report_file),
         "model report file name")("is-inference",
                                   po::value<int>(&is_inference),
                                   "whether it is an inference job")(
            "inference-endpoint", po::value<std::string>(&inference_endpoint),
            "endpoint to listen inference request")(
            "is-distributed", po::value<int>(&is_distributed),
            "is distributed")("distributed-train-network-file",
                              po::value<string>(&distributed_network_file),
                              "ps network file")(
            "worker-id", po::value<int>(&worker_id),
            "worker id")("distributed-role", po::value<int>(&distributed_role),
                         "distributed role, worker:1, parameter server:0");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(description).run(),
              vm);
    po::notify(vm);
    if (vm.count("help")) {
      std::cout << "Usage: options_description [options]\n";
      std::cout << description;
      return 0;
    }
    // init glog file given the received log file name
    FLAGS_log_dir = log_file;
    LOG(INFO) << "Init log file.";
    // print the received arguments
    print_arguments(vm);
  } catch (std::exception &e) {
    cout << e.what() << "\n";
    return 1;
  }

  // define temporary result file for comparison
  std::string tmp_res_file = data_input_file + ".phe" + std::to_string(PHE_KEY_SIZE)
      + ".prec" + std::to_string(PHE_FIXED_POINT_PRECISION);

  // execute the main logic
  std::exception_ptr eptr;
  try {
    auto start_time = high_resolution_clock::now();

    Party party(party_id, party_num, static_cast<falcon::PartyType>(party_type),
                static_cast<falcon::FLSetting>(fl_setting), data_input_file);

    log_info("Parse algorithm name and run the program");
    falcon::AlgorithmName parsed_algorithm_name =
        parse_algorithm_name(algorithm_name);
    // parsed_algorithm_name = falcon::GBDT;

    // decode the base64 string to pb string, assume that only accept config
    // string from coordinator
    std::string algorithm_params_pb_str =
        base64_decode_to_pb_string(algorithm_params);
    std::string network_config_pb_str =
        base64_decode_to_pb_string(network_file);
    std::string ps_network_config_pb_str =
        base64_decode_to_pb_string(distributed_network_file);

    // if non-distributed train or inference
    if (is_distributed == 0) {
      // for non-distributed train or inference, init the network and keys
      // directly
      party.init_network_channels(network_config_pb_str);
      party.init_phe_keys(use_existing_key, key_file);
      // centralized inference
      if (is_inference) {
        log_info("Execute inference logic");
        // invoke creating endpoint for inference requests
        if (party_type == falcon::ACTIVE_PARTY) {
          run_inference_server(inference_endpoint, model_save_file, party,
                               parsed_algorithm_name, is_distributed,
                               ps_network_config_pb_str);
        } else {
          run_passive_server(model_save_file, party, parsed_algorithm_name);
        }
      }
      // centralized training
      else {
        log_info("Execute training logic");
        switch (parsed_algorithm_name) {
        case falcon::FeatureSelection:
          pre_feat_sel(party, algorithm_params, data_output_file);
          break;
        case falcon::LOG_REG:
          train_logistic_regression(&party, algorithm_params_pb_str,
                                    model_save_file, model_report_file);
          break;
        case falcon::LINEAR_REG:
          train_linear_regression(&party, algorithm_params_pb_str,
                                  model_save_file, model_report_file);
          break;
        case falcon::DT:
          train_decision_tree(&party, algorithm_params_pb_str, model_save_file,
                              model_report_file);
          break;
        case falcon::RF:
          train_random_forest(party, algorithm_params_pb_str, model_save_file,
                              model_report_file);
          break;
        case falcon::GBDT:
          train_gbdt(party, algorithm_params, model_save_file,
                     model_report_file);
          break;
        case falcon::MLP:
          train_mlp(&party, algorithm_params_pb_str, model_save_file,
                    model_report_file);
          break;
        case falcon::LIME_SAMPLING:
          lime_sampling(party, algorithm_params, data_output_file);
          break;
        case falcon::LIME_COMP_PRED:
          // lime_conv_pred_plain2cipher(party, algorithm_params, data_output_file); // only for preparing data
          lime_comp_pred(party, algorithm_params, data_output_file);
          break;
        case falcon::LIME_COMP_WEIGHT:
          // lime_conv_pred_plain2cipher(party, algorithm_params,
          // data_output_file); // only for preparing data
          lime_comp_weight(party, algorithm_params, data_output_file, tmp_res_file);
          break;
        case falcon::LIME_FEAT_SEL:
          lime_feat_sel(party, algorithm_params, data_output_file, tmp_res_file);
          break;
        case falcon::LIME_INTERPRET:
          lime_interpret(party, algorithm_params, data_output_file, tmp_res_file);
          break;
        default:
          train_logistic_regression(&party, algorithm_params_pb_str,
                                    model_save_file, model_report_file);
          break;
        }
        log_info("-------- Finish algorithm --------");
      }
    }

    // if distributed train or inference, and distributed_role = parameter
    // server
    if (is_distributed == 1 && distributed_role == falcon::DistPS) {
      log_info("Execute as parameter server");
      if (is_inference) {
        log_info("Execute distributed inference logic");
        // if parameter server, for inference, does not init channels with
        // passive ps party.init_network_channels(network_config_pb_str);
        party.init_phe_keys(use_existing_key, key_file);
        if (party_type == falcon::ACTIVE_PARTY) {
          // only active party have parameter server, which is a mapper
          run_inference_server(inference_endpoint, model_save_file, party,
                               parsed_algorithm_name, is_distributed,
                               ps_network_config_pb_str);
        } else {
          log_info("Passive Party doesn't run parameter server in distributed "
                   "inference");
        }
      } else {
        log_info("Execute distributed training logic");
        // if parameter server, init the party itself network channels and phe
        // keys
        party.init_network_channels(network_config_pb_str);
        party.init_phe_keys(use_existing_key, key_file);
        switch (parsed_algorithm_name) {
        case falcon::LOG_REG:
          launch_log_reg_parameter_server(&party, algorithm_params_pb_str,
                                          ps_network_config_pb_str,
                                          model_save_file, model_report_file);
          break;
        case falcon::LINEAR_REG:
          launch_linear_reg_parameter_server(
              &party, algorithm_params_pb_str, ps_network_config_pb_str,
              model_save_file, model_report_file);
          break;
        case falcon::DT:
          launch_dt_parameter_server(&party, algorithm_params_pb_str,
                                     ps_network_config_pb_str, model_save_file,
                                     model_report_file);
          break;
        case falcon::RF:
          log_error("Type distributed falcon::RF not implemented");
          exit(1);
        case falcon::GBDT:
          log_error("Type distributed falcon::GBDT not implemented");
          exit(1);
        case falcon::MLP:
          launch_mlp_parameter_server(&party, algorithm_params_pb_str,
                                      ps_network_config_pb_str, model_save_file,
                                      model_report_file);
          break;
        case falcon::LIME_COMP_PRED:
          lime_comp_pred(party, algorithm_params, data_output_file,
                         ps_network_config_pb_str, is_distributed,
                         distributed_role);
          break;
        case falcon::LIME_COMP_WEIGHT:
          lime_comp_weight(party, algorithm_params, data_output_file, tmp_res_file,
                           ps_network_config_pb_str, is_distributed,
                           distributed_role);
          break;
        case falcon::LIME_FEAT_SEL:
          lime_feat_sel(party, algorithm_params, data_output_file,
                        tmp_res_file, ps_network_config_pb_str,
                        is_distributed, distributed_role);
          break;
        case falcon::LIME_INTERPRET:
          lime_interpret(party, algorithm_params, data_output_file,
                         tmp_res_file, ps_network_config_pb_str,
                         is_distributed, distributed_role);
          break;
        default:
          launch_log_reg_parameter_server(&party, algorithm_params_pb_str,
                                          ps_network_config_pb_str,
                                          model_save_file, model_report_file);
          break;
        }
      }
      log_info("Parameter Server are running");
    }

    // if distributed train or inference, and distributed_role = worker
    if (is_distributed == 1 && distributed_role == falcon::DistWorker) {
      // if distributed workers, need to init the network channels
      // but workers do not init phe keys but will receive it from ps
      party.init_network_channels(network_config_pb_str);
      // party.init_phe_keys(use_existing_key, key_file);

      if (is_inference) {
        log_info("Execute distributed inference logic");
        // worker is created to communicate with parameter server
        auto worker = new Worker(ps_network_config_pb_str, worker_id);
        if (party_type == falcon::ACTIVE_PARTY) {
          // receive batch index from ps
          while (true) {
            std::vector<double> labels;
            std::vector<std::vector<double>> probabilities;
            // receive mini batch index from parameter server
            std::string mini_batch_indexes_str;
            worker->recv_long_message_from_ps(mini_batch_indexes_str);
            std::vector<int> mini_batch_indexes;
            deserialize_int_array(mini_batch_indexes, mini_batch_indexes_str);
            auto *decrypted_labels =
                new EncodedNumber[mini_batch_indexes.size()];
            // do prediction
            run_active_server(party, model_save_file, mini_batch_indexes,
                              parsed_algorithm_name, decrypted_labels);
            // send result to parameter
            std::string decrypted_predict_label_str;
            serialize_encoded_number_array(decrypted_labels,
                                           (int)mini_batch_indexes.size(),
                                           decrypted_predict_label_str);
            worker->send_long_message_to_ps(decrypted_predict_label_str);
            delete[] decrypted_labels;
          }
        } else {
          run_passive_server(model_save_file, party, parsed_algorithm_name);
        }
        delete worker;
      } else {
        log_info("Execute distributed training logic");
        if (distributed_role == falcon::DistWorker) {
          log_info("Execute as worker");
          switch (parsed_algorithm_name) {
          case falcon::LOG_REG: {
            // worker is created to communicate with parameter server
            auto worker = new Worker(ps_network_config_pb_str, worker_id);
            train_logistic_regression(&party, algorithm_params_pb_str,
                                      model_save_file, model_report_file,
                                      is_distributed, worker);
            delete worker;
            break;
          }
          case falcon::LINEAR_REG: {
            // worker is created to communicate with parameter server
            auto worker = new Worker(ps_network_config_pb_str, worker_id);
            train_linear_regression(&party, algorithm_params_pb_str,
                                    model_save_file, model_report_file,
                                    is_distributed, worker);
            delete worker;
            break;
          }
          case falcon::DT: {
            auto worker = new Worker(ps_network_config_pb_str, worker_id);
            train_decision_tree(&party, algorithm_params_pb_str,
                                model_save_file, model_report_file,
                                is_distributed, worker);
            delete worker;
            break;
          }
          case falcon::RF:
            log_error("Type distributed falcon::RF not implemented");
            exit(1);
          case falcon::GBDT:
            log_error("Type distributed falcon::GBDT not implemented");
            exit(1);
          case falcon::MLP: {
            // worker is created to communicate with parameter server
            auto worker = new Worker(ps_network_config_pb_str, worker_id);
            train_mlp(&party, algorithm_params_pb_str, model_save_file,
                      model_report_file, is_distributed, worker);
            delete worker;
            break;
          }
          case falcon::LIME_COMP_PRED: {
            party.init_phe_keys(use_existing_key, key_file);
            lime_comp_pred(party, algorithm_params, data_output_file,
                           ps_network_config_pb_str, is_distributed,
                           distributed_role, worker_id);
            break;
          }
          case falcon::LIME_COMP_WEIGHT: {
            party.init_phe_keys(use_existing_key, key_file);
            lime_comp_weight(party, algorithm_params, data_output_file,
                             tmp_res_file, ps_network_config_pb_str,
                             is_distributed, distributed_role, worker_id);
            break;
          }
          case falcon::LIME_FEAT_SEL: {
            party.init_phe_keys(use_existing_key, key_file);
            lime_feat_sel(party, algorithm_params, data_output_file,
                          tmp_res_file, ps_network_config_pb_str,
                          is_distributed, distributed_role, worker_id);
            break;
          }
          case falcon::LIME_INTERPRET: {
            party.init_phe_keys(use_existing_key, key_file);
            lime_interpret(party, algorithm_params, data_output_file,
                           tmp_res_file, ps_network_config_pb_str,
                           is_distributed, distributed_role, worker_id);
            break;
          }
          default: {
            // worker is created to communicate with parameter server
            auto worker = new Worker(ps_network_config_pb_str, worker_id);
            train_logistic_regression(&party, algorithm_params_pb_str,
                                      model_save_file, model_report_file,
                                      is_distributed, worker);
            delete worker;
            break;
          }
          }
          log_info("Finish distributed training algorithm");
        }
        log_info("Finish distributed training algorithm");
      }
    }

    // After function call
    auto stop_time = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop_time - start_time);
    log_info("Time taken by main func: " + std::to_string(duration.count()) +
             " microseconds");
    return EXIT_SUCCESS;
  } catch (...) {
    eptr = std::current_exception(); // capture
  }
  handle_eptr(eptr);
}

void print_arguments(const boost::program_options::variables_map &vm) {
  log_info("-------- parse parameters correct --------");
  log_info("party-id: " + std::to_string(vm["party-id"].as<int>()));
  log_info("party-num: " + std::to_string(vm["party-num"].as<int>()));
  log_info("fl-setting: " + std::to_string(vm["fl-setting"].as<int>()));
  log_info("existing-key: " + std::to_string(vm["existing-key"].as<int>()));
  log_info("network-file: " + vm["network-file"].as<std::string>());
  log_info("log-file: " + vm["log-file"].as<std::string>());
  log_info("data-input-file: " + vm["data-input-file"].as<std::string>());
  log_info("data-output-file: " + vm["data-output-file"].as<std::string>());
  log_info("key-file: " + vm["key-file"].as<std::string>());
  log_info("algorithm-name: " + vm["algorithm-name"].as<std::string>());
  log_info("model-save-file: " + vm["model-save-file"].as<std::string>());
  log_info("model-report-file: " + vm["model-report-file"].as<std::string>());
  log_info("is-inference: " + std::to_string(vm["is-inference"].as<int>()));
  log_info("inference-endpoint: " + vm["inference-endpoint"].as<std::string>());
  log_info("is-distributed: " + std::to_string(vm["is-distributed"].as<int>()));
  log_info("distributed-train-network-file: " +
           vm["distributed-train-network-file"].as<std::string>());
  log_info("worker-id: " + std::to_string(vm["worker-id"].as<int>()));
  log_info("distributed-role: " +
           std::to_string(vm["distributed-role"].as<int>()));
}

// passing by value is ok
void handle_eptr(std::exception_ptr eptr) {
  try {
    if (eptr) {
      std::rethrow_exception(eptr);
    }
  } catch (const std::exception &e) {
    std::cout << "Caught exception \"" << e.what() << "\"\n";
  }
}