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
// Created by wuyuncheng on 14/11/20.
//

#include <falcon/algorithm/model_builder.h>
#include <falcon/utils/logger/logger.h>
#include <utility>

ModelBuilder::ModelBuilder() {
  training_accuracy = 0.0;
  testing_accuracy = 0.0;
}

ModelBuilder::ModelBuilder(const ModelBuilder &builder) {
  log_info("[ModelBuilder]: copy constructor");
  training_data = builder.training_data;
  training_labels = builder.training_labels;
  training_accuracy = builder.training_accuracy;
  testing_data = builder.testing_data;
  testing_labels = builder.testing_labels;
  testing_accuracy = builder.testing_accuracy;
}

ModelBuilder::ModelBuilder(std::vector<std::vector<double>> m_training_data,
                           std::vector<std::vector<double>> m_testing_data,
                           std::vector<double> m_training_labels,
                           std::vector<double> m_testing_labels,
                           double m_training_accuracy,
                           double m_testing_accuracy) {
  training_data = std::move(m_training_data);
  training_labels = std::move(m_training_labels);
  training_accuracy = m_training_accuracy;
  testing_data = std::move(m_testing_data);
  testing_labels = std::move(m_testing_labels);
  testing_accuracy = m_testing_accuracy;
}

ModelBuilder::~ModelBuilder() {}
