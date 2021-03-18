#include <cmath>
#include <falcon/utils/metric/classification.h>

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <vector>
#include <iostream>


TEST(metric_accuracy, ClassificationMetrics) {
    std::vector<float> labels;
    std::vector<int> predictions;

    // ground truth = [1,1,0]
    labels.push_back(1);
    labels.push_back(1);
    labels.push_back(0);

    // predicted probabilities = [1,1,1]
    predictions.push_back(1);
    predictions.push_back(1);
    predictions.push_back(1);

    ClassificationMetrics ClfMetrics(predictions, labels);
    std::cout << "FN = " << ClfMetrics.FN << std::endl;
    EXPECT_GT(0.2, 0.99);
}
