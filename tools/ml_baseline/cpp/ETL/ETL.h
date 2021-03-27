// code modified from https://github.com/coding-ai/machine_learning_cpp
// YouTube tutorial by coding-ai at
// https://www.youtube.com/playlist?list=PLNpKaH98va-FJ1YN8oyMQWnR1pKzPu-GI

#ifndef ETL_h
#define ETL_h

#include <iostream>
#include <fstream>
#include <vector>
#include <Eigen/Dense>
// use eigen for the linear algebra

using namespace std;

// ETL class with APIs to read data from CSV file
class ETL
{
    // accept input with string
    string dataset;
    string delimiter;
    // whether have header or not
    bool header;

public:

    // constructor
    ETL(
        string dataset,
        string delimiter,
        bool header
    ) : dataset(dataset),
        delimiter(delimiter),
        header(header)
    {}

    // method to read CSV file
    vector<vector<string>> readCSV();

    // method to build the data matrix
    Eigen::MatrixXd CSVtoEigen(
        vector<vector<string>> dataString,
        int rows,
        int cols
    );

    // using z-score normalization
    // after obtaining the mean, std
    Eigen::MatrixXd NormalizeZscore(Eigen::MatrixXd dataMat);

    // two helper functions for NormalizeZscore
    // helper 1: calculate the mean
    // return the mean for each of the column
    // the mean of each of the features x
    auto Mean(Eigen::MatrixXd dataMat) -> decltype(dataMat.colwise().mean());

    // helper 2: calculate the standard deviation
    // return the standard deviation for each of the column
    // the std of each of the features x
    auto Std(Eigen::MatrixXd dataMat) -> decltype(
        (
            (dataMat.array().square().colwise().sum())
            /
            (dataMat.rows() - 1)
        ).sqrt());
};

#endif
