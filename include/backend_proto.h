#ifndef BACKEND_PROTO_H
#define BACKEND_PROTO_H

#include "message.h"

int proxy_msg_prosess(uint8_t *msg);
int proxy_msg_response(uint8_t *msg);

int proxy_dev_msg_prosess(uint8_t *msg);
int proxy_dev_msg_response(uint8_t *msg);

int proxy_dev_msg_prosess(uint8_t *msg);
int proxy_dev_msg_response(uint8_t *msg);

int proxy_strgy_msg_process(uint8_t *msg);
int proxy_strgy_msg_response(uint8_t *msg);

int proxy_sess_msg_process(uint8_t *msg);
int proxy_sess_msg_response(uint8_t *msg);

#endif