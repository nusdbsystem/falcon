//
// Created by wuyuncheng on 13/8/20.
//

#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <stack>

#include <glog/logging.h>

#include <falcon/operator/phe/djcs_t_aux.h>
#include <falcon/network/ConfigFile.hpp>
#include <falcon/party/party.h>
#include <falcon/utils/io_util.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>

Party::Party(){}

Party::Party(int m_party_id,
             int m_party_num,
             falcon::PartyType m_party_type,
             falcon::FLSetting m_fl_setting,
             const std::string& m_network_file,
             const std::string& m_data_file,
             bool m_use_existing_key,
             const std::string& m_key_file) {
  // copy params
  party_id = m_party_id;
  party_num = m_party_num;
  party_type = m_party_type;
  fl_setting = m_fl_setting;

  // read dataset
  local_data = read_dataset(m_data_file);
  sample_num = local_data.size();
  feature_num = local_data[0].size();

  // if this is active party, slice the last column as labels
  if (party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < sample_num; i++) {
      labels.push_back(local_data[i][feature_num-1]);
      local_data[i].pop_back();
    }
    feature_num = feature_num - 1;
  }

  // read network config file
  ConfigFile config_file(m_network_file);
  std::string port_string, ip_string;
  std::vector<int> ports(party_num);
  std::vector<std::string> ips(party_num);
  for (int i = 0; i < party_num; i++) {
    port_string = "party_" + to_string(i) + "_port";
    ip_string = "party_" + to_string(i) + "_ip";
    //get parties ips and ports
    ports[i] = stoi(config_file.Value("", port_string));
    ips[i] = config_file.Value("", ip_string);
  }

  // establish communication connections
  SocketPartyData me, other;
  for (int  i = 0;  i < party_num; ++ i) {
    if (i < party_id) {
      // this party will be the receiver in the protocol
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]), ports[party_id] + i);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + party_id - 1);
      shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

      // connect to the other party and add channel
      channel->join(500,5000);
      LOG(INFO) << "Communication channel established with party " << i << ", port is " << ports[party_id] + i << ".";
      channels.push_back(std::move(channel));
    } else if (i > party_id) {
      // this party will be the sender in the protocol
      me = SocketPartyData(boost_ip::address::from_string(ips[party_id]), ports[party_id] + i - 1);
      other = SocketPartyData(boost_ip::address::from_string(ips[i]), ports[i] + party_id);
      shared_ptr<CommParty> channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

      // connect to the other party and add channel
      channel->join(500,5000);
      LOG(INFO) << "Communication channel established with party " << i << ", port is " << ports[party_id] + i << ".";
      channels.push_back(std::move(channel));
    } else {
      // self communication
      shared_ptr<CommParty> channel = nullptr;
      channels.push_back(std::move(channel));
    }
  }

  host_names = ips;

  // init phe keys: if use existing key, read key file
  // otherwise, generate keys and broadcast to others
  phe_random = hcs_init_random();
  phe_pub_key = djcs_t_init_public_key();
  phe_auth_server = djcs_t_init_auth_server();
  if (m_use_existing_key) {
    init_with_key_file(m_key_file);
  } else {
    // the active party generates new phe keys and send to passive parties
    init_with_new_phe_keys(PHE_EPSILON, PHE_KEY_SIZE, party_num);
    std::string phe_keys_str;
    serialize_phe_keys(phe_pub_key, phe_auth_server, phe_keys_str);
    write_key_to_file(phe_keys_str, m_key_file);
  }
}

Party::Party(const Party &party) {
  // copy public variables
  party_id = party.party_id;
  party_num = party.party_num;
  party_type = party.party_type;
  fl_setting = party.fl_setting;
  channels = party.channels;
  host_names = party.host_names;

  // copy private variables
  feature_num = party.getter_feature_num();
  sample_num = party.getter_sample_num();
  local_data = party.getter_local_data();
  labels = party.getter_labels();

  // init phe keys and copy from party
  phe_random = hcs_init_random();
  phe_pub_key = djcs_t_init_public_key();
  phe_auth_server = djcs_t_init_auth_server();
  hcs_random* tmp_random = hcs_init_random();
  djcs_t_public_key* tmp_pub_key = djcs_t_init_public_key();
  djcs_t_auth_server* tmp_auth_server = djcs_t_init_auth_server();
  party.getter_phe_random(tmp_random);
  party.getter_phe_pub_key(tmp_pub_key);
  party.getter_phe_auth_server(tmp_auth_server);
  djcs_t_hcs_random_copy(tmp_random, phe_random);
  djcs_t_public_key_copy(tmp_pub_key, phe_pub_key);
  djcs_t_auth_server_copy(tmp_auth_server, phe_auth_server);

  // free tmp phe key objects
  hcs_free_random(tmp_random);
  djcs_t_free_public_key(tmp_pub_key);
  djcs_t_free_auth_server(tmp_auth_server);
}

