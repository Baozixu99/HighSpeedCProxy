#ifndef BACKEND_PROTO_H
#define BACKEND_PROTO_H

#include "message.h"
#include "session.h"

#define BACKEND_PROXY_PROSESS_OK               0
#define BACKEND_PROXY_PROSESS_ERROR            1

#define PROXY_MSG_TYPE_VALID(x) (((x) == PROXY_MSG_TYPE_DEV)   || \
                                 ((x) == PROXY_MSG_TYPE_STRGY) || \
                                 ((x) == PROXY_MSG_TYPE_SESS)  || \
                                 ((x) == PROXY_MSG_TYPE_DATA)) 

#define PROXY_MSG_LEN_VALID(x) (((x) >= PROXY_MSG_MIN_SIZE)   || \
                                 ((x) <= PROXY_MSG_MAX_SIZE))

#define FRONTEND_ADMIN_SESSION_ID                            0
#define BACKEND_ADMIN_SESSION_ID                             0
#define FRONTEND_HANDOVER_SESSION_ID                         0xFFFF
#define BACKEND_HANDOVER_SESSION_ID                          0xFFFF



int backend_proxy_msg_prosess(uint8_t *msg);
int backend_proxy_msg_response(uint8_t *msg);

int backend_proxy_dev_msg_prosess(uint8_t *msg);
int backend_proxy_dev_msg_response(uint8_t *msg);

int backend_proxy_strgy_msg_process(uint8_t *msg);
int backend_proxy_strgy_msg_response(uint8_t *msg);

int backend_proxy_sess_msg_process(uint8_t *msg);
int backend_proxy_sess_msg_response(uint8_t *msg);

int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg);
int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg);

#endif