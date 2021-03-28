cmake_minimum_required(VERSION 3.10)

# set the project name
project(ETL)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# -g option gives more information
# -fno-omit-frame-pointer gets nicer stack traces in error messages
# -fno-common to make global variables protected by ASAN
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fsanitize=address -g -fno-common -fno-omit-frame-pointer")

# Automatically add the current source
# and build directories to the include path
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# using eigen in cmake
# Eigen is a header only library
# and does not need to be compiled,
# just included
include_directories(
    ./eigen-3.3.9
)

# manually add the sources
set(LINREG_SOURCES
    ./ETL/ETL.cpp
    ./LinearRegression/LinearRegression.cpp
    ./main/LinearRegression.cpp
)
set(LOGREG_SOURCES
    ./ETL/ETL.cpp
    ./LogisticRegression/LogisticRegression.cpp
    ./main/LogisticRegression.cpp
)

# Make sure to add the CMAKE_CXX_STANDARD declarations
# above the call to add_executable

# add the executable
# 1st param: name of the executable
# 2nd param: source file from which to build the executable
add_executable(LinReg ${LINREG_SOURCES})
add_executable(LogReg ${LOGREG_SOURCES})
