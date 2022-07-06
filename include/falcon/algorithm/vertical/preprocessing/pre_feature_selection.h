//
// Created by naili on 21/6/22.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_PREPROCESSING_PEARSON_FEATURE_SELECTION_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_PREPROCESSING_PEARSON_FEATURE_SELECTION_H_


#include <falcon/party/party.h>
#include <falcon/common.h>


struct FeatSelParams {
  int num_samples;
  // feature selection method, current options are 'pearson', 'lasso_path',
  string feature_selection;
  // selected features to be saved
  string selected_features_file;
};


class FeatSel {
 public:
  FeatSel() = default;
  ~FeatSel() = default;

  /**
  * This function selects features for training, it wll call multiple algs.
  *
  * @param party: the participating party
  * @param num_samples: the number of selected samples
  * @param feature_selection: selection method, currently only support pearson
   * for output prefix + file Name
  * @param selected_features_file: the selected features file to be saved
   * for distributed training
  * @param ps_network_str: the parameters of ps network string,
  * @param is_distributed: whether use distributed interpretable model training
  * @param distributed_role: if is_distributed = 1, meaningful; if 0, ps, else: worker
  * @param worker_id: if is_distributed = 1 and distributed_role = 1
  */
  void select_features(Party party,
                       int num_samples,
                       const std::string& feature_selection,
                       const std::string& selected_features_file,
                       const std::string& ps_network_str = std::string(),
                       int is_distributed = 0,
                       int distributed_role = 0,
                       int worker_id = 0);

};

/**
 * select the features with weighted pearson algorithm (optional)
 *
 * @param party: the participating party
 * @param params_str: the algorithm params, aka. LimeFeatureSelectionParams
 * @param ps_network_str: the parameters of ps network string,
 * @param is_distributed: whether use distributed interpretable model training
 * @param distributed_role: if is_distributed = 1, meaningful; if 0, ps, else: worker
 * @param worker_id: if is_distributed = 1 and distributed_role = 1
 */
void pre_feat_sel(Party party, const std::string& params_str,
                   const std::string& ps_network_str = std::string(),
                   int is_distributed = 0,
                   int distributed_role = 0,
                   int worker_id = 0);


std::vector<std::vector<int>> find_party_feat_index(
    std::vector< EncodedNumber* > global_score,
    int topK);




#endif //FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_PREPROCESSING_PEARSON_FEATURE_SELECTION_H_
