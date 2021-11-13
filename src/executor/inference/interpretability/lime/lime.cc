//
// Created by root on 11/11/21.
//

#include <falcon/inference/interpretability/lime/lime.h>

void LimeExplainer::precompute_predictions(const Party &party,
                                           const std::vector<std::vector<double>> &selected_samples,
                                           const falcon::AlgorithmName &algorithm_name,
                                           const std::string &model_saved_file) {

}

std::vector<std::vector<double>> LimeExplainer::generate_random_samples(const Party &party,
                                                                        bool sample_around_instance,
                                                                        const std::vector<double> &data_row,
                                                                        int sample_instance_num,
                                                                        const std::string &sampling_method) {

}