//
// Created by wuyuncheng on 3/11/20.
//

#ifndef FALCON_INCLUDE_FALCON_OPERATOR_MPC_SPDZ_CONNECTOR_H_
#define FALCON_INCLUDE_FALCON_OPERATOR_MPC_SPDZ_CONNECTOR_H_

// header files from MP-SPDZ library
#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "Networking/sockets.h"
#include "Networking/ssl_sockets.h"
#include "Tools/int.h"
#include "Math/Setup.h"
#include "Protocols/fake-stuff.h"

#include "falcon/common.h"

#include <sodium.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>

#include <glog/logging.h>

/**
 * setup sockets to communicate with spdz parties
 *
 * @param n_parties: number of parties for spdz computations
 * @param my_party_id: current party id
 * @param player_data_path: ssl related key path
 * @param host_names: ip addresses of the spdz parties
 * @param port_base: port for the spdz program
 * @return socket vector
 */
std::vector<ssl_socket*> setup_sockets(int n_parties,
    int my_party_id,
    std::string player_data_path,
    std::vector<std::string> host_names,
    int port_base);

/**
 * send public values to spdz parties (i.e., cint or cfloat)
 * currently only int vector is supported
 *
 * @param values: the values that need to be sent to spdz engines
 * @param sockets: the ssl socket connections to spdz engines
 * @param n_parties: number of spdz parties
 */
template <class T>
void send_public_values(std::vector<T> values, vector<ssl_socket*>& sockets, int n_parties)
{
  octetStream os;
  int size = values.size();
  vector<gfp> parameters(size);

  if (std::is_same<T, int>::value) {
    for (int i = 0; i < size; i++) {
      parameters[i].assign(values[i]);
      parameters[i].pack(os);
    }
  } else {
    LOG(ERROR) << "Public values other than int type are not supported, aborting.";
    // cerr << "Public values other than int type are not supported, aborting.\n";
    exit(1);
  }

  for (int i = 0; i < n_parties; i++) {
    os.Send(sockets[i]);
  }
}

/**
 * send the private values masked with a random share:
 * first receive shares of a preprocessed triple from each spdz engine,
 * then combine and check if the triples are valid,
 * finally send to each spdz engine.
 *
 * @param values: the gfp values that need to be sent to spdz engines
 * @param sockets: the ssl socket connections to spdz engines
 * @param n_parties: number of spdz parties
 */
void send_private_values(std::vector<gfp> values, vector<ssl_socket*>& sockets, int n_parties);

/**
 * send private inputs to the spdz parties with secret sharing
 *
 * @param inputs: the plaintext inputs, int or float
 * @param sockets: the ssl socket connections to spdz engines
 * @param n_parties: number of spdz parties
 */
template <class T>
void send_private_inputs(const std::vector<T>& inputs, vector<ssl_socket*>& sockets, int n_parties)
{
  // now only support float type
  if (!std::is_same<T, float >::value) {
    LOG(ERROR) << "Private inputs other than float type are not supported, aborting.";
    // cerr << "Private inputs other than int type are not supported, aborting.\n";
    exit(1);
  }

  int size = inputs.size();
  std::vector<int64_t> long_shares(size);

  // step 1: convert to int or long according to the fixed precision
  for (int i = 0; i < size; ++i) {
    long_shares[i] = static_cast<int64_t>(round(inputs[i] * pow(2, SPDZ_FIXED_POINT_PRECISION)));
  }

  // step 2: convert to the gfp value and call send_private_inputs
  // Map inputs into gfp
  vector<gfp> input_values_gfp(size);
  for (int i = 0; i < size; i++) {
    input_values_gfp[i].assign(long_shares[i]);
  }

  // call sending values
  send_private_values(input_values_gfp, sockets, n_parties);
}

/**
 * Receive the shares and post-process for the following computations.
 *
 * @param sockets: the ssl sockets of the spdz parties
 * @param nparties: the number of parties
 * @param size: size of recieved results
 */
std::vector<float> receive_result(vector<ssl_socket*>& sockets, int n_parties, int size);


#endif //FALCON_INCLUDE_FALCON_OPERATOR_MPC_SPDZ_CONNECTOR_H_
