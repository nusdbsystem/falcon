# Linear model

Linear models (such as linear regression and logistic regression) output predictions by multiplying the parameters or weights with the input features `X`, plus the constant _bias_ or _intercept_ term.

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

`LOGREG_THRES` classifier threshold for decision making. The threshold is used to make a decision.

### Optimization Objective and Cost Function

The cost function used for logistic regression is called the _`log_loss`_, aka logistic loss or cross-entropy loss. It uses log to smoothen the cost function curve, making it a convex function. The smoothened convex cost function implies that there is a global optimum.

- Logistic regression's cost function is convex
- Convex function has one and only one global optimum
- Full batch gradient descent is guaranteed to find global optimum

### Mini-batches with Replacement

In vertical FL setting, the mini-batches used for Gradient Descent are **sampled with replacement**.


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

## Training Vertical FL in Falcon

The training stage consists of the following steps:
```
1. init encrypted local weights, "local_weights" = [w1],[w2],...[wn]
2. iterative computation
  2.1 randomly select a batch of indexes for current iteration
  2.2 compute homomorphic aggregation on the batch samples:
        eg, the party select m examples and have n features
        "local_batch_phe_aggregation"(m*1) = { [w1]x11+[w2]x12+...+[wn]x1n,
                                               [w1]x21+[w2]x22+...+[wn]x2n,
                                                        ....
                                               [w1]xm1+[w2]xm2+...+[wn]xmn,}
      2.2.1. For active party:
         a) receive batch_phe_aggregation string from other parties
         b) add the received batch_phe_aggregation with current batch_phe_aggregation
                after adding, the final batch_phe_aggregation =
                    {   [w1]x11+[w2]x12+...+[wn]x1n + [w(n+1)]x1(n+1) + [wF]x1F,
                        [w1]x21+[w2]x22+...+[wn]x2n + [w(n+1)]x2(n+1) + [wF]x2F,
                                             ...
                        [w1]xm1+[w2]xm2+...+[wn]xmn  + [w(n+1)]xm(n+1) + [wF]xmF, }
                    = { [WX1], [WX2],...,[WXm]  }
                F is total number of features. m is number of examples
          c) board-cast the final batch_phe_aggregation to other party
       2.2.2. For passive party:
          a) receive final batch_phe_aggregation string from active parties
  2.3 convert the batch "batch_phe_aggregation" to secret shares, Whole process follow Algorithm 1 in paper "Privacy Preserving Vertical Federated Learning for Tree-based Model"
    2.3.1. For active party:
       a) randomly chooses ri belongs and encrypts it as [ri]
       b) collect other party's encrypted_shares_str, eg [ri]
       c) computes [e] = [batch_phe_aggregation[i]] + [r1]+...+[rk] (k parties)
       d) board-cast the complete aggregated_shares_str to other party
       e) collaborative decrypt the aggregated shares, clients jointly decrypt [e]
       f) set secret_shares[i] = e - r1 mod q
    2.3.2. For passive party:
       a) send local encrypted_shares_str (serialized [ri]) to active party
       b) receive the aggregated shares from active party
       c) the same as active party eg,collaborative decrypt the aggregated shares, clients jointly decrypt [e]
       d) set secret_shares[i] = -ri mod q
2.4 connect to spdz parties, feed the batch_aggregation_shares and do mpc computations to get the gradient [1/(1+e^(wx))],
    which is also stored as shares.Name it as loss_shares
2.5 combine loss shares and update encrypted local weights carefully
    2.5.1. For active party:
       a) collect other party's loss shares
       c) aggregate other party's encrypted loss shares with local loss shares, and deserialize it to get [1/(1+e^(wx))]
       d) board-cast the dest_ciphers to other party
       e) calculate loss_i: [yi] + [-1/(1+e^(Wxi))] for each example i
       f) board-cast the encrypted_batch_losses_str to other party
       g) update the local weight using [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}}
    2.5.2. For passive party:
       a) send local encrypted_shares_str to active party
       b) receive the dest_ciphers from active party
       c) update the local weight using [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}}
3. decrypt weights ciphertext
```

## References

- Aurlien Gron. 2017. Hands-On Machine Learning with Scikit-Learn and TensorFlow: Concepts, Tools, and Techniques to Build Intelligent Systems (1st. ed.). O'Reilly Media, Inc.
- Coursera Machine Learning Course by Andrew Ng https://www.coursera.org/learn/machine-learning
- Speech and Language Processing (3rd ed. draft) by  Dan Jurafsky and James H. Martin https://web.stanford.edu/~jurafsky/slp3/5.pdf
