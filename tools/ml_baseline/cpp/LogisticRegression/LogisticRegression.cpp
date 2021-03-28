#include "LogisticRegression.h"

#include <Eigen/Dense>
#include <iostream>
#include <map>
#include <list>


// sigmoid or logistic function
Eigen::MatrixXd LogisticRegression::Sigmoid(Eigen::MatrixXd Z) {
    // Z is the logit
    // Z = weight * x + bias
    return 1/(1+(-Z.array()).exp());
}

// propagate
// computes the cost and gradient of the propagation
std::tuple<Eigen::MatrixXd, double, double> LogisticRegression::Propagate(
            Eigen::MatrixXd Weights,
            double bias,
            Eigen::MatrixXd X,
            Eigen::MatrixXd y,
            double lambda
        ) {
    int m = y.rows();

    Eigen::MatrixXd Z = (Weights.transpose() * X.transpose()).array() + bias;

    Eigen::MatrixXd logit = Sigmoid(Z);

    auto cross_entropy_total = -(y.transpose() * (Eigen::VectorXd) logit.array().log().transpose() + ((Eigen::VectorXd) (1 - y.array())).transpose() * (Eigen::VectorXd) (1-logit.array()).log().transpose());
    auto cross_entropy = cross_entropy_total / m;

    // optional regularization term to prevent overfitting
    double l2_reg_cost = Weights.array().pow(2).sum() * (lambda/(2*m));

    double cost = static_cast<const double> (cross_entropy.array()[0]) + l2_reg_cost;

    Eigen::MatrixXd dw = (Eigen::MatrixXd)(((Eigen::MatrixXd)(logit-y.transpose()) * X)/m) + ((Eigen::MatrixXd)(lambda/m*Weights)).transpose();

    double db = (logit-y.transpose()).array().sum()/m;

    return std::make_tuple(dw,db,cost);
}

