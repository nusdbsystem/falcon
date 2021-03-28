// code modified from https://github.com/coding-ai/machine_learning_cpp
// YouTube tutorial by coding-ai at
// https://www.youtube.com/playlist?list=PLNpKaH98va-FJ1YN8oyMQWnR1pKzPu-GI

#include "ETL/ETL.h"
#include "LinearRegression/LinearRegression.h"

#include <iostream>
#include <string>
#include <Eigen/Dense>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include <vector>


using namespace std;

// main program reading cmd arguments
int main(int argc, char* argv[]) {
    cout << "LinearRegression program called\n";
    if (argc != 3) {
        cout << "[Usage] dataset delimiter\n";
        return EXIT_FAILURE;
    }
    string dataset = argv[1];
    string delimiter = argv[2];
    cout << "dataset = " << argv[1] << endl;
    cout << "delimiter = " << argv[2] << endl;

    // create ETL instance
    ETL etl(dataset, delimiter);

    // read the csv
    vector<vector<string>> dataString = etl.readCSV();

    // build the eigen matrix from csv
    int rows = dataString.size();
    int cols = dataString[0].size();

    Eigen::MatrixXd dataMat = etl.CSVtoEigen(
                                dataString,
                                rows,
                                cols);

    cout << "dataMat is\n" << dataMat << endl;

    Eigen::MatrixXd norm_z = etl.NormalizeZscore(
        dataMat,
        true
    );

    cout << "norm_z is\n" << norm_z << endl;

    // train and test split
    float split_ratio = 0.8;
    Eigen::MatrixXd X_train, y_train, X_test, y_test;
    tuple<Eigen::MatrixXd,Eigen::MatrixXd,Eigen::MatrixXd,Eigen::MatrixXd> split_data = etl.TrainTestSplit(
        norm_z,
        split_ratio
    );

    // use tie to unpack the tuple
    tie(X_train, y_train, X_test, y_test) = split_data;

    cout << "X_train is\n" << X_train << endl;
    cout << "y_train is\n" << y_train << endl;
    cout << "X_test is\n" << X_test << endl;
    cout << "y_test is\n" << y_test << endl;

    // sanity check the shape of the train test split
    cout << "norm_z rows() cols() =  " << norm_z.rows() << " " << norm_z.cols() << endl;
    cout << "X_train rows() cols() =  " << X_train.rows() << " " << X_train.cols() << endl;
    cout << "y_train rows() cols() =  " << y_train.rows() << " " << y_train.cols() << endl;
    cout << "X_test rows() cols() =  " << X_test.rows() << " " << X_test.cols() << endl;
    cout << "y_test rows() cols() =  " << y_test.rows() << " " << y_test.cols() << endl;

    // try out the linear regression
    // train test vectors?
    Eigen::VectorXd vec_train = Eigen::VectorXd::Ones(X_train.rows());
    Eigen::VectorXd vec_test = Eigen::VectorXd::Ones(X_test.rows());

    // resize the matrices to allocate the vector of ones
    X_train.conservativeResize(
        X_train.rows(), X_train.cols()+1
    );
    X_train.col(X_train.cols()-1) = vec_train;

    X_test.conservativeResize(
        X_test.rows(), X_test.cols()+1
    );
    X_test.col(X_test.cols()-1) = vec_test;

    cout << "after conservativeResize:\n";
    cout << "X_train is\n" << X_train << endl;
    cout << "y_train is\n" << y_train << endl;
    cout << "X_test is\n" << X_test << endl;
    cout << "y_test is\n" << y_test << endl;
    cout << "X_train rows() cols() =  " << X_train.rows() << " " << X_train.cols() << endl;
    cout << "y_train rows() cols() =  " << y_train.rows() << " " << y_train.cols() << endl;
    cout << "X_test rows() cols() =  " << X_test.rows() << " " << X_test.cols() << endl;
    cout << "y_test rows() cols() =  " << y_test.rows() << " " << y_test.cols() << endl;


    // init the weights
    Eigen::VectorXd theta = Eigen::VectorXd::Zero(X_train.cols());
    // learning rate
    float learning_rate = 0.01;
    // number of iterations
    int iters = 1000;

    // outputs
    Eigen::VectorXd thetaOut;
    vector<float> cost_vec;

    LinearRegression linreg;

    tuple<Eigen::VectorXd, vector<float>> gradient_descent_outputs = linreg.GradientDescent(
        X_train,
        y_train,
        theta,
        learning_rate,
        iters
    );

    tie(thetaOut, cost_vec) = gradient_descent_outputs;
    // cout << "thetaOut is\n" << thetaOut << endl;

    // save the thetaOut as file
    etl.EigentoFile(thetaOut, "thetaOut.txt");

    // cout << "cost vector is\n";
    // for (float v : cost_vec) {
    //     cout << v << endl;
    // }

    // save the cost_vec as file
    etl.ExportCostVec(cost_vec, "cost_vec.txt");

    return EXIT_SUCCESS;
}