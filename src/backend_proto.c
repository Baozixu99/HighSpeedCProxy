#include "backend_proto.h"
#include "common_utils.h"


int backend_proxy_msg_prosess(uint8_t *msg){
    ProxyMsgHeader *proxy_msg_hdr;
    int proxy_proto_ver, msg_type, msg_len;
    uint32_t frontend_sess_id, backend_sess_id;

    struct BackendSession* sess;

    proxy_msg_hdr = (ProxyMsgHeader *)msg;

/*
 * Currently, the backend protocol stack does not differentiate the protocol version, we reserve the protocol version for future extensions.
 */
    proxy_proto_ver = proxy_msg_hdr->version;
    frontend_sess_id = proxy_msg_hdr->frontend_sess_id;
    msg_type = proxy_msg_hdr->proxy_msg_type;
    msg_len = proxy_msg_hdr->payload_len;
/*
 * Check the validity of the message type.
 */
    if(!PROXY_MSG_TYPE_VALID(msg_type)){
        error_print("Unsupported message type!");
        return BACKEND_PROXY_PROSESS_ERROR;
    }// Unsupported message type.

/*
 * Check the validity of the message length.
 */
    if(!PROXY_MSG_LEN_VALID(msg_type)){
        error_print("Message length error!");
        return BACKEND_PROXY_PROSESS_ERROR;
    }// Unsupported message type.

    if(PROXY_MSG_TYPE_DEV == msg_type){

    }else if(PROXY_MSG_TYPE_STRGY == msg_type){

    }else if(PROXY_MSG_TYPE_SESS == msg_type){

    }else{
/*
 * msg_type is PROXY_MSG_TYPE_DATA.
 */
    }

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