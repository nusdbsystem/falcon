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


## Set Batch Size in MPC

**The batch size is hard coded into the MPC server, so to set batch size, change both in the train_job json as well as in the MPC code**.

Go to `/opt/falcon/third_party/MP-SPDZ/Programs/Source/logistic_regression.mpc`, update the batch size in the **`ARRAY_SIZE`**:
```
### set necessary parameters
ARRAY_SIZE = 8
PORT_NUM = 14000
MAX_NUM_CLIENTS = 3
MAX_NBITS = 14
NULL = -2147483648
```

Then re-compile the mpc program:
```sh
/opt/falcon/third_party/MP-SPDZ$ sudo ./compile.py Programs/Source/logistic_regression.mpc 
Default bit length: 64
Default security parameter: 40
Compiling file Programs/Source/logistic_regression.mpc
WARNING: Order of memory instructions not preserved, errors possible
Writing to /opt/falcon/third_party/MP-SPDZ/Programs/Bytecode/logistic_regression-multithread-1.bc
WARNING: Order of memory instructions not preserved, errors possible
WARNING: Order of memory instructions not preserved, errors possible
WARNING: Order of memory instructions not preserved, errors possible
WARNING: Order of memory instructions not preserved, errors possible
WARNING: Order of memory instructions not preserved, errors possible
WARNING: Order of memory instructions not preserved, errors possible
WARNING: Order of memory instructions not preserved, errors possible
WARNING: Order of memory instructions not preserved, errors possible
Going to unknown from         6992 integer triples,        29312 integer bits,          inf virtual machine rounds
Program requires:
         inf integer triples
         inf integer bits
         inf virtual machine rounds
Writing to /opt/falcon/third_party/MP-SPDZ/Programs/Schedules/logistic_regression.sch
Writing to /opt/falcon/third_party/MP-SPDZ/Programs/Bytecode/logistic_regression-0.bc
```

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

### Batch 32, LR 0.1, Weight Init with 0~1
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
| 2 | 0.390186 | 0.42481 | 0.70465 | 0.302167 | 0.149385 | 0.256717 | 0.189223 |
| 49 | 0.382782 | 0.40973 | 0.660811 | 0.306059 | 0.123733 | 0.247892 | 0.132697 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    3210        0
     Class0    406        0

# test set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    790        0
     Class0    115        0
```

### Batch 4, LR 0.2, Weight Init with 0~1

All positive predictions.

### Batch 1, LR 0.01, Weight Init with 0~1

All positive predictions.

### Batch 1, LR 0.01, Iter 2000, Weight Init with 0~1

Left running for 2000 iterations, traning set confusion matrix all positive predictions, but test set confusion matrix has increased two negative classes predictions:
```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    3210        0
     Class0    406        0

# test set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    789        1
     Class0    114        1
```

### Batch 1, LR 0.01, Weight Init with -10~10

**change random init for weights from 0~1 to -10~10**

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 9.21466 | -6.22625 | 2.57578 | 1.51133 | -5.9338 | 8.48535 | -3.77146 |
| 1 | 9.12357 | -6.16278 | 2.58164 | 1.51328 | -5.9338 | 8.48535 | -3.77146 |
| 2 | 9.03468 | -6.12664 | 2.58604 | 1.51523 | -5.9338 | 8.48535 | -3.77146 |
| 7 | 8.59865 | -5.89984 | 2.60386 | 1.52134 | -5.91671 | 8.48925 | -3.70481 |


### Batch 1, LR 0.01, Weight Init with -10~10 (with `DEBUG=1`)

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 9.82756 | 2.81664 | -3.70706 | 9.33133 | 4.16822 | 5.97419 | 2.50276 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    1        3209
     Class0    0        406
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 9.77651 | 2.85278 | -3.70194 | 9.33524 | 4.16822 | 5.97419 | 2.50276 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    2        3208
     Class0    0        406
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 9.73014 | 2.91627 | -3.6995 | 9.33719 | 4.16822 | 5.97419 | 2.50276 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    3        3207
     Class0    0        406
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 5 | 9.53107 | 3.03394 | -3.68436 | 9.34915 | 4.16822 | 5.97419 | 2.50276 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    4        3206
     Class0    0        406
```

### Batch 8, LR 0.05, Weight Init with -10~10

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 1.01779 | 2.30302 | -6.13074 | 8.95114 | 4.51005 | -9.71927 | 1.05759 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    2655        555
     Class0    333        73
```

After 1000 iterations:

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    2919        291
     Class0    345        61

# test set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    716        74
     Class0    98        17
```

### Batch 8, LR 0.1, Weight Init with -10~10

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0.993073 | -1.41551 | -0.202029 | 5.43804 | -9.86373 | -7.19927 | 5.99492 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    3143        67
     Class0    391        15
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 499 | 0.82847 | -1.86168 | -0.789742 | 5.40117 | -10.1322 | -7.32813 | 5.22167 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    2900        310
     Class0    330        76
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 999 | 0.689748 | -1.80772 | -1.33873 | 5.39463 | -10.1818 | -7.36709 | 4.84694 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    2982        228
     Class0    344        62

