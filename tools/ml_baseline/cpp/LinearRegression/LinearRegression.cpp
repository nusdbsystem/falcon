#include "LinearRegression.h"

#include <Eigen/Dense>
#include <iostream>
#include <cmath>
#include <vector>


using namespace std;

// Ordinary Least Square (OLS) cost function
float LinearRegression::OLS_Cost(
        Eigen::MatrixXd X,
        Eigen::MatrixXd y,
        Eigen::MatrixXd theta) {
    Eigen::MatrixXd inner = pow(
                                (X * theta - y).array(),
                                2
                            );
    // X.rows is the number of examples,
    // divided by 2 * X.rows() means the average loss over all examples,
    // since the cost is only use to display, it's ok to show the average loss,
    // the form of gradient in GradientDescent func is correct
    return (inner.sum() / (2 * X.rows()));
}


// gradient descent for cost function OLS
tuple<Eigen::VectorXd, vector<float>> LinearRegression::GradientDescent(
        Eigen::MatrixXd X,
        Eigen::MatrixXd y,
        Eigen::MatrixXd theta,
        float learning_rate,
        int iters) {

    // place holder for to-be-updated weights (theta)
    Eigen::MatrixXd updated_theta = theta;

    // number of parameters in theta == number of features
    int nb_params = theta.rows();

    vector<float> cost_vec;
    float initial_cost = OLS_Cost(X, y, theta);
    cost_vec.push_back(initial_cost);

    // for each iteration
    for (int i=0; i<iters; i++) {
        // error from the weights theta
        Eigen::MatrixXd error = X*theta - y;
        for (int j=0; j<nb_params; j++) {
            Eigen::MatrixXd X_i = X.col(j);
            Eigen::MatrixXd term = error.cwiseProduct(X_i);
            updated_theta(j,0) = theta(j,0) - (learning_rate/X.rows())*term.sum();
        }
        float new_cost = OLS_Cost(X, y, updated_theta);
        cost_vec.push_back(new_cost);
        // update the weights for next iteration
        theta = updated_theta;
    }

    return make_tuple(theta, cost_vec);
}

