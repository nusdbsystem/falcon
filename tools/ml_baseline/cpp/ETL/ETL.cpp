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
Eigen::MatrixXd ETL::CSVtoEigen(vector<vector<string>> dataString, int rows, int cols) {
    // if first row is header
    if (header == true) {
        rows--;
    }

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
