# Linear model

## Logistic Regression

Here we introduce the logistic regression (LogR) model, a regression algorithm that is used for **classification** (the prediction is a discrete value).

### Overview of Logistic Regression

Logistic Regression, also called _Logit Regression_, is a model that estimates the probability of a sample being of positive class or negative class.

If the estimated probability is greater than a **threshold (usually 50%)**, then the Logistic Regression model will predict a **positive class (labelled `1`)** for that sample with features `x`, otherwise a **negative class (labelled `0`)**. We will focus on the binary logistic regression.

```
if h(x) >= 0.5, predict y=1
if h(x) < 0.5, predict y=0
```

### Estimating Probabilities

The probabilities are estimated via `logistic_function` (in `src/executor/utils/math/math_ops.cc`). Logistic function, or sigmoid function, produces probabilities between 0 and 1.

**Our classifier, the logistic function, will output values between 0 and 1.** The input to the logistic function, or _logit_, is the weighted sum of the input features.


### Interpretation of Classifier Output

The output of the logistic regression classifier is **the estimated probability that `y=1` given sample with features `x`**. For example, if `h(x)=0.7`, the interpretation of the classifier output is "70% chance of a positive class", or in other words, the estimated probability that `y` is equal to `1`, or `y` is positive:

>probability that y=1, given x, parameterized by weights

Since `y` must be either 0 or 1,
```
P(y=0|x; weights) + P(y=1|x; weights) = 1
```
The probability that `y=0` and `y=1`, given a sample with feature `x`, must add up to 1.

**Decision boundaries is determined only by the paramaters or weights (theta)**.

### Optimization Objective and Cost Function


`LOGREG_THRES` classifier threshold for decision making


In the vertical FL setting, the features are distributed among parties
while the label is held by only the active party. We use a hybrid of MPC and PHE 
to train the LR model.
         
The detailed method is as follows.

The input of this algorithm includes:
 * learning rate: default is 0.1
 * max iteration: the maximum number of iteration for the training
 * convergence threshold: the threshold for stopping the training
 * batch size: default is 128
 * evaluation metric: accuracy, ks, etc.
 * with validation: the flag for using validation during training
 * with dp: the flag for using differential privacy during training
 * training/testing data: only the active party has the label
 * party params: for setting up http connections among them
 * MPC params: for running MPC protocol during the computations
 * Paillier public key: for exchanging encrypted values among parties
 * Paillier partial private key: for threshold decryption and conversion to secret
 shares
 
The output of this algorithm includes:
 * trained model
 * model evaluation report, the metric could be accuracy, ks, etc.

The main algorithm includes the following steps (may add a diagram later).
 * each party initializes an encrypted weight vector for the features she has
 * for each iteration:
    * the active party randomly selects a batch, broadcasts the batch ids and the
    corresponding encrypted label
    * each party computes the encrypted local aggregation of each sample in the 
    batch, and sends the encrypted local aggregation to the active party
    * the active party aggregates the encrypted global aggregation of each sample 
    in the batch
    * the parties jointly convert the encrypted global aggregation as well as
    the encrypted label into secret shares, and compute logistic function and then
    the sample loss using SPDZ protocol
    * the sample loss will be converted back into encrypted loss and broadcast to
    all parties
    * each party update her encrypted local model using the encrypted loss and her
    local training data 
 * after obtaining the trained model, the parties store it to a specified path
 * then the parties jointly evaluate the model (the model evaluation is a half 
 process of the training algorithm):
    * for each sample in the testing data:
        * each party does the encrypted local aggregation and sends to active party
        * the active party aggregates the encrypted global aggregation
        * the parties jointly convert the encrypted global aggregation and the 
        encrypted ground-truth label into secret shares 
        * the parties compute the prediction and compare with the ground-truth label
        to check if it is correctly predicted using SPDZ protocol
        * the parties jointly evaluate the model using SPDZ protocol according to 
        the comparison result 
        * the evaluation result is revealed to all parties
    * store the evaluation report to a specified path


## Experiments on UCI Telemarketing Bank Dataset

With the follow settings to train on client 0:
```
fl_setting: 1
use_existing_key: 0
data_input_file: /opt/falcon/data/dataset/bank_marketing_data/client0/client.txt
key_file: /opt/falcon/data/dataset/bank_marketing_data/client0/phe_keys
algorithm_name: logistic_regression
```

Client 0 (`party type = 0, party id = 0`)'s dataset contains:
```
sample_num = 4521
feature_num = 7
Split percentage for train-test = 0.8
training_data_size = 3616
# using seed 42
data_indexes[0] = 2189
data_indexes[1] = 2105
data_indexes[2] = 1870
```

### Batch 32, LR 0.1, Weight Init with 0-1
The parameters for logistic regression model:
```
params.batch_size = 32
params.max_iteration = 50
params.learning_rate = 0.1
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0.390207 | 0.425388 | 0.707466 | 0.302098 | 0.148763 | 0.256853 | 0.190513 |
| 1 | 0.39018 | 0.423562 | 0.705654 | 0.302178 | 0.149343 | 0.257083 | 0.192161 |
| 2 | 0.390186 | 0.42481 | 0.70465 | 0.302167 | 0.149385 | 0.256717 | 
0.189223 |
| 49 | 0.382782 | 0.40973 | 0.660811 | 0.306059 | 0.123733 | 0.247892 | 0.132697 |


## Experiments on UCI Breast Cancer Dataset

With the follow settings to train on client 0:
```
fl_setting: 1
use_existing_key: 0
data_input_file: /opt/falcon/data/dataset/breast_cancer_data/client0/client.txt
key_file: /opt/falcon/data/dataset/breast_cancer_data/client0/phe_keys
algorithm_name: logistic_regression
```

Client 0 (`party type = 0, party id = 0`)'s dataset contains:
```
sample_num = 569
feature_num = 11
Split percentage for train-test = 0.8
training_data_size = 455
# using seed 42
data_indexes[0] = 63
data_indexes[1] = 65
data_indexes[2] = 378
```

### Batch 32, LR 0.0001, Weight Init with 0-1, Data Un-Normalized
The parameters for logistic regression model:
```
params.batch_size = 32
params.max_iteration = 500
params.learning_rate = 0.0001
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 7.03428 | -0.84739 | -0.588216 | -7.29925 | -78.0561 | 0.375225 | 0.946247 | 0.276411 | 0.606979 | 0.528495 | 0.904433 |
| 1 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 7.26996e+10 | 1.86415e+11 | 1.86359e+11 | 6.00479e+10 | 2.04935e+11 | 2.68281e+10 |
| 2 | 7.03428 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -9.27175e+10 | -2.139e+11 | -3.09327e+11 | -1.07213e+11 | -2.75432e+11 | -3.12711e+09 |
| 3 | 7.03428 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -3.25312e+11 | -5.3642e+11 | -5.49756e+11 | -3.95e+11 | -5.49756e+11 | -5.68732e+10 |
| 398 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 |
| 498 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 |
| 499 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 |

When `DEBUG=1` (calculate training loss for every iteration), for 500 iterations, the `Training time = 2060.51`.

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    278        0
     Class0    177        0 

# test set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    79        0
     Class0    35        0
```


## References

- Aurlien Gron. 2017. Hands-On Machine Learning with Scikit-Learn and TensorFlow: Concepts, Tools, and Techniques to Build Intelligent Systems (1st. ed.). O'Reilly Media, Inc.
- Coursera Machine Learning Course by Andrew Ng https://www.coursera.org/learn/machine-learning
