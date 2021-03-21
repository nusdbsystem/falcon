# Data Preparation

## Data Normalization

With the C++ program `tools/data_preparation/data_normalization.cc`:

```sh
g++ -o data_normalization data_normalization.cc

$ ./data_normalization /opt/falcon/data/dataset/breast_cancer_data/breast_cancer.data
number of samples: 569
number of attributes (including label): 31
outfile name: /opt/falcon/data/dataset/breast_cancer_data/breast_cancer.data.norm
write finished
```

## Data Spliter

With the C++ program `tools/data_preparation/data_spliter.cc`:

```sh
g++ -o data_spliter data_spliter.cc

$ ./data_spliter /opt/falcon/data/dataset/breast_cancer_data/breast_cancer.data 3
number of samples: 569
number of attributes (including label): 31
threshold index 2 = 10
threshold index 1 = 20
threshold index 0 = 30
```
