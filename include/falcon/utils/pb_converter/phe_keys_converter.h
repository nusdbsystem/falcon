//
// Created by wuyuncheng on 12/11/20.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_PHE_KEYS_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_PHE_KEYS_CONVERTER_H_

#include <string>

#include "gmp.h"
#include "libhcs.h"

/**
 * serialize phe keys proto message
 *
 * @param phe_pub_key: public key object
 * @param phe_auth_server: authenticated server object
 * @param output_message: serialized string
 */
void serialize_phe_keys(djcs_t_public_key *phe_pub_key,
                        djcs_t_auth_server *phe_auth_server,
                        std::string &output_message);

/**
 * deserialize phe keys from proto message
 *
 * @param phe_pub_key: public key object
 * @param phe_auth_server: authenticated server object
 * @param input_message: serialized string
 */
void deserialize_phe_keys(djcs_t_public_key *phe_pub_key,
                          djcs_t_auth_server *phe_auth_server,
                          std::string input_message);

#endif // FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_PHE_KEYS_CONVERTER_H_
