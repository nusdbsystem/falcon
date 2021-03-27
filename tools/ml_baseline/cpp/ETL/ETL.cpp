// code modified from https://github.com/coding-ai/machine_learning_cpp
// YouTube tutorial by coding-ai at
// https://www.youtube.com/playlist?list=PLNpKaH98va-FJ1YN8oyMQWnR1pKzPu-GI

#include "ETL.h"

#include <vector>
#include <stdlib.h>
#include <cmath>
#include <boost/algorithm/string.hpp>

using namespace std;

// parse the content of the csv file line by line
// and store the parsed data in vector of vector of strings
vector<vector<string>> ETL::readCSV() {
    // use ifstream to open the file
    ifstream file(dataset);
    // to store the data string and return
    vector<vector<string>> dataString;

    // iterate each line with the delimiter
    string line = "";
    while(getline(file, line)) {
        vector<string> vec;
        boost::algorithm::split(
            vec,
            line,
            boost::is_any_of(delimiter)
        );
        dataString.push_back(vec);
    }

    // close the file
    file.close();

    // return the dataString
    return dataString;
}

// store the parsed csv string as Eigen matrix
// build the matrix based on the input data
Eigen::MatrixXd ETL::CSVtoEigen(
        vector<vector<string>> dataString, int rows, int cols) {
    // create the eigen matrix based on the rows and cols
    // variable of type MatrixXd (double) and specifies that it is a matrix
    Eigen::MatrixXd mat(cols, rows);

    for (int i=0; i<rows; i++) {
        for (int j=0; j<cols; j++) {
            // Convert a string to a floating-point number
            mat(j,i) = atof(dataString[i][j].c_str());
        }
    }

    // transpose the eigen mat, so we get rows x cols
    return mat.transpose();
}

// using z-score normalization
// after obtaining the mean, std
Eigen::MatrixXd ETL::NormalizeZscore(Eigen::MatrixXd dataMat) {
    cout << "ETL NormalizeZscore called\n";
    // cout << "dataMat is:\n" << dataMat << endl;
    // calculate the mean
    auto mean = dataMat.colwise().mean();
    // auto mean = Mean(dataMat);
    cout << "mean = " << mean << endl;
    cout << "mean.rows(), mean.cols() = " << mean.rows() << " " << mean.cols() << "\n";
    cout << "dataMat.rows(), dataMat.cols() = " << dataMat.rows() << " " << dataMat.cols() << "\n";
    // calculate the x-mean
    Eigen::MatrixXd scaled_data = dataMat.rowwise() - mean;
    cout << "scaled_data =\n" << scaled_data << endl;
    // calculate the std
    auto std = (
        (scaled_data.array().square().colwise().sum())
        /
        (scaled_data.rows() - 1)
    ).sqrt();
    // auto std = Std(scaled_data);
    cout << "std =\n" << std << endl;
    // apply the z-score normalization
    Eigen::MatrixXd norm_z = scaled_data.array().rowwise() / std;
    // cout << "norm_z\n" << norm_z << endl;
    return norm_z;
}


// two helper functions for NormalizeZscore
// helper 1: calculate the mean
// return the mean for each of the column
// the mean of each of the features x
auto ETL::Mean(Eigen::MatrixXd dataMat) -> decltype(dataMat.colwise().mean()) {
    cout << "Mean called\n";
    cout << "dataMat is:\n" << dataMat << endl;
    cout << "dataMat colwise sum:\n" << dataMat.colwise().sum() << endl;
    cout << "dataMat colwise mean:\n" << dataMat.colwise().mean() << endl;
    return dataMat.colwise().mean();
}

// helper 2: calculate the standard deviation
// return the standard deviation for each of the column
// the std of each of the features x
auto ETL::Std(Eigen::MatrixXd dataMat) -> decltype((
        (dataMat.array().square().colwise().sum())
        /
        (dataMat.rows() - 1)
    ).sqrt()) {
    // the dataMat is already the difference with the mean
    // dataMat.array() is xi - mean
    // dataMat.array().square() is (xi-mean)^2
    // dataMat.array().square().colwise().sum() is the sum of each
    // column or each feature's sum diff squared
    return (
        (dataMat.array().square().colwise().sum())
        /
        (dataMat.rows() - 1)
    ).sqrt();
}
