//
// Created by wuyuncheng on 31/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_BUILDER_H_

#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_model.h>

// number of estimators (note that the number of total trees in the model
// does not necessarily equal to the number of estimators for classification)


struct GbdtParams {
  // number of estimators (note that the number of total trees in the model
  // does not necessarily equal to the number of estimators for classification)
  int n_estimator;
  // loss function to be optimized
  std::string loss;
  // learning rate shrinks the contribution of each tree
  double learning_rate;
  // the fraction of samples to be used for fitting individual base learners
  // default 1.0, reserved here for future usage
  double subsample;
  // decision tree builder params
  DecisionTreeParams dt_param;
};

class GbdtBuilder : public ModelBuilder {
 public:
  // number of estimators (note that the number of total trees in the model
  // does not necessarily equal to the number of estimators for classification)
  int n_estimator;
  // loss function to be optimized
  std::string loss;
  // learning rate shrinks the contribution of each tree
  double learning_rate;
  // the fraction of samples to be used for fitting individual base learners
  // default 1.0, reserved here for future usage
  double subsample;
  // decision tree builder params
  DecisionTreeParams dt_param;

 public:
  // local feature num in the training dataset
  int local_feature_num;
  // tree builders of each tree in the gbdt builder
  std::vector<DecisionTreeBuilder> tree_builders;
  // gbdt model
  GbdtModel gbdt_model;

 public:
  /** default constructor */
  GbdtBuilder();

  /** default destructor */
  ~GbdtBuilder();

  /** constructor */
  GbdtBuilder(GbdtParams gbdt_params,
              std::vector< std::vector<double> > m_training_data,
              std::vector< std::vector<double> > m_testing_data,
              std::vector<double> m_training_labels,
              std::vector<double> m_testing_labels,
              double m_training_accuracy = 0.0,
              double m_testing_accuracy = 0.0);
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_BUILDER_H_
