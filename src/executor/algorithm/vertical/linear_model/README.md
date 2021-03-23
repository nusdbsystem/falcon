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



## References

- Aurlien Gron. 2017. Hands-On Machine Learning with Scikit-Learn and TensorFlow: Concepts, Tools, and Techniques to Build Intelligent Systems (1st. ed.). O'Reilly Media, Inc.
- Coursera Machine Learning Course by Andrew Ng https://www.coursera.org/learn/machine-learning
