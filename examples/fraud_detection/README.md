## Credit Card Fraud Detection

This example is for horizontal FL setting. The goal is to recognize
fraudulent credit card transactions. 

 * **Dataset:** [credit card fraud detection](https://www.kaggle.com/mlg-ulb/creditcardfraud).
 The dataset consists of 31 features, and there are 492 frauds out of 
 284807 transactions (i.e., the positive class account for 0.172% of
 all transactions). 
 
 * **Party:** there are three parties, we horizontally and equally split 
 the dataset into three sub-datasets. The original dataset and split 
 sub-datasets are in the data/ folder. 
 
 * **Model:** we refer to the model described [here](https://www.kaggle.com/currie32/predicting-fraud-with-tensorflow).
 There are four hidden layers and one output layer. The model is trained 
 using TensorFlow.

In this example, the training pipeline includes two stages.
 
 * **Pre-processing:** in horizontal FL, the pre-processing stage can be 
 computed locally, we apply the pre-processing method described in the 
 above model.
 
 * **Model training:** we use the **horizontal neural network** training, 
 presented in executor/algorithms/ folder.
 
 * **Model serving:** after obtain the trained model, we deploy it for
 serving new requests. (In horizontal FL, the requests can be handled 
 locally since the features are known by each party).

The detailed method should be added later.