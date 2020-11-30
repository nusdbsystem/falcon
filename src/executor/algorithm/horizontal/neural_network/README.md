## Deep neural network (DNN)

For DNN models, Falcon is built on top of Singa library. In each iteration of the
training stage, each party first train using Singa at local, obtaining the weights
of this iteration. Then the parties apply the threshold Paillier cryptosystem to 
average (i.e., FedAvg method) their weights.

Specifically, all parties hold the public key and each party holds a partial secret
key such that all parties need to collaborate to decrypt an encrypted value. The 
algorithm is executed as follows.

 * The parties initialize public key and secret key of TPHE.
 * In each iteration:
    * each party i train on her own dataset at local by weights w using Singa training
    API, obtain new weights, say w_i, then she encrypts w_i, obtaining [w_i]. 
    * each party sends the encrypted weights to one party, e.g., the first party,
    for computing average.
    * the first party aggregates the weights and computes the average according to
    homomorphic properties, obtaining [w] = \sum_{i} [w_i].
    * the parties jointly decrypt [w], and each party receives w for the computation
    in the next iteration.
 * After finishing a number of iterations or epochs, the model is obtained.