## Algorithms

We mainly focus on two FL settings: vertical FL and horizontal FL.

### Vertical FL

In vertical FL, we support pre-processing and model training algorithms, including:
 * PSI: sample alignment
 * WOE & IV: pre-processing for selecting features
 * LR: linear models [logistic regression](vertical/linear_model/README.md)
 * DT: tree-based models
 * RF & GBDT: ensemble models
 * NN: neural networks (will be supported in the future)

### Horizontal FL

In horizontal FL, we support:
 * LR: linear models
 * NN: neural networks (based on [SINGA](https://singa.apache.org/) framework)
