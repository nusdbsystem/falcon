//
// Created by wuyuncheng on 12/11/20.
//

#include "../../include/message/phe_keys.pb.h"
#include <falcon/utils/pb_converter/phe_keys_converter.h>

#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/common.h>

void serialize_phe_keys(djcs_t_public_key* phe_pub_key,
    djcs_t_auth_server* phe_auth_server,
    std::string& output_message) {
  com::nus::dbsystem::falcon::v0::PHEKeys phe_keys;
  // serialize public key
  phe_keys.set_public_key_s(phe_pub_key->s);
  phe_keys.set_public_key_w(phe_pub_key->w);
  phe_keys.set_public_key_l(phe_pub_key->l);
  char* g_str_c = mpz_get_str(NULL, PHE_STR_BASE, phe_pub_key->g);
  char* delta_str_c = mpz_get_str(NULL, PHE_STR_BASE, phe_pub_key->delta);
  std::string g_str(g_str_c), delta_str(delta_str_c);
  phe_keys.set_public_key_g(g_str);
  phe_keys.set_public_key_delta(delta_str);
  if (phe_pub_key->n != NULL) {
    for (int i = 0; i < phe_pub_key->s + 1; i++) {
      char* ni_str_c = mpz_get_str(NULL, PHE_STR_BASE, phe_pub_key->n[i]);
      std::string ni_str(ni_str_c);
      phe_keys.add_public_key_n(ni_str);
      free(ni_str_c);
    }
  }
  // serialize auth server
  phe_keys.set_auth_server_i(phe_auth_server->i);
  char * si_str_c = mpz_get_str(NULL, PHE_STR_BASE, phe_auth_server->si);
  std::string si_str(si_str_c);
  phe_keys.set_auth_server_si(si_str);

  phe_keys.SerializeToString(&output_message);

  free(g_str_c);
  free(delta_str_c);
  free(si_str_c);
  phe_keys.Clear();
}

void deserialize_phe_keys(djcs_t_public_key* phe_pub_key,
    djcs_t_auth_server* phe_auth_server,
    std::string input_message) {
  com::nus::dbsystem::falcon::v0::PHEKeys deserialized_phe_keys;
  if (!deserialized_phe_keys.ParseFromString(input_message)) {
    log_error("Deserialize phe keys message failed.");
    exit(EXIT_FAILURE);
  }
  // set public key
  phe_pub_key->s = deserialized_phe_keys.public_key_s();
  phe_pub_key->l = deserialized_phe_keys.public_key_l();
  phe_pub_key->w = deserialized_phe_keys.public_key_w();
  mpz_t g, delta;
  mpz_init(g);
  mpz_init(delta);
  mpz_set_str(g, deserialized_phe_keys.public_key_g().c_str(), PHE_STR_BASE);
  mpz_set_str(delta, deserialized_phe_keys.public_key_delta().c_str(), PHE_STR_BASE);
  mpz_set(phe_pub_key->g, g);
  mpz_set(phe_pub_key->delta, delta);
  phe_pub_key->n = (mpz_t *) malloc(sizeof(mpz_t) * (phe_pub_key->s+1));
  for (int i = 0; i < deserialized_phe_keys.public_key_n_size(); i++) {
    mpz_t ni;
    mpz_init(ni);
    mpz_set_str(ni, deserialized_phe_keys.public_key_n(i).c_str(), PHE_STR_BASE);
    mpz_init_set(phe_pub_key->n[i], ni);
    mpz_clear(ni);
  }
  // set auth server
  mpz_t si;
  mpz_init(si);
  mpz_set_str(si, deserialized_phe_keys.auth_server_si().c_str(), PHE_STR_BASE);
  phe_auth_server->i = deserialized_phe_keys.auth_server_i();
  mpz_set(phe_auth_server->si, si);
  //gmp_printf("phe_pub_key->g = %Zd\n", phe_pub_key->g);
  //gmp_printf("phe_pub_key->delta = %Zd\n", phe_pub_key->delta);
  //gmp_printf("phe_auth_server->si = %Zd\n", phe_auth_server->si);

  mpz_clear(g);
  mpz_clear(delta);
  mpz_clear(si);
}

