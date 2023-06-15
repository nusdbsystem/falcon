//
// Created by wuyuncheng on 13/8/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_MODEL_PB_CONVERTER_H_
#define FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_MODEL_PB_CONVERTER_H_

#include <string>

/**
 * serialize a model publish request protobuf message
 *
 * @param model_id: the target model id
 * @param initiator_party_id: the party that sends the request
 * @param output_message: serialized message to be sent
 */
void serialize_model_publish_request(int model_id, int initiator_party_id,
                                     std::string &output_message);

/**
 * deserialize model publish request from a protobuf message
 *
 * @param model_id: the target model id
 * @param initiator_party_id: the party that sends the request
 * @param input_message: received message
 */
void deserialize_model_publish_request(int &model_id, int &initiator_party_id,
                                       std::string input_message);

/**
 * serialize a model publish response protobuf message
 *
 * @param model_id: the target model id
 * @param initiator_party_id: the party that sends the request
 * @param is_success: whether the request is succeed
 * @param error_code
 * @param error_msg
 * @param output_message: serialized message to be sent
 */
void serialize_model_publish_response(int model_id, int initiator_party_id,
                                      int is_success, int error_code,
                                      const std::string &error_msg,
                                      std::string &output_message);

/**
 * deserialize model publish response from a protobuf message
 *
 * @param model_id: the target model id
 * @param initiator_party_id: the party that sends the request
 * @param is_success: whether the request is succeed
 * @param error_code
 * @param error_msg
 * @param input_message: received message
 */
void deserialize_model_publish_response(int &model_id, int &initiator_party_id,
                                        int &is_success, int &error_code,
                                        std::string &error_msg,
                                        const std::string &input_message);

#endif // FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_MODEL_PB_CONVERTER_H_
