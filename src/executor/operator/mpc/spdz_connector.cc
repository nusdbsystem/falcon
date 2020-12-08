//
// Created by wuyuncheng on 3/11/20.
//

#include "falcon/operator/mpc/spdz_connector.h"
#include "falcon/common.h"

#include "math.h"

#include <glog/logging.h>


std::vector<ssl_socket*> setup_sockets(int n_parties,
    int my_party_id,
    std::string player_data_path,
    std::vector<std::string> host_names,
    int port_base) {
  // setup connections from this party to each spdz party socket
  vector<int> plain_sockets(n_parties);
  std::vector<ssl_socket*> sockets(n_parties);
  ssl_ctx ctx(player_data_path, "C" + to_string(my_party_id));
  ssl_service io_service;
  octetStream specification;
  for (int i = 0; i < n_parties; i++)
  {
    set_up_client_socket(plain_sockets[i], host_names[i].c_str(), port_base + i);
    send(plain_sockets[i], (octet*) &my_party_id, sizeof(int));
    sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                "P" + to_string(i), "C" + to_string(my_party_id), true);
    if (i == 0){
      // receive gfp prime
      specification.Receive(sockets[0]);
    }
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                                                          " sockets = " << sockets[i] << ", port_num = " << port_base + i << ".";
  }
  LOG(INFO) << "Finish setup socket connections to spdz engines.";

  int type = specification.get<int>();
  switch (type)
  {
    case 'p':
    {
      gfp::init_field(specification.get<bigint>());
      LOG(INFO) << "Using prime " << gfp::pr();
      break;
    }
    default:
      LOG(ERROR) << "Type " << type << " not implemented";
      exit(1);
  }
  LOG(INFO) << "Finish initializing gfp field.";

  return sockets;
}

void setup_sockets(int n_parties,
    int my_party_id,
    std::string player_data_path,
    std::vector<std::string> host_names,
    int port_base,
    std::vector<ssl_socket*>& sockets) {
  // setup connections from this party to each spdz party socket
  vector<int> plain_sockets(n_parties);
  //std::vector<ssl_socket*> sockets(n_parties);
  ssl_ctx ctx(player_data_path, "C" + to_string(my_party_id));
  ssl_service io_service;
  octetStream specification;
  for (int i = 0; i < n_parties; i++)
  {
    set_up_client_socket(plain_sockets[i], host_names[i].c_str(), port_base + i);
    send(plain_sockets[i], (octet*) &my_party_id, sizeof(int));
    sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                "P" + to_string(i), "C" + to_string(my_party_id), true);
    if (i == 0){
      // receive gfp prime
      specification.Receive(sockets[0]);
    }
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                                                          " sockets = " << sockets[i] << ", port_num = " << port_base + i << ".";
  }
  LOG(INFO) << "Finish setup socket connections to spdz engines.";

  int type = specification.get<int>();
  switch (type)
  {
    case 'p':
    {
      gfp::init_field(specification.get<bigint>());
      LOG(INFO) << "Using prime " << gfp::pr();
      break;
    }
    default:
      LOG(ERROR) << "Type " << type << " not implemented";
      exit(1);
  }
  LOG(INFO) << "Finish initializing gfp field.";

  //return sockets;
}

void send_private_values(std::vector<gfp> values, vector<ssl_socket*>& sockets, int n_parties)
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

std::vector<float> receive_result(vector<ssl_socket*>& sockets, int n_parties, int size)
{
  LOG(INFO) << "Receive mpc computation result from the SPDZ engine";
  std::vector<gfp> output_values(size);
  octetStream os;
  for (int i = 0; i < n_parties; i++)
  {
    os.reset_write_head();
    os.Receive(sockets[i]);
    for (int j = 0; j < size; j++)
    {
      gfp value;
      value.unpack(os);
      output_values[j] += value;
    }
  }

  std::vector<float> res_shares(size);

  for (int i = 0; i < size; i++) {
    gfp val = output_values[i];
    bigint aa;
    to_signed_bigint(aa, val);
    long t = aa.get_si();
    // cout<< "i = " << i << ", t = " << t <<endl;
    res_shares[i] = static_cast<float>(t * pow(2, 0 - SPDZ_FIXED_POINT_PRECISION));
  }

  return res_shares;
}