void Party::init_with_new_phe_keys(int epsilon, int phe_key_size, int required_party_num) {
  if (party_type == falcon::ACTIVE_PARTY) {
    // generate phe keys
    hcs_random* random = hcs_init_random();
    djcs_t_public_key* pub_key = djcs_t_init_public_key();
    djcs_t_private_key* priv_key = djcs_t_init_private_key();
    djcs_t_auth_server** auth_server = (djcs_t_auth_server **) malloc (party_num * sizeof(djcs_t_auth_server *));
    mpz_t* si = (mpz_t *) malloc (party_num * sizeof(mpz_t));
    djcs_t_generate_key_pair(pub_key, priv_key, random, epsilon, phe_key_size, party_num, required_party_num);
    mpz_t* coeff = djcs_t_init_polynomial(priv_key, random);
    for (int i = 0; i < party_num; i++) {
      mpz_init(si[i]);
      djcs_t_compute_polynomial(priv_key, coeff, si[i], i);
      auth_server[i] = djcs_t_init_auth_server();
      djcs_t_set_auth_server(auth_server[i], si[i], i);
    }

    // serialize phe keys and send to passive parties
    for(int i = 0; i < party_num; i++) {
      if (i == party_id) {
        djcs_t_public_key_copy(pub_key, phe_pub_key);
        djcs_t_auth_server_copy(auth_server[i], phe_auth_server);
      } else {
        std::string phe_keys_message_i;
        serialize_phe_keys(pub_key, auth_server[i], phe_keys_message_i);
        send_long_message(i, phe_keys_message_i);
      }
    }

    // free memory
    hcs_free_random(random);
    djcs_t_free_public_key(pub_key);
    djcs_t_free_private_key(priv_key);
    djcs_t_free_polynomial(priv_key, coeff);
    for (int i = 0; i < party_num; i++) {
      mpz_clear(si[i]);
      djcs_t_free_auth_server(auth_server[i]);
    }
    free(si);
    free(auth_server);
  } else {
    // for passive parties, receive the phe keys message from the active party
    // and set its own keys with the received message
    // TODO: now active party id is 0 by design, could abstract as a variable
    std::string recv_phe_keys_message;
    recv_long_message(0, recv_phe_keys_message);
    deserialize_phe_keys(phe_pub_key, phe_auth_server, recv_phe_keys_message);
  }
}

void Party::init_with_key_file(const std::string& key_file) {
  // read key file as string
  std::string phe_keys_str = read_key_file(key_file);
  deserialize_phe_keys(phe_pub_key, phe_auth_server, phe_keys_str);
}

void Party::send_message(int id, std::string message) {
  channels[id]->write((const byte *) message.c_str(), message.size());
}

void Party::send_long_message(int id, string message) {
  channels[id]->writeWithSize(message);
}

void Party::recv_message(int id, std::string message, byte *buffer, int expected_size) {
  channels[id]->read(buffer, expected_size);
  // the size of all strings is 2. Parse the message to get the original strings
  auto s = string(reinterpret_cast<char const*>(buffer), expected_size);
  message = s;
}

void Party::recv_long_message(int id, std::string &message) {
  vector<byte> recv_message;
  channels[id]->readWithSizeIntoVector(recv_message);
  const byte * uc = &(recv_message[0]);
  string recv_message_str(reinterpret_cast<char const*>(uc), recv_message.size());
  message = recv_message_str;
}

Party::~Party() {
  io_service.stop();
  hcs_free_random(phe_random);
  djcs_t_free_public_key(phe_pub_key);
  djcs_t_free_auth_server(phe_auth_server);
}

