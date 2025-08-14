#ifndef BACKEND_PROTO_H
#define BACKEND_PROTO_H

#include "message.h"
#include "session.h"
#include "common_utils.h"

#define BACKEND_PROXY_PROCESS_OK               0
#define BACKEND_PROXY_PROCESS_ERROR            1

#define PROXY_MSG_TYPE_VALID(x) (((x) == PROXY_MSG_TYPE_DEV)   || \
                                 ((x) == PROXY_MSG_TYPE_STRGY) || \
                                 ((x) == PROXY_MSG_TYPE_SESS)  || \
                                 ((x) == PROXY_MSG_TYPE_DATA)) 

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


int backend_proxy_msg_prosess(uint8_t *msg);
int backend_proxy_msg_response(uint8_t *msg);

int backend_proxy_dev_msg_process(uint8_t *msg);
int backend_proxy_dev_msg_response(uint8_t *msg);

int backend_proxy_dev_msg_process_ver1(uint32_t msg_type, uint32_t msg_id, uint32_t action_type, uint32_t payload_len, uint8_t *msg_payload);
int backend_proxy_dev_msg_process_enable_ver1(uint32_t payload_len, uint8_t *msg_payload);
int backend_proxy_dev_msg_process_disable_ver1(uint32_t payload_len, uint8_t *msg_payload);
int backend_proxy_dev_msg_process_query_ver1(uint32_t payload_len, uint8_t *msg_payload);


int backend_proxy_strgy_msg_process(uint8_t *msg);
int backend_proxy_strgy_msg_response(uint8_t *msg);

int backend_proxy_sess_msg_process(uint8_t *msg);
int backend_proxy_sess_msg_response(uint8_t *msg);

int backend_proxy_data_msg_prosess(uint32_t frontend_sess_id, uint32_t backend_sess_id, uint8_t *msg);
int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg);
int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg);

struct BackendSession *backend_proxy_search_sess(uint32_t backend_sess_id);

#endif