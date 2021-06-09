//
// Created by wuyuncheng on 3/2/21.
//

#include <vector>

#include <falcon/model/model_io.h>

#include <gtest/gtest.h>

TEST(Model_IO, SaveLRModel) {
  int weight_size = 5;
  EncodedNumber* encoded_model = new EncodedNumber[weight_size];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < weight_size; i++) {
    encoded_model[i].setter_n(v_n);
    encoded_model[i].setter_value(v_value);
    encoded_model[i].setter_exponent(v_exponent);
    encoded_model[i].setter_type(v_type);
  }

  std::string save_model_file = "saved_model.txt";
  save_lr_model(encoded_model, weight_size, save_model_file);

  EncodedNumber* loaded_model = new EncodedNumber[weight_size];
  int loaded_weight_size = 0;
  load_lr_model(save_model_file, loaded_weight_size, loaded_model);

  EXPECT_EQ(weight_size, loaded_weight_size);

  for (int i = 0; i < weight_size; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    loaded_model[i].getter_n(deserialized_n);
    loaded_model[i].getter_value(deserialized_value);
    int n_cmp = mpz_cmp(v_n, deserialized_n);
    int value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, loaded_model[i].getter_exponent());
    EXPECT_EQ(v_type, loaded_model[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
  delete [] encoded_model;
  delete [] loaded_model;
}

TEST(Model_IO, SaveDTModel) {
  Tree tree;
  tree.type = falcon::CLASSIFICATION;
  tree.class_num = 2;
  tree.max_depth = 2;
  tree.internal_node_num = 3;
  tree.total_node_num = 7;
  tree.capacity = 7;
  tree.nodes = new Node[tree.capacity];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < tree.capacity; i++) {
    if (i < 7) {
      tree.nodes[i].node_type = falcon::INTERNAL;
    } else {
      tree.nodes[i].node_type = falcon::LEAF;
    }
    tree.nodes[i].depth = 1;
    tree.nodes[i].is_self_feature = 1;
    tree.nodes[i].best_party_id = 0;
    tree.nodes[i].best_feature_id = 1;
    tree.nodes[i].best_split_id = 2;
    tree.nodes[i].split_threshold = 0.25;
    tree.nodes[i].node_sample_num = 1000;
    tree.nodes[i].node_sample_distribution.push_back(200);
    tree.nodes[i].node_sample_distribution.push_back(300);
    tree.nodes[i].node_sample_distribution.push_back(500);
    tree.nodes[i].left_child = 2 * i + 1;
    tree.nodes[i].right_child = 2 * i + 2;
    EncodedNumber impurity, label;
    impurity.setter_n(v_n);
    impurity.setter_value(v_value);
    impurity.setter_exponent(v_exponent);
    impurity.setter_type(v_type);
    label.setter_n(v_n);
    label.setter_value(v_value);
    label.setter_exponent(v_exponent);
    label.setter_type(v_type);
    tree.nodes[i].impurity = impurity;
    tree.nodes[i].label = label;
  }

  std::string save_model_file = "saved_model.txt";
  save_dt_model(tree, save_model_file);
  Tree saved_tree_model;
  load_dt_model(save_model_file, saved_tree_model);

  EXPECT_EQ(saved_tree_model.type, tree.type);
  EXPECT_EQ(saved_tree_model.class_num, tree.class_num);
  EXPECT_EQ(saved_tree_model.max_depth, tree.max_depth);
  EXPECT_EQ(saved_tree_model.internal_node_num, tree.internal_node_num);
  EXPECT_EQ(saved_tree_model.total_node_num, tree.total_node_num);
  EXPECT_EQ(saved_tree_model.capacity, tree.capacity);
  for (int i = 0; i < tree.capacity; i++) {
    EXPECT_EQ(saved_tree_model.nodes[i].node_type, tree.nodes[i].node_type);
    EXPECT_EQ(saved_tree_model.nodes[i].depth, tree.nodes[i].depth);
    EXPECT_EQ(saved_tree_model.nodes[i].is_self_feature, tree.nodes[i].is_self_feature);
    EXPECT_EQ(saved_tree_model.nodes[i].best_party_id, tree.nodes[i].best_party_id);
    EXPECT_EQ(saved_tree_model.nodes[i].best_feature_id, tree.nodes[i].best_feature_id);
    EXPECT_EQ(saved_tree_model.nodes[i].best_split_id, tree.nodes[i].best_split_id);
    EXPECT_EQ(saved_tree_model.nodes[i].split_threshold, tree.nodes[i].split_threshold);
    EXPECT_EQ(saved_tree_model.nodes[i].node_sample_num, tree.nodes[i].node_sample_num);
    EXPECT_EQ(saved_tree_model.nodes[i].left_child, tree.nodes[i].left_child);
    EXPECT_EQ(saved_tree_model.nodes[i].right_child, tree.nodes[i].right_child);
    for (int j = 0; j < tree.nodes[i].node_sample_distribution.size(); j++) {
      EXPECT_EQ(saved_tree_model.nodes[i].node_sample_distribution[j], tree.nodes[i].node_sample_distribution[j]);
    }

    EncodedNumber deserialized_impurity = saved_tree_model.nodes[i].impurity;
    EncodedNumber deserialized_label = saved_tree_model.nodes[i].label;
    mpz_t deserialized_impurity_n, deserialized_impurity_value;
    mpz_t deserialized_label_n, deserialized_label_value;
    mpz_init(deserialized_impurity_n);
    mpz_init(deserialized_impurity_value);
    mpz_init(deserialized_label_n);
    mpz_init(deserialized_label_value);

    deserialized_impurity.getter_n(deserialized_impurity_n);
    deserialized_impurity.getter_value(deserialized_impurity_value);
    int n_cmp = mpz_cmp(v_n, deserialized_impurity_n);
    int value_cmp = mpz_cmp(v_value, deserialized_impurity_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_impurity.getter_exponent());
    EXPECT_EQ(v_type, deserialized_impurity.getter_type());

    deserialized_label.getter_n(deserialized_label_n);
    deserialized_label.getter_value(deserialized_label_value);
    n_cmp = mpz_cmp(v_n, deserialized_label_n);
    value_cmp = mpz_cmp(v_value, deserialized_label_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_label.getter_exponent());
    EXPECT_EQ(v_type, deserialized_label.getter_type());

    mpz_clear(deserialized_impurity_n);
    mpz_clear(deserialized_impurity_value);
    mpz_clear(deserialized_label_n);
    mpz_clear(deserialized_label_value);
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
}

TEST(Model_IO, SaveRFModel) {
  std::vector<Tree> trees;
  int n_estimator = 3;
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int t = 0; t < n_estimator; t++) {
    Tree tree;
    tree.type = falcon::CLASSIFICATION;
    tree.class_num = 2;
    tree.max_depth = 2;
    tree.internal_node_num = 3;
    tree.total_node_num = 7;
    tree.capacity = 7;
    tree.nodes = new Node[tree.capacity];

    for (int i = 0; i < tree.capacity; i++) {
      if (i < 7) {
        tree.nodes[i].node_type = falcon::INTERNAL;
      } else {
        tree.nodes[i].node_type = falcon::LEAF;
      }
      tree.nodes[i].depth = 1;
      tree.nodes[i].is_self_feature = 1;
      tree.nodes[i].best_party_id = 0;
      tree.nodes[i].best_feature_id = 1;
      tree.nodes[i].best_split_id = 2;
      tree.nodes[i].split_threshold = 0.25;
      tree.nodes[i].node_sample_num = 1000;
      tree.nodes[i].node_sample_distribution.push_back(200);
      tree.nodes[i].node_sample_distribution.push_back(300);
      tree.nodes[i].node_sample_distribution.push_back(500);
      tree.nodes[i].left_child = 2 * i + 1;
      tree.nodes[i].right_child = 2 * i + 2;
      EncodedNumber impurity, label;
      impurity.setter_n(v_n);
      impurity.setter_value(v_value);
      impurity.setter_exponent(v_exponent);
      impurity.setter_type(v_type);
      label.setter_n(v_n);
      label.setter_value(v_value);
      label.setter_exponent(v_exponent);
      label.setter_type(v_type);
      tree.nodes[i].impurity = impurity;
      tree.nodes[i].label = label;
    }
    trees.push_back(tree);
  }

  std::string save_model_file = "saved_model.txt";
  save_rf_model(trees, n_estimator, save_model_file);
  std::vector<Tree> saved_trees;
  int deserialized_n_estimator;
  load_rf_model(save_model_file, saved_trees, deserialized_n_estimator);
  EXPECT_EQ(n_estimator, deserialized_n_estimator);
  for (int t = 0; t < n_estimator; t++) {
    Tree saved_tree_model = saved_trees[t];
    Tree tree = trees[t];
    EXPECT_EQ(saved_tree_model.type, tree.type);
    EXPECT_EQ(saved_tree_model.class_num, tree.class_num);
    EXPECT_EQ(saved_tree_model.max_depth, tree.max_depth);
    EXPECT_EQ(saved_tree_model.internal_node_num, tree.internal_node_num);
    EXPECT_EQ(saved_tree_model.total_node_num, tree.total_node_num);
    EXPECT_EQ(saved_tree_model.capacity, tree.capacity);
    for (int i = 0; i < tree.capacity; i++) {
      EXPECT_EQ(saved_tree_model.nodes[i].node_type, tree.nodes[i].node_type);
      EXPECT_EQ(saved_tree_model.nodes[i].depth, tree.nodes[i].depth);
      EXPECT_EQ(saved_tree_model.nodes[i].is_self_feature, tree.nodes[i].is_self_feature);
      EXPECT_EQ(saved_tree_model.nodes[i].best_party_id, tree.nodes[i].best_party_id);
      EXPECT_EQ(saved_tree_model.nodes[i].best_feature_id, tree.nodes[i].best_feature_id);
      EXPECT_EQ(saved_tree_model.nodes[i].best_split_id, tree.nodes[i].best_split_id);
      EXPECT_EQ(saved_tree_model.nodes[i].split_threshold, tree.nodes[i].split_threshold);
      EXPECT_EQ(saved_tree_model.nodes[i].node_sample_num, tree.nodes[i].node_sample_num);
      EXPECT_EQ(saved_tree_model.nodes[i].left_child, tree.nodes[i].left_child);
      EXPECT_EQ(saved_tree_model.nodes[i].right_child, tree.nodes[i].right_child);
      for (int j = 0; j < tree.nodes[i].node_sample_distribution.size(); j++) {
        EXPECT_EQ(saved_tree_model.nodes[i].node_sample_distribution[j], tree.nodes[i].node_sample_distribution[j]);
      }

      EncodedNumber deserialized_impurity = saved_tree_model.nodes[i].impurity;
      EncodedNumber deserialized_label = saved_tree_model.nodes[i].label;
      mpz_t deserialized_impurity_n, deserialized_impurity_value;
      mpz_t deserialized_label_n, deserialized_label_value;
      mpz_init(deserialized_impurity_n);
      mpz_init(deserialized_impurity_value);
      mpz_init(deserialized_label_n);
      mpz_init(deserialized_label_value);

      deserialized_impurity.getter_n(deserialized_impurity_n);
      deserialized_impurity.getter_value(deserialized_impurity_value);
      int n_cmp = mpz_cmp(v_n, deserialized_impurity_n);
      int value_cmp = mpz_cmp(v_value, deserialized_impurity_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_impurity.getter_exponent());
      EXPECT_EQ(v_type, deserialized_impurity.getter_type());

      deserialized_label.getter_n(deserialized_label_n);
      deserialized_label.getter_value(deserialized_label_value);
      n_cmp = mpz_cmp(v_n, deserialized_label_n);
      value_cmp = mpz_cmp(v_value, deserialized_label_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_label.getter_exponent());
      EXPECT_EQ(v_type, deserialized_label.getter_type());

      mpz_clear(deserialized_impurity_n);
      mpz_clear(deserialized_impurity_value);
      mpz_clear(deserialized_label_n);
      mpz_clear(deserialized_label_value);
    }
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
}