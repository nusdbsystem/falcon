//
// Created by root on 11/13/21.
//

#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/metric/classification.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/utils/logger/logger.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision
#include <utility>

#include <glog/logging.h>

// generate a batch (array) of data indexes according to the shuffle
// array size is the batch-size
// called in each new iteration
std::vector<int> select_batch_idx(const Party& party,
                                  int batch_size,
                                  std::vector<int> data_indexes) {
  std::vector<int> batch_indexes;
  // if active party, randomly select batch indexes and send to other parties
  // if passive parties, receive batch indexes and return
  if (party.party_type == falcon::ACTIVE_PARTY) {
    log_info("ACTIVE_PARTY select batch indexes");
    // NOTE: cannot use a fixed seed here!!
    // select_batch_idx actually return a batch of indexes
    // cannot use a fixed seed, otherwise the data indexes are fixed
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);
    for (int i = 0; i < batch_size; i++) {
      // LOG(INFO) << "data_indexes selected = " << data_indexes[i] << std::endl;
      batch_indexes.push_back(data_indexes[i]);
    }
    // send to other parties
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        std::string batch_indexes_str;
        serialize_int_array(batch_indexes, batch_indexes_str);
        party.send_long_message(i, batch_indexes_str);
      }
    }
  } else {
    std::string recv_batch_indexes_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_batch_indexes_str);
    deserialize_int_array(batch_indexes, recv_batch_indexes_str);
  }
  return batch_indexes;
}

void init_encrypted_random_numbers(const Party& party,
                                   int vector_size,
                                   EncodedNumber* encrypted_vector,
                                   int precision) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  std::random_device rd;
  std::mt19937 mt(rd());
  // initialization of weights
  // random initialization with a uniform range
  std::uniform_real_distribution<double> dist(
      WEIGHTS_INIT_MIN,
      WEIGHTS_INIT_MAX
  );
  // srand(static_cast<unsigned> (time(nullptr)));
  for (int i = 0; i < vector_size; i++) {
    // generate random double values within (0, 1],
    // init fixed point EncodedNumber,
    // and encrypt with public key
    double v = dist(mt);
    // double v = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    EncodedNumber t;
    t.set_double(phe_pub_key->n[0], v, precision);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_vector[i], t);
  }
  djcs_t_free_public_key(phe_pub_key);
}
