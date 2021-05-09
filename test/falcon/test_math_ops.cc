#include <gtest/gtest.h>
#include <vector>
#include <iostream>
#include <falcon/utils/math/math_ops.h>


/**
 * test cases mirrored in ml_baseline/python's
 * logreg_from_scratch/falcon/math_ops_test.py
 */

// logistic function is a sigmoid function (S-shaped)
// logistic function outputs an estimated probability between 0 and 1
TEST(Math_ops, LogisticFunction) {
    // error within 10 decimal point
    double abs_error = 1e-10;
    double logit, est_prob;

    logit = 10.0;
    est_prob = logistic_function(logit);
    EXPECT_GT(est_prob, 0.9999);

    logit = 1.0;
    est_prob = logistic_function(logit);
    EXPECT_NEAR(est_prob, 0.7310585786300049, abs_error);

    logit = 0.0;
    est_prob = logistic_function(logit);
    EXPECT_NEAR(est_prob, 0.5, abs_error);

    logit = -2.5;
    est_prob = logistic_function(logit);
    EXPECT_NEAR(est_prob, 0.07585818, abs_error);

    logit = -10.0;
    est_prob = logistic_function(logit);
    EXPECT_NEAR(est_prob, 0.0000453978, abs_error);
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
    // error within 10 decimal point
    double abs_error = 1e-10;

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
    // EXPECT_FLOAT_EQ(cost, 0.43644444);
    EXPECT_NEAR(cost, 0.43644443999458743, abs_error);

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
    // NOTE: with double, the precison increases
    // The difference between cost and 0 is 9.9920072216264148e-16
    EXPECT_NEAR(cost, 0, abs_error);

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
    // ln(0.99) + ln(0.98) + ln(0.99) = -0.0403
    // EXPECT_FLOAT_EQ(cost, 0);
    // NOTE: with double, the precison increases
    EXPECT_NEAR(cost, 0.01343445967, abs_error);

    // array size of 4
    pred_probs.resize(4);
    labels.resize(4);
    pred_probs[0] = 1;
    pred_probs[1] = 0.0;
    pred_probs[2] = 0;
    pred_probs[3] = 1.0;
    // test_ln_one
    labels.assign(pred_probs.begin(), pred_probs.end());
    cost = logistic_regression_loss(pred_probs, labels);
    // all correct prediction all ln(1), clipped to be zero
    EXPECT_NEAR(cost, 1e-10, abs_error);

    // all completely wrong predictions
    pred_probs[0] = 0;
    pred_probs[1] = 1.0;
    pred_probs[2] = 1;
    pred_probs[3] = 0.0;
    // test_ln_zero
    std::cout << std::endl;
    cost = logistic_regression_loss(pred_probs, labels);
    // all completely wrong prediction all ln(0), clipped to contain
    EXPECT_FLOAT_EQ(cost, 34.539177);
}

// more advanced logLoss test cases
TEST(Math_ops, LogisticRegressionLossAdvanced) {
    std::vector<double> pred_probs, labels;
    double cost;
    // error within 10 decimal point
    double abs_error = 1e-10;

    // test cases from Speech and Language Processing
    // by Dan Jurafsky
    // https://web.stanford.edu/~jurafsky/slp3/5.pdf
    pred_probs.resize(1);
    labels.resize(1);
    pred_probs[0] = 0.69;
    // y is 1
    labels[0] = 1;
    cost = logistic_regression_loss(pred_probs, labels);
    EXPECT_NEAR(cost, 0.37106368139083207, abs_error);
    // y is 0
    labels[0] = 0;
    cost = logistic_regression_loss(pred_probs, labels);
    EXPECT_NEAR(cost, 1.1711829815029449, abs_error);

    // example taken from sklearn
    // https://scikit-learn.org/stable/modules/generated/sklearn.metrics.log_loss.html
    pred_probs.resize(4);
    labels.resize(4);
    pred_probs[0] = 0.9;
    pred_probs[1] = 0.1;
    pred_probs[2] = 0.2;
    pred_probs[3] = 0.65;
    labels[0] = 1;
    labels[1] = 0;
    labels[2] = 0;
    labels[3] = 1;
    cost = logistic_regression_loss(pred_probs, labels);
    EXPECT_NEAR(cost, 0.21616187468057912, abs_error);

    // example taken from sklearn
    // https://scikit-learn.org/stable/modules/model_evaluation.html#log-loss
    // The first [.9, .1] in y_pred denotes
    // 90% probability that sample has label 0
    // 10% probability  that sample has label 1
    pred_probs[0] = 0.1;
    pred_probs[1] = 0.2;
    pred_probs[2] = 0.7;
    pred_probs[3] = 0.99;
    labels[0] = 0;
    labels[1] = 0;
    labels[2] = 1;
    labels[3] = 1;
    cost = logistic_regression_loss(pred_probs, labels);
    EXPECT_FLOAT_EQ(cost, 0.173807336);

    // test case taken from stackoverflow answer:
    // https://stackoverflow.com/questions/58511404/how-to-compute-binary-log-loss-per-sample-of-scikit-learn-ml-model
    // confirm that the computation above agrees with
    // the total (average) loss as returned by scikit-learn
    pred_probs.resize(8);
    labels.resize(8);
    pred_probs[0] = 0.25;
    pred_probs[1] = 0.65;
    pred_probs[2] = 0.20;
    pred_probs[3] = 0.51;
    pred_probs[4] = 0.01;
    pred_probs[5] = 0.10;
    pred_probs[6] = 0.34;
    pred_probs[7] = 0.97;
    labels[0] = 1;
    labels[1] = 0;
    labels[2] = 0;
    labels[3] = 0;
    labels[4] = 0;
    labels[5] = 0;
    labels[6] = 0;
    labels[7] = 1;
    cost = logistic_regression_loss(pred_probs, labels);
    EXPECT_NEAR(cost, 0.4917494284709932, abs_error);
}
