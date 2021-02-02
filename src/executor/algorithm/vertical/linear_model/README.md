## Linear model

Here we introduce the logistic regression (LR) model, which is similar to the linear regression model.
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