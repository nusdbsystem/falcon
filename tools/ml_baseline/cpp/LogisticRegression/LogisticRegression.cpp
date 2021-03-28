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

// update the parameters with gradient descend
std::tuple<Eigen::MatrixXd, double, Eigen::MatrixXd, double, std::list<double>> LogisticRegression::Optimize(Eigen::MatrixXd W, double b, Eigen::MatrixXd X, Eigen::MatrixXd y, int num_iter, double learning_rate, double lambda, bool log_cost) {

    std::list<double> costsList;

    Eigen::MatrixXd dw;
    double db, cost;

    for(int i=0; i<num_iter; i++){
        std::tuple<Eigen::MatrixXd, double, double> propagate = Propagate(W, b, X, y, lambda);
        std::tie(dw, db, cost) = propagate;

        W = W - (learning_rate*dw).transpose();
        b = b - (learning_rate*db);

        // print the cost every 100 iterations
        if(i%100==0) {
            costsList.push_back(cost);
        }

        if(log_cost && i%100==0) {
            std::cout << "Cost after iteration " << i << ": " << cost << std::endl;
        }
    }

    return std::make_tuple(W,b,dw,db,costsList);
}

// predict whether the label is 1 or 0
Eigen::MatrixXd LogisticRegression::Predict(Eigen::MatrixXd W, double b, Eigen::MatrixXd X) {

    int m = X.rows();

    double threshold = 0.5;

    Eigen::MatrixXd y_pred = Eigen::VectorXd::Zero(m).transpose();

    Eigen::MatrixXd Z = (W.transpose() * X.transpose()).array() + b;
    Eigen::MatrixXd A = Sigmoid(Z);

    for(int i=0; i<A.cols(); i++) {
        if(A(0,i) <= threshold) {
            y_pred(0,i) = 0;
        } else {
            y_pred(0,i) = 1;
        }
    }

    return y_pred.transpose();
}
