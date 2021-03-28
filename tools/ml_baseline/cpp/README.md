# C++ Linear Models from Scratch

This directory contains C++ implementations of two linear models: Linear Regression and Logistic Regression.

The code is modified from Github repo (https://github.com/coding-ai/machine_learning_cpp).
YouTube tutorial by coding-ai at https://www.youtube.com/playlist?list=PLNpKaH98va-FJ1YN8oyMQWnR1pKzPu-GI

## Compile the Programs

Run the script to build the project with `cmake`:
```sh
bash build_linear_models.sh
```

This will create two executables `LinReg  LogReg` in `build` folder.

## Run the Executables

The command line arguments to the two models are path to the dataset, followed by the delimiter:
```sh
# Linear Regression exe
$ ./build/LinReg ../../../data/dataset/wine_data/winedata.csv ","

# Logistic Regression exe
$ ./build/LogReg ../../../data/dataset/census_adult_data/adult_data.csv ","
```
