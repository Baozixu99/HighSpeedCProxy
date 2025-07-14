#include "backend_proto.h"


int backend_proxy_msg_prosess(uint8_t *msg){
    ProxyMsgHeader *proxy_msg_hdr;
    int msg_type, msy_len;
    uint32_t frontend_sess_id, backend_sess_id;

    proxy_msg_hdr = (ProxyMsgHeader *)msg;

    msg_type = proxy_msg_hdr->proxy_msg_type;
    

    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_dev_msg_prosess(uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_dev_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_strgy_msg_process(uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_strgy_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_sess_msg_process(uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_sess_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}