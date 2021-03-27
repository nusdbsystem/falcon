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