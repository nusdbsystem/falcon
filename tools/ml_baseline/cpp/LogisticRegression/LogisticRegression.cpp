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
