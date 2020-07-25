## Credit Card Application Approval

This example is for vertical FL setting. The goal is to determine if a credit
card application is credible or not credible. 

 * **Dataset:** [credit card application](https://archive.ics.uci.edu/ml/datasets/default+of+credit+card+clients#).
 The dataset consists of 30000 samples with 24 features. 
 
 * **Party:** there are three parties, we vertically and equally split 
 the dataset into three sub-datasets. Only party 1 has the class label.
 The original dataset and split sub-datasets are in the data/ folder. 
 
 * **Model:** we use logistic regression (LR) model for this example.

In this example, the training pipeline includes the following stages.
 
 * **Sample alignment:** in vertical setting, finding the intersected samples
 is an indispensable stage. (We omit this stage at the beginning and will
 add it later).
 
 * **Pre-processing:** in LR model, usually we first evaluate the importance
 of each feature using information value (IV) and eliminate those uninformative
 features. (This stage calls the algorithm described in 
 executor/algorithms/vertical/preprocessing folder).
 
 * **Model training:** after finding the informative features, we train 
 a vertical LR model (described in executor/algorithms/vertical/linear_model 
 folder).
 
 * **Model serving:** after obtain the trained model, we deploy it for
 serving new requests. (In vertical FL, the features of new requests are
 distributed among parties, thus, we need the parties to collaborate for
 model serving.)

The detailed method should be added later.