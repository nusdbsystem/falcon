// code modified from https://github.com/coding-ai/machine_learning_cpp
// YouTube tutorial by coding-ai at
// https://www.youtube.com/playlist?list=PLNpKaH98va-FJ1YN8oyMQWnR1pKzPu-GI

#include "ETL/ETL.h"

#include <iostream>
#include <string>
#include <Eigen/Dense>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include <vector>


using namespace std;

// main program reading cmd arguments
int main(int argc, char* argv[]) {
    cout << "ETL program called\n";
    if (argc != 4) {
        cout << "[Usage] dataset delimiter use_header\n";
        return EXIT_FAILURE;
    }
    string dataset = argv[1];
    string delimiter = argv[2];
    // bool header = argv[3];
    cout << "dataset = " << argv[1] << endl;
    cout << "delimiter = " << argv[2] << endl;
    cout << "use_header = " << argv[3] << endl;

    // create ETL instance
    ETL etl(dataset, delimiter, argv[3]);

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
        dataMat
    );

    cout << "norm_z is\n" << norm_z << endl;

    return EXIT_SUCCESS;
}