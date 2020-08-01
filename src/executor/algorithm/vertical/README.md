## Vertical FL

The parties have the same samples but with different features, only the active 
party has the label information (the label cannot be directly shared in plaintext). 

In the training stage of this setting, the parties except the active party cannot
train a local model due to no label information. Thus, for each iteration, all
the parties need to collaborate to compute the model parameters. Note that the
model parameters usually are distributed among parties, since each party has her
own feature space. 

In the serving stage of this setting, each party has the trained model and a 
subset features of an input sample. Therefore, the parties need to collaboratively
provide the serving service.
