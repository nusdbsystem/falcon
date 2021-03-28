#ifndef LinearRegression_h
#define LinearRegression_h

#include <Eigen/Dense>
#include <vector>


using namespace std;

class LinearRegression
{
public:
    // constructor
    LinearRegression()
    {}

    // Ordinary Least Square Cost function
    float OLS_Cost(
        Eigen::MatrixXd X,
        Eigen::MatrixXd y,
        Eigen::MatrixXd theta
    );

    // gradient descent for cost function OLS
    tuple<Eigen::VectorXd, vector<float>> GradientDescent(
        Eigen::MatrixXd X,
        Eigen::MatrixXd y,
        Eigen::MatrixXd theta,
        float learning_rate,
        int iters
    );

};

#endif
