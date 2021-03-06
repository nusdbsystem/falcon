//
// Created by wuyuncheng on 6/3/21.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_INFERENCE_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_INFERENCE_H_

#include <string>

class Inference {
 public:
  // endpoint ip address
  std::string endpoint_ip;
  // endpoint port
  int endpoint_port;

 public:
  /**
   * default constructor
   */
  Inference() {}

  /**
   * constructor
   * @param endpoint: received from coordinator
   */
  explicit Inference(const std::string& endpoint);

  /**
   * create a listener to accept user's inference requests
   */
  virtual void create_listener();
};

#endif //FALCON_SRC_EXECUTOR_ALGORITHM_INFERENCE_H_
