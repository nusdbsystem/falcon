// code modified from https://github.com/coding-ai/machine_learning_cpp
// YouTube tutorial by coding-ai at
// https://www.youtube.com/playlist?list=PLNpKaH98va-FJ1YN8oyMQWnR1pKzPu-GI

#include "ETL/ETL.h"
#include "LogisticRegression/LogisticRegression.h"

#include <iostream>
#include <string>
#include <Eigen/Dense>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <list>


using namespace std;

// main program reading cmd arguments
int main(int argc, char* argv[]) {
    cout << "LogisticRegression program called\n";
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

    cout << "Rows: " << rows << " Cols: " << cols << endl;

    Eigen::MatrixXd dataMat = etl.CSVtoEigen(
                                dataString,
                                rows,
                                cols);

    // cout << "dataMat is\n" << dataMat << endl;

    // normalizeTarget is false for classification
    Eigen::MatrixXd norm_z = etl.NormalizeZscore(
        dataMat,
        false
    );

    // cout << "norm_z is\n" << norm_z << endl;

    // train and test split
    float split_ratio = 0.8;
    Eigen::MatrixXd X_train, y_train, X_test, y_test;
    tuple<Eigen::MatrixXd,Eigen::MatrixXd,Eigen::MatrixXd,Eigen::MatrixXd> split_data = etl.TrainTestSplit(
        norm_z,
        split_ratio
    );

    // use tie to unpack the tuple
    tie(X_train, y_train, X_test, y_test) = split_data;

    // cout << "X_train is\n" << X_train << endl;
    // cout << "y_train is\n" << y_train << endl;
    // cout << "X_test is\n" << X_test << endl;
    // cout << "y_test is\n" << y_test << endl;

    // sanity check the shape of the train test split
    cout << "norm_z rows() cols() =  " << norm_z.rows() << " " << norm_z.cols() << endl;
    cout << "X_train rows() cols() =  " << X_train.rows() << " " << X_train.cols() << endl;
    cout << "y_train rows() cols() =  " << y_train.rows() << " " << y_train.cols() << endl;
    cout << "X_test rows() cols() =  " << X_test.rows() << " " << X_test.cols() << endl;
    cout << "y_test rows() cols() =  " << y_test.rows() << " " << y_test.cols() << endl;

    // try out the logistic regression
    LogisticRegression logreg;

    int dims = X_train.cols();

    // init the weights
    Eigen::MatrixXd W = Eigen::VectorXd::Zero(dims);
    // init the bias
    double b = 0.0;
    // init the lambda, for l2_reg_cost
    double lambda = 0.0;

    bool print_cost = true;

    // learning rate
    double learning_rate = 0.1;
    // number of iterations
    int num_iter = 100;

    // print the hyper-parameters
    std::cout << "learning rate = " << learning_rate << ", num_iter = " << num_iter << std::endl;

    Eigen::MatrixXd dw;
    double db;
    std::list<double> costs;
    std::tuple<Eigen::MatrixXd, double, Eigen::MatrixXd, double, std::list<double>> optimize = logreg.Optimize(
                                                                                                    W,
                                                                                                    b,
                                                                                                    X_train,
                                                                                                    y_train,
                                                                                                    num_iter,
                                                                                                    learning_rate,
                                                                                                    lambda,
                                                                                                    print_cost);

    std::tie(W,b,dw,db,costs) = optimize;

    Eigen::MatrixXd y_pred_train = logreg.Predict(W, b, X_train);
    Eigen::MatrixXd y_pred_test = logreg.Predict(W, b, X_test);

    // compare the accuracy rate
    auto train_acc = (100 - (y_pred_train - y_train).cwiseAbs().mean()*100);
    auto test_acc = (100 - (y_pred_test - y_test).cwiseAbs().mean()*100);

    cout << "Train acc = " << train_acc << endl;
    cout << "Test acc = " << test_acc << endl;

    // save the thetaOut as file
    // etl.EigentoFile(thetaOut, "thetaOut.txt");

    // cout << "cost vector is\n";
    // for (float v : cost_vec) {
    //     cout << v << endl;
    // }

    // save the cost_vec as file
    std::vector<float> cost_vec(costs.begin(), costs.end());
    etl.ExportCostVec(cost_vec, "log_reg_cost_vec.txt");

    return EXIT_SUCCESS;
}