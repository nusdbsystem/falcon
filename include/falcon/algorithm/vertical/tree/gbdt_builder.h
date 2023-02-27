//
// Created by wuyuncheng on 31/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_BUILDER_H_

#include <falcon/algorithm/vertical/tree/gbdt_model.h>
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/common.h>

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
  GbdtBuilder(const GbdtParams &gbdt_params,
              std::vector<std::vector<double>> m_training_data,
              std::vector<std::vector<double>> m_testing_data,
              std::vector<double> m_training_labels,
              std::vector<double> m_testing_labels,
              double m_training_accuracy = 0.0,
              double m_testing_accuracy = 0.0);

  /**
   * build each tree of gbdt model
   *
   * @param party
   */
  void train(Party party) override;

  /**
   * build the decision tree model
   * @param party
   */
  void distributed_train(const Party &party, const Worker &worker) override;

  /**
   * train gbdt regression task
   *
   * @param party
   */
  void train_regression_task(Party party);

  /**
   * train gbdt classification task
   *
   * @param party
   */
  void train_classification_task(Party party);

  /**
   * compute the encrypted square residual via spdz
   *
   * @param party: the participating party
   * @param residuals: the encrypted residual vector
   * @param squared_residuals: the returned encrypted residual square vector
   * @param size: the size of the residual vector
   * @param phe_precision: the precision of each element in encrypted residual
   */
  void square_encrypted_residual(Party party, EncodedNumber *residuals,
                                 EncodedNumber *squared_residuals, int size,
                                 int phe_precision);

  /**
   * evaluate the accuracy on the dataset
   * @param party
   * @param eval_type
   * @param report_save_path
   */
  void eval(Party party, falcon::DatasetType eval_type,
            const std::string &report_save_path = std::string()) override;
};

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_GBDT_BUILDER_H_
