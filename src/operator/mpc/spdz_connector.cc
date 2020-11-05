//
// Created by wuyuncheng on 3/11/20.
//

#include "falcon/operator/mpc/spdz_connector.h"
#include "falcon/common.h"

#include "math.h"

#include <glog/logging.h>

std::vector<int> setup_sockets(int n_parties, int my_party_id,
    std::vector<std::string> host_names, int port_base) {
  // setup connections from this party to each spdz party socket
  std::vector<int> sockets(n_parties);
  for (int i = 0; i < n_parties; i++)
  {
    set_up_client_socket(sockets[i], host_names[i].c_str(), port_base + i);
    send(sockets[i], (octet*) &my_party_id, sizeof(int));
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
      " sockets = " << sockets[i] << ", port_num = " << port_base + i << ".";
  }
  LOG(INFO) << "Finish setup socket connections to spdz engines.";
  return sockets;
}

void initialise_fields(string param_data_file)
{
  LOG(INFO) << "Param data file is " << param_data_file << ".";

  int lg2;
  bigint p;

  ifstream inpf(param_data_file.c_str());
  if (inpf.fail()) {
    LOG(ERROR) << "Open " << param_data_file << "failed.";
    //throw file_error(param_data_file.c_str());
  }
  inpf >> p;
  inpf >> lg2;

  inpf.close();

  gfp::init_field(p);
  //gf2n::init_field(lg2);
}

template <typename T>
void send_public_values(std::vector<T> values, std::vector<int> sockets, int n_parties) {
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

void send_private_values(std::vector<gfp> values, std::vector<int> sockets, int n_parties)
{
  int num_inputs = values.size();
  octetStream os;
  std::vector< std::vector<gfp> > triples(num_inputs, vector<gfp>(3));
  std::vector<gfp> triple_shares(3);

  // receive num_inputs triples from spdz engines
  for (int j = 0; j < n_parties; j++)
  {
    os.reset_write_head();
    os.Receive(sockets[j]);

    for (int j = 0; j < num_inputs; j++)
    {
      for (int k = 0; k < 3; k++)
      {
        triple_shares[k].unpack(os);
        triples[j][k] += triple_shares[k];
      }
    }
  }

  // check triple relations (is a party cheating?)
  for (int i = 0; i < num_inputs; i++)
  {
    if (triples[i][0] * triples[i][1] != triples[i][2])
    {
      LOG(ERROR) << "Incorrect triple at " << i << ", aborting.";
      // cerr << "Incorrect triple at " << i << ", aborting\n";
      exit(1);
    }
  }

  // send inputs + triple[0], so spdz engines can compute shares of each value
  os.reset_write_head();
  for (int i = 0; i < num_inputs; i++)
  {
    gfp y = values[i] + triples[i][0];
    y.pack(os);
  }
  for (int j = 0; j < n_parties; j++)
    os.Send(sockets[j]);
}

template <typename T>
void send_private_inputs(std::vector<T> inputs, std::vector<int> sockets, int n_parties) {
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