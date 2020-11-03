//
// Created by wuyuncheng on 3/11/20.
//

#ifndef FALCON_INCLUDE_FALCON_OPERATOR_MPC_SPDZ_CONNECTOR_H_
#define FALCON_INCLUDE_FALCON_OPERATOR_MPC_SPDZ_CONNECTOR_H_

// header files from MP-SPDZ library
#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "Networking/sockets.h"
#include "Tools/int.h"
#include "Math/Setup.h"
#include "Protocols/fake-stuff.h"

#include <sodium.h>

#include <iostream>
#include <sstream>
#include <fstream>


/**
 * setup sockets to communicate with spdz parties
 *
 * @param n_parties: number of parties for spdz computations
 * @param my_party_id: current party id
 * @param host_names: ip addresses of the spdz parties
 * @param port_base: port for the spdz program
 * @return socket vector
 */
std::vector<int> setup_sockets(int n_parties, int my_party_id,
    std::vector<std::string> host_names, int port_base);

/**
 * the Scripts/setup-online.sh in mp-spdz need to be
 * run beforehand to compute the prime param data
 *
 * @param param_data_file: the path that stores the prime param data
 */
void initialise_fields(string param_data_file);

/**
 * send public values to spdz parties (i.e., cint or cfloat)
 * currently only int vector is supported
 *
 * @param values: the values that need to be sent to spdz engines
 * @param sockets: the socket connections to spdz engines
 * @param n_parties: number of spdz parties
 */
template <typename T>
void send_public_values(std::vector<T> values, std::vector<int> sockets, int n_parties);

/**
 * send the private values masked with a random share:
 * first receive shares of a preprocessed triple from each spdz engine,
 * then combine and check if the triples are valid,
 * finally send to each spdz engine.
 *
 * @param values: the values that need to be sent to spdz engines
 * @param sockets: the socket connections to spdz engines
 * @param n_parties: number of spdz parties
 */
void send_private_values(std::vector<gfp> values, std::vector<int> sockets, int n_parties);

/**
 * send private inputs to the spdz parties with secret sharing
 *
 * @param inputs: the plaintext inputs, int or float
 * @param sockets: the socket connections to spdz engines
 * @param n_parties: number of spdz parties
 */
template <typename T>
void send_private_inputs(std::vector<T> inputs, std::vector<int> sockets, int n_parties);

/**
 * Receive the shares and post-process for the following computations.
 *
 * @param sockets the sockets of the spdz parties
 * @param nparties the number of parties
 * @param size
 */
std::vector<float> receive_result(std::vector<int>& sockets, int n_parties, int size);


#endif //FALCON_INCLUDE_FALCON_OPERATOR_MPC_SPDZ_CONNECTOR_H_
