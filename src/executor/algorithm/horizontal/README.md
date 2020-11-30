## Horizontal FL

The parties have the disjoint samples but with the same features. 

In the training stage of this setting, each party is able to train a model at 
local. The goal is to combine the model parameters of all parties to obtain a 
more accurate model over the global dataset.

In the serving stage of this setting, each party has all the features of an input
sample and the trained model. Therefore, each party can provide serving service
by her own.
