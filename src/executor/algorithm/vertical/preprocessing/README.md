## Pre-Processing

In machine learning, usually we need to select those features that are useful for
the prediction. For linear models, a typical method is to use weight of evidence 
(WOE) and [information value (IV)](https://www.listendata.com/2015/03/weight-of-evidence-woe-and-information.html) 
for selecting features. 

In the vertical FL setting, the features are distributed among parties while the
label is held by only the active party. Similar to other vertical FL algorithms,
we use a hybrid of PHE and MPC method to compute WOE and IV. 

The detailed method is as follows.

The input of this algorithm includes:
 * max feature bins: to categorize continuous or categorical features
 * iv threshold: the iv of those features less than this threshold will be dropped
 * training/testing data: only the active party has the label
 * party params: for setting up http connections among them
 * MPC params: for running MPC protocol during the computations
 * Paillier public key: for exchanging encrypted values among parties
 * Paillier partial private key: for threshold decryption and conversion to secret
 shares
 
The output of this algorithm includes:
 * output paths for pre-processed training data and testing data

The main algorithm includes the following steps (may add a diagram later).
 * the active party encrypts the one-hot encoded label information and 
 broadcasts to all parties
 * for each local feature:
    * each party categorizes the feature into bins
    * each party computes encrypted statistics according to bins and the 
    encrypted label, e.g., the number of samples for each bin and each class
 * the parties jointly convert these encrypted statistics into secret shares and
 compute the iv using SPDZ protocol
 * each party drops those features that have low iv
 * each party selects the training/testing data for using in model training stage