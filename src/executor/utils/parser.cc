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
// Created by root on 12/22/21.
//

#include <falcon/utils/parser.h>

falcon::AlgorithmName parse_algorithm_name(const std::string &name) {
  falcon::AlgorithmName output = falcon::LOG_REG;
  if ("logistic_regression" == name)
    output = falcon::LOG_REG;
  if ("linear_regression" == name)
    output = falcon::LINEAR_REG;
  if ("decision_tree" == name)
    output = falcon::DT;
  if ("random_forest" == name)
    output = falcon::RF;
  if ("gbdt" == name)
    output = falcon::GBDT;
  if ("mlp" == name)
    output = falcon::MLP;
  if ("lime_sampling" == name)
    output = falcon::LIME_SAMPLING;
  if ("lime_compute_prediction" == name)
    output = falcon::LIME_COMP_PRED;
  if ("lime_compute_weights" == name)
    output = falcon::LIME_COMP_WEIGHT;
  if ("lime_feature_selection" == name)
    output = falcon::LIME_FEAT_SEL;
  if ("lime_interpret" == name)
    output = falcon::LIME_INTERPRET;
  return output;
}

falcon::SpdzMlpActivationFunc parse_mlp_act_func(const std::string &name) {
  falcon::SpdzMlpActivationFunc func = falcon::LOGISTIC;
  if ("sigmoid" == name)
    func = falcon::LOGISTIC;
  if ("relu" == name)
    func = falcon::RELU;
  if ("identity" == name)
    func = falcon::IDENTITY;
  if ("softmax" == name)
    func = falcon::SOFTMAX;
  return func;
}