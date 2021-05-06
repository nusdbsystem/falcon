#include <gtest/gtest.h>
#include <vector>
#include <iostream>
#include <falcon/utils/math/math_ops.h>


// logistic function is a sigmoid function (S-shaped)
// logistic function outputs an estimated probability between 0 and 1
TEST(Math_ops, LogisticFunction) {
    double abs_error = 0.01;
    double logit, est_prob;
    logit = 10.0;
    est_prob = logistic_function(logit);
    EXPECT_GT(est_prob, 0.99);

    logit = 1.0;
    est_prob = logistic_function(logit);
    EXPECT_NEAR(est_prob, 0.731058, abs_error);

    logit = 0.0;
    est_prob = logistic_function(logit);
    EXPECT_NEAR(est_prob, 0.5, abs_error);

    logit = -10.0;
    est_prob = logistic_function(logit);
    EXPECT_LT(est_prob, 0.01);
}

// Logistic Regressrion Loss calculates the loss of a binary LR model
// the model estimates high probabilities for positive instances (y = 1)
// and low probabilities for negative instances (y = 0)
// for a single instance x, estimated positive probability = p
// if y=1, the closer p is to 1 the better, cost := -log(p)
// if y=0, the closer p is to 0 the better, cost := -log(1-p)
// The cost function over the whole training set is
// the average cost over all training instances
TEST(Math_ops, LogisticRegressionLoss) {
    std::vector<double> pred_probs, labels;
    double cost;

    // ground truth = [1,1,0]
    labels.push_back(1);
    labels.push_back(1);
    labels.push_back(0);

    // predicted probabilities = [.9, .5, .4]
    pred_probs.push_back(0.9);
    pred_probs.push_back(0.5);
    pred_probs.push_back(0.4);

    cost = logistic_regression_loss(pred_probs, labels);
    // ln(0.9) + ln(0.5) + ln(0.6) = -1.3093
    // NOTE: FLOAT_EQ is the two values are within 4 ULP's from each other.
    // with ULP is stand for Unit in the last places
    EXPECT_FLOAT_EQ(cost, 0.43644444);

    // all correct predictions
    pred_probs.assign(labels.begin(), labels.end());
    // for (float i : pred_probs) {
    //     std::cout << i << " ";
    // }
    // for (float i : labels) {
    //     std::cout << i << " ";
    // }
    // std::cout << std::endl;
    cost = logistic_regression_loss(pred_probs, labels);
    // ln(1) + ln(1) + ln(1) = 0
    // EXPECT_FLOAT_EQ(cost, 0);
    // NOTE: anything more precise than 0.00001 will fail...
    EXPECT_NEAR(cost, 0, 0.0001);

    // all very close to correct predictions
    pred_probs[0] = 0.99;
    pred_probs[1] = 0.98;
    pred_probs[2] = 0.01;
    // for (float i : pred_probs) {
    //     std::cout << i << " ";
    // }
    // for (float i : labels) {
    //     std::cout << i << " ";
    // }
    // std::cout << std::endl;
    cost = logistic_regression_loss(pred_probs, labels);
    // ln(0.99) + ln(0.98) + ln(0.99) = 0.0403
    // EXPECT_FLOAT_EQ(cost, 0);
    // NOTE: anything more precise than 0.00001 will fail...
    EXPECT_NEAR(cost, 0.0134, 0.0001);

    // test cases from Speech and Language Processing
    // by Dan Jurafsky
    // https://web.stanford.edu/~jurafsky/slp3/5.pdf
    pred_probs.resize(1);
    labels.resize(1);
    pred_probs[0] = 0.69;
    // y is 1
    labels[0] = 1;
    cost = logistic_regression_loss(pred_probs, labels);
    EXPECT_FLOAT_EQ(cost, 0.37106368);
    // y is 0
    labels[0] = 0;
    cost = logistic_regression_loss(pred_probs, labels);
    EXPECT_FLOAT_EQ(cost, 1.171183);
}
