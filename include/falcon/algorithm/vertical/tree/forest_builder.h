//
// Created by wuyuncheng on 9/6/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FOREST_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FOREST_BUILDER_H_

#include <falcon/algorithm/vertical/tree/tree_builder.h>

struct RandomForestParams {
  // number of trees in the forest
  int n_estimator;
  // sample rate for each tree in the forest
  double sample_rate;
  // decision tree builder params
  DecisionTreeParams dt_param;
};

class RandomForestBuilder : public ModelBuilder {
 public:
  // number of trees in the forest
  int n_estimator;
  // sample rate for each tree in the forest
  double sample_rate;
  // decision tree builder params
  DecisionTreeParams dt_param;

 public:
  // local feature num in the training dataset
  int local_feature_num;
  // tree builders of each tree in the forest
  std::vector<DecisionTreeBuilder> tree_builders;

 public:
  /** default constructor */
  RandomForestBuilder();

  /** destructor */
  ~RandomForestBuilder();

  /** constructor */
  RandomForestBuilder(RandomForestParams params,
    std::vector< std::vector<double> > m_training_data,
    std::vector< std::vector<double> > m_testing_data,
    std::vector<double> m_training_labels,
    std::vector<double> m_testing_labels,
    double m_training_accuracy = 0.0,
    double m_testing_accuracy = 0.0);

  /**
   * init the tree builders for the forest
   * @param party
   */
  void init_forest_builder(Party& party);

  /**
     * shuffle and assign training data to a decision tree of random forest
     *
     * @param party
     * @param tree_id
     * @param sampled_training_data: shuffle and select sample_rate * training_data_size samples
     * @param sampled_training_labels: the labels of the above sampled trainig data
     */
  void shuffle_and_assign_training_data(Party& party,
      int tree_id,
      std::vector< std::vector<double> >& sampled_training_data,
      std::vector<double>& sampled_training_labels);

  /**
   * build each tree of random forest
   *
   * @param party
   * @param sample_rate
   */
  void train(Party party);

  /**
   * evaluate the accuracy on the dataset
   * @param party
   * @param eval_type
   * @param report_save_path
*/
  void eval(Party party, falcon::DatasetType eval_type,
      const std::string& report_save_path = std::string());
};

/**
 * train a random forest model
 * @param party: initialized party object
 * @param params: RandomForestParam serialized string
 * @param model_save_file: saved model file
 * @param model_report_file: saved report file
 */
void train_random_forest(Party party, const std::string& params_str,
    const std::string& model_save_file, const std::string& model_report_file);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_FOREST_BUILDER_H_
