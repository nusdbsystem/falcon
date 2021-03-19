#include <falcon/utils/metric/classification.h>
#include <cmath>
#include <gtest/gtest.h>
#include <vector>
#include <iostream>


TEST(Classification_metrics, test_TP) {
    std::vector<int> predictions = {1,0,1,0,1};
    std::vector<float> labels    = {1,1,1,1,1};

    ClassificationMetrics ClfMetrics;
    ClfMetrics.compute_metrics(predictions, labels);
    EXPECT_EQ(ClfMetrics.TP, 3);
}

TEST(Classification_metrics, test_FP) {
    std::vector<int> predictions = {1,0,1,0,1};
    std::vector<float> labels    = {1,1,1,1,1};

    ClassificationMetrics ClfMetrics;
    ClfMetrics.compute_metrics(predictions, labels);
    EXPECT_EQ(ClfMetrics.FP, 0);
}

TEST(Classification_metrics, test_FN) {
    std::vector<int> predictions = {1, 1, 1, 0};
    std::vector<float> labels    = {0, 1, 0, 1};

    ClassificationMetrics ClfMetrics;
    ClfMetrics.compute_metrics(predictions, labels);
    EXPECT_EQ(ClfMetrics.FN, 1);
}

TEST(Classification_metrics, test_TN) {
    std::vector<int> predictions = {1, 1, 0, 0};
    std::vector<float> labels    = {0, 1, 0, 1};

    ClassificationMetrics ClfMetrics;
    ClfMetrics.compute_metrics(predictions, labels);
    EXPECT_EQ(ClfMetrics.TN, 1);
}

// TEST(Classification_metrics, test_confusion_matrix) {

// }