# test set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    730        60
     Class0    93        22
```

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

### Batch 32, LR 0.0001, Weight Init with 0~1, Data Un-Normalized
The parameters for logistic regression model:
```
params.batch_size = 32
params.max_iteration = 500
params.learning_rate = 0.0001
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 7.03428 | -0.84739 | -0.588216 | -7.29925 | -78.0561 | 0.375225 | 0.946247 | 0.276411 | 0.606979 | 0.528495 | 0.904433 |
| 1 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 7.26996e+10 | 1.86415e+11 | 1.86359e+11 | 6.00479e+10 | 2.04935e+11 | 2.68281e+10 |
| 2 | 7.03428 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -9.27175e+10 | -2.139e+11 | -3.09327e+11 | -1.07213e+11 | -2.75432e+11 | -3.12711e+09 |
| 3 | 7.03428 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -5.49756e+11 | -3.25312e+11 | -5.3642e+11 | -5.49756e+11 | -3.95e+11 | -5.49756e+11 | -5.68732e+10 |
| 398 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 |
| 498 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 |
| 499 | 4.47814 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 | 5.49756e+11 |

**These e+11 values are due to the data not normalized to 0-1**.

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

### Batch 32, LR 0.001, Weight Init with 0~1, Data Un-Normalized

Model all predict one class.

### Batch 8, LR 0.1, Weight Init with -10~10, Data Normalized (1000 iterations)

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 6.2684 | 5.30431 | -5.11779 | 3.38805 | -1.56313 | 2.03624 | -1.48285 | 2.03476 | -8.62319 | 6.8028 | -3.13318 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    0        278
     Class0    0        177
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 499 | 1.06126 | 5.53269 | -1.67568 | 3.57748 | -2.21367 | 5.40977 | -0.555947 | 2.2034 | -7.79355 | 8.83396 | -1.27898 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    148        130
     Class0    88        89
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 999 | 0.672281 |4.68279 | -1.07434 | 2.77152 | -3.15586 | 5.25516 | -0.883009 | 1.48397 | -8.78242 | 8.75854 | -0.9861 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    202        76
     Class0    64        113

# test set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    53        26
     Class0    8        27
```

### Batch 8, LR 0.1, Weight Init with -10~10, Data Normalized (3000 iterations)

**ran 3000 iterations, debug every 500 iterations**

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 3.39229 | 9.29677 | 4.11092 | 6.66367 | 3.11665 | -7.06371 | 2.24842 | 9.35231 | -0.455175 | -8.43499 | 0.819866 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    267        11
     Class0    177        0
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 499 | 0.707449 | 7.39123 | 2.33091 | 4.75423 | 1.49793 | -6.21278 | 1.14621 | 7.58821 | -2.70513 | -8.51909 | 0.827386 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    171        107
     Class0    54        123
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 999 | 0.478582 | 7.03439 | 1.56522 | 4.34924 | 1.00095 | -4.9129 | 0.828025 | 6.78155 | -3.26836 | -8.31315 | 1.11674 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    211        67
     Class0    41        136
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1499 | 0.389878 | 6.93696 | 0.968055 | 4.21786 | 0.755564 | -4.16399 | 0.614732 | 6.27741 | -3.63679 | -8.30875 | 1.24658 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    222        56
     Class0    34        143
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1999 | 0.340084 | 6.93578 | 0.524623 | 4.17326 | 0.601563 | -3.6682 | 0.415088 | 5.83238 | -3.90582 | -8.28579 | 1.26775 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    235        43
     Class0    29        148
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2499 | 0.306887 | 6.94466 | 0.136344 | 4.16066 | 0.486568 | -3.28119 | 0.354567 | 5.56056 | -4.08498 | -8.3022 | 1.33811 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    246        32
     Class0    30        147
```

| Iteration | Loss | weight 0 | weight 1 | weight 2 | weight 3 | weight 4 | weight 5 | weight 6 | weight 7 | weight 8 | weight 9 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2999 | 0.283205 | 6.99403 | -0.180983 | 4.18847 | 0.402345 | -2.99765 | 0.226676 | 5.29039 | -4.25608 | -8.314 | 1.32681 |

```
# training set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    251        27
     Class0    28        149

# test set
Confusion Matrix
               Pred
               Class1    Class0
True Class1    74        5
     Class0    3        32
```



## References

- Aurlien Gron. 2017. Hands-On Machine Learning with Scikit-Learn and TensorFlow: Concepts, Tools, and Techniques to Build Intelligent Systems (1st. ed.). O'Reilly Media, Inc.
- Coursera Machine Learning Course by Andrew Ng https://www.coursera.org/learn/machine-learning
