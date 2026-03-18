#ifndef BACKEND_PROTO_H
#define BACKEND_PROTO_H

#include "message.h"
#include "session.h"
#include "session_pool.h"
#include "common_utils.h"


#define PROXY_MSG_TYPE_VALID(x) (((x) == PROXY_MSG_TYPE_DEV)   || \
                                 ((x) == PROXY_MSG_TYPE_STRGY) || \
                                 ((x) == PROXY_MSG_TYPE_SESS)  || \
                                 ((x) == PROXY_MSG_TYPE_DATA)  || \
                                 ((x) == PROXY_MSG_TYPE_IOT)) 

#define PROXY_MSG_LEN_VALID(x) (((x) >= PROXY_MSG_MIN_SIZE)   || \
                                 ((x) <= PROXY_MSG_MAX_SIZE))

#define PROXY_ADMIN_SESSION_ID                               0
#define FRONTEND_ADMIN_SESSION_ID                            PROXY_ADMIN_SESSION_ID
#define BACKEND_ADMIN_SESSION_ID                             PROXY_ADMIN_SESSION_ID

#define PROXY_HANDOVER_SESSION_ID                            0xFFFF
#define FRONTEND_HANDOVER_SESSION_ID                         PROXY_HANDOVER_SESSION_ID
#define BACKEND_HANDOVER_SESSION_ID                          PROXY_HANDOVER_SESSION_ID

#define APP_SESSION_ID_VALID(x) (((x) != PROXY_ADMIN_SESSION_ID)   || \
                                 ((x) != PROXY_HANDOVER_SESSION_ID))


int backend_proxy_msg_process(uint8_t *msg);
int backend_proxy_msg_response(uint8_t *msg);

int backend_proxy_dev_msg_process(uint8_t *msg);
int backend_proxy_dev_msg_response(uint8_t *msg);

int backend_proxy_dev_msg_process_ver1(uint16_t msg_type, uint16_t msg_id, uint16_t action_type, uint16_t payload_len, uint8_t *msg_payload);
int backend_proxy_dev_msg_process_enable_ver1(uint16_t payload_len, uint8_t *msg_payload);
int backend_proxy_dev_msg_process_disable_ver1(uint16_t payload_len, uint8_t *msg_payload);
int backend_proxy_dev_msg_process_query_ver1(uint16_t payload_len, uint8_t *msg_payload);


int backend_proxy_strgy_msg_process(uint8_t *msg);
int backend_proxy_strgy_msg_response(uint8_t *msg);
int backend_proxy_strgy_msg_process_ver1(uint16_t msg_type, uint16_t msg_id, uint16_t action_type, uint16_t payload_len, uint8_t *msg_payload);
int backend_proxy_strgy_msg_process_set_ver1(uint16_t payload_len, uint8_t *msg_payload);
int backend_proxy_strgy_msg_process_query_ver1(uint16_t payload_len, uint8_t *msg_payload);


int backend_proxy_sess_msg_process(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint8_t *msg);
int backend_proxy_sess_msg_response(uint8_t *msg);
int backend_proxy_sess_msg_process_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t msg_type, 
                                        uint16_t action_type, uint16_t ip_version, uint16_t payload_len, 
                                        uint8_t *msg_payload);
int backend_proxy_sess_msg_process_active_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload);
int backend_proxy_sess_msg_process_passive_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload);
int backend_proxy_sess_msg_process_close_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t payload_len, uint8_t *msg_payload);


int __backend_proxy_sess_msg_process_active_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload);
int __backend_proxy_sess_msg_process_passive_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload);
int __backend_proxy_sess_msg_process_close_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t payload_len, uint8_t *msg_payload);


int backend_proxy_data_msg_process(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t data_len, uint8_t *msg);
int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg);
int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg);



int backend_proxy_shmem_data_msg_recv(struct BackendSession *sess, uint8_t **msg);
int backend_proxy_shmem_data_msg_send(struct BackendSession *sess, const uint8_t **msg);
int backend_proxy_sock_data_msg_recv(struct BackendSession *sess, uint8_t *msg);
int backend_proxy_sock_data_msg_send(struct BackendSession *sess, uint8_t *msg);


struct BackendSession *backend_proxy_search_sess(uint32_t backend_sess_id);


struct BackendEngine_;

int backend_proxy_generate_dev_msg_enable_response(struct BackendEngine_ *eng, uint16_t msg_id, uint8_t **msg);
int backend_proxy_generate_dev_msg_disable_response(struct BackendEngine_ *eng, uint16_t msg_id, uint8_t **msg);
int backend_proxy_generate_dev_msg_query_response(struct BackendEngine_ *eng, uint16_t msg_id, uint8_t **msg);


int backend_proxy_generate_sess_msg_create_response(struct BackendSession *sess, SessOpRespData *op_resp, uint8_t **msg);
int backend_proxy_generate_sess_msg_close_response(struct BackendSession *sess, SessOpRespData *op_resp, uint8_t **msg);
int backend_proxy_generate_sess_msg_create_close_response(struct BackendSession *sess, int sess_msg_type, SessOpRespData *op_resp, uint8_t **msg);
int backend_proxy_generate_sess_msg_create_close_response_standalone(struct BackendEngine_ *eng, uint16_t frontend_sess_id, uint16_t backend_sess_id,
                                                                     int ip_version, int sess_msg_type, SessOpRespData *op_resp, uint8_t **msg);

int backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem(struct BackendEngine_ *eng, uint16_t frontend_sess_id, 
                                                                 uint16_t backend_sess_id, int ip_version, 
                                                                 int sess_msg_type, SessOpRespData *op_resp);

int backend_proxy_send_sess_msg_to_frontend_via_shmem(struct BackendSession *sess, int sess_msg_type, SessOpRespData *op_resp);



int backend_proxy_iot_msg_process(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t msg_len, uint8_t *msg);


int backend_proxy_bluetooth_msg_process(uint16_t frontend_sess_id, 
                                        uint16_t backend_sess_id,
                                        IotMsgHeader *iot_header,
                                        uint8_t *iot_data);


int backend_proxy_can_msg_process(uint16_t frontend_sess_id, 
                                  uint16_t backend_sess_id,
                                  IotMsgHeader *iot_header,
                                  uint8_t *iot_data);


int backend_proxy_zigbee_msg_process(uint16_t frontend_sess_id, 
                                     uint16_t backend_sess_id,
                                     IotMsgHeader *iot_header,
                                     uint8_t *iot_data);


int backend_proxy_lora_msg_process(uint16_t frontend_sess_id, 
                                   uint16_t backend_sess_id,
                                   IotMsgHeader *iot_header,
                                   uint8_t *iot_data);


int backend_proxy_powerlink_msg_process(uint16_t frontend_sess_id, 
                                        uint16_t backend_sess_id,
                                        IotMsgHeader *iot_header,
                                        uint8_t *iot_data);

int backend_proxy_powerlink_msg_process(uint16_t frontend_sess_id, 
                                        uint16_t backend_sess_id,
                                        IotMsgHeader *iot_header,
                                        uint8_t *iot_data);

size_t get_iot_addr_length(IotProtoType addr_type);


#endif