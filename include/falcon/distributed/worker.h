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


  void assign_train_feature_prefix(int train_feature_prefix_m){
    train_feature_prefix = train_feature_prefix_m;
  };

  void assign_test_feature_prefix(int test_feature_prefix_m){
    test_feature_prefix = test_feature_prefix_m;
  };

  int get_test_feature_prefix() const{
    return test_feature_prefix;
  };

  int get_train_feature_prefix() const{
    return train_feature_prefix;
  };

 private:
  // feature prefix, used in tree， default to be 0
  int train_feature_prefix = 0;
  int test_feature_prefix = 0;

};


#endif //FALCON_WORKER_H
