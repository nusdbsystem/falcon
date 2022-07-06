# Sample Datasets Used in Falcon Platform

## How Falcon Uses the Dataset

### Split-train-test

Executor split the train/test in `src/executor/party/party.cc`:
```c++
void Party::split_train_test_data(float split_percentage,
...
  // if the party is active party, shuffle the data and labels,
  // and send the shuffled indexes to passive parties for splitting accordingly
...
    // send the data_indexes to passive parties
```

The `src/executor/algorithm/vertical/linear_model/logistic_regression.cc` splits by a ratio of 80-20:

```cpp
  float split_percentage = 0.8;
  party.split_train_test_data(split_percentage,
      training_data,
      testing_data,
      training_labels,
      testing_labels);
```

## Logistic Regression: UCI Tele-marketing Bank Dataset

Here we introduce the logistic regression (LR) model, similar to the linear regression model.
In the vertical FL setting, the features are distributed among parties
while the label is held by only the active party. We use a hybrid of MPC and PHE 
to train the LR model.

More details on the LR model training and setup can be found at `src/executor/algorithm/vertical/linear_model/README.md`

### UCI Bank Marketing Data Set Details

URL of the data set at https://archive.ics.uci.edu/ml/datasets/Bank+Marketing

It is a dataset from a Portuguese bank, where they used machine learning for tele-marketing. The classification goal is a binary Yes (0) or No (1) of whether a client will subscribe a term deposit after the tele-marketing.

The bank marketing data contains 20 feature inputs, 10 are numeric features and 10 are categorical features.

### Data Preparation

The data set values are normalized with `tools/data_preparation/data_normalization.cc`, the output of the normalized is in `data/dataset/bank_marketing_data/bank.data.norm`

The training data consists of 3616 rows, while the testing data consists of 905 rows.


### Split Features for Vertical FL

The feature inputs (attributes) are split into clients, for vertial federated learning, using `tools/data_preparation/data_spliter.cc`

For example, the 0th row of the bank data:
```py
# csv schema:
"age";"job";"marital";"education";"default";"balance";"housing";"loan";"contact";"day";"month";"duration";"campaign";"pdays";"previous";"poutcome";"y"
# original csv data:
30;"unemployed";"married";"primary";"no";1787;"no";"no";"cellular";19;"oct";79;1;-1;0;"unknown";"no"
# original data preprocessed:
30,1,1,1,1,1787,1,1,1,19,10,79,1,-1,0,0,1
# original data preprocessed and normalized:
0.161777,0.090992,0.500250,0.333555,1.000000,0.068455,1.000000,1.000000,0.500250,0.600013,0.818198,0.024827,0.000020,0.000001,0.000040,0.000333,1.000000
```

Q: why does client-0 have 7 features, but client-1 and client-2 only have 5 features each?

A: **client-0 gets the LAST 7-column part of the row:**
```py
0.818198,0.024827,0.000020,0.000001,0.000040,0.000333,1.000000
```

**client-1 gets the middle 5-column part of the row:**
```py
0.068455,1.000000,1.000000,0.500250,0.600013
```

**client-2 gets the first 5-column part of the row:**
```py
0.161777,0.090992,0.500250,0.333555,1.000000
```


For **active party only (client-0)**, the last column is labels (Yes=0 or No=1)


## Logistic Regression: UCI Breast Cancer Dataset

From Sklearn's datasets, which is taken from UCI ML dataset for breast cancer binary classification (https://archive.ics.uci.edu/ml/datasets/Breast+Cancer+Wisconsin+(Diagnostic)).

> Features are computed from a digitized image of a fine needle aspirate (FNA) of a breast mass. They describe characteristics of the cell nuclei present in the image.
There are 30 real-value features, 10 for each of the 3 types of cell nuclei.

All feature values are recoded with four significant digits.

Missing attribute values: none

Class distribution: 357 benign, 212 malignant

**positive class (`y==1`) is Benign, negative class (`y==0`) is Maglignant**

```py
# Class Distribution: 212 - Malignant, 357 - Benign
np.testing.assert_equal(np.count_nonzero(y==1), 357)
# benign class is 1
np.testing.assert_equal(np.count_nonzero(y==0), 212)
# malignant class is 0
```

Split 10 features into each client 0,1,2, with client 0 carrying the labels.


## Logistic Regression: UCI Census Income Dataset

Census income dataset: https://archive.ics.uci.edu/ml/datasets/census+income

Abstract: Predict whether income exceeds $50K/yr based on census data. Also known as the "Adult" dataset.

### Data Set Information:

Extraction was done by Barry Becker from the 1994 Census database. A set of reasonably clean records was extracted using the following conditions: ((AAGE>16) && (AGI>100) && (AFNLWGT>1)&& (HRSWK>0))

The prediction task is to determine whether a person makes over 50K a year.