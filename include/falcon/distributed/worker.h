//
// Created by nai li xing on 11/08/21.
//

#ifndef FALCON_WORKER_H
#define FALCON_WORKER_H


class Worker {
 public:
  // Worker(Worker worker);
  // communication channel between parameter server and follower
  shared_ptr<CommParty> ps_channel;
  // boost i/o functionality
  boost::asio::io_service io_service;

  // worker id
  int worker_id;

 public:

  Worker()=default;
  Worker(const std::string& ps_network_config_pb_str, int worker_id);
  ~Worker();

  /**
   * send long message to parameter server via channel commParty
   * @param message: sent message
   */
  void send_long_message_to_ps(string message) const;

  /**
   * receive message from parameter server via channel comm_party
   * @param message: received message
   */
  void recv_long_message_from_ps(std::string& message) const;

};


#endif //FALCON_WORKER_H
