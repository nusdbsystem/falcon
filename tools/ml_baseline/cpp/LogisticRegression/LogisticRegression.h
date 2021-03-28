#ifndef LogisticRegression_h
#define LogisticRegression_h

#include <Eigen/Dense>
#include <list>
#include <vector>


// Logistic Regression has four main methods
// 1. Sigmoid
// 2. Propagate
// 3. Optimize
// 4. Predict
class LogisticRegression
{
public:
    LogisticRegression()
    {}

    // Sigmoid
    Eigen::MatrixXd Sigmoid(Eigen::MatrixXd Z);

    // propagate (calculate the cost)
    std::tuple<Eigen::MatrixXd, double, double> Propagate(
        Eigen::MatrixXd Weights,
        double bias,
        Eigen::MatrixXd X,
        Eigen::MatrixXd y,
        double lambda
    );
};


#endif
