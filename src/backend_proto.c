#include "backend_proto.h"
#include "common_utils.h"


int backend_proxy_msg_prosess(uint8_t *msg){
    ProxyMsgHeader *proxy_msg_hdr;
    int proxy_proto_ver, msg_type, msg_len, ret;
    uint32_t frontend_sess_id, backend_sess_id;
    uint8_t *msg_ptr;

    struct BackendSession* sess;

    proxy_msg_hdr = (ProxyMsgHeader *)msg;

/*
 * Currently, the backend protocol stack does not differentiate the protocol version, we reserve the protocol version for future extensions.
 */
    proxy_proto_ver = proxy_msg_hdr->version;
    frontend_sess_id = proxy_msg_hdr->frontend_sess_id;
    backend_sess_id = proxy_msg_hdr->backend_sess_id;
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

    msg_ptr = (uint8_t *)proxy_msg_hdr;
    msg_ptr += PROXY_MSG_HDR_SIZE;
    if(PROXY_MSG_TYPE_DEV == msg_type){
    /* 
     * The frontend proxy delivers device messages from the frontend admin session to the backend proxy for the backend admin session.
     */
        if (frontend_sess_id != FRONTEND_ADMIN_SESSION_ID || backend_sess_id != BACKEND_ADMIN_SESSION_ID){
            error_print("Only admin sessions can deliver and process device messages!");
            return BACKEND_PROXY_PROSESS_ERROR;
        }
        ret = backend_proxy_dev_msg_prosess(msg_ptr);
    }else if(PROXY_MSG_TYPE_STRGY == msg_type){
    /* 
     * The frontend proxy delivers strategy messages from the frontend admin session to the backend proxy for the backend admin session.
     */
        if (frontend_sess_id != FRONTEND_ADMIN_SESSION_ID || backend_sess_id != BACKEND_ADMIN_SESSION_ID){
            error_print("Only admin sessions can deliver and process strategy messages!");
            return BACKEND_PROXY_PROSESS_ERROR;
        }
        ret = backend_proxy_strgy_msg_process(msg_ptr);
    }else if(PROXY_MSG_TYPE_SESS == msg_type){
/*
 * If the frontend wants to establish a session between the frontend and backend, the frontend_sess_id in the handover request should be 
 * FRONTEND_HANDOVER_SESSION_ID, and the backend_sess_id should be BACKEND_HANDOVER_SESSION_ID in proxy message.
 */
         if (frontend_sess_id != FRONTEND_HANDOVER_SESSION_ID || backend_sess_id != BACKEND_HANDOVER_SESSION_ID){
            error_print("The front end id in handover request message should be FRONTEND_HANDOVER_SESSION_ID， and the backend_sess_id should be BACKEND_HANDOVER_SESSION_ID!");
            return BACKEND_PROXY_PROSESS_ERROR;
        }
        ret = backend_proxy_sess_msg_process(msg_ptr);
    }else{
/*
 * When msg_type is PROXY_MSG_TYPE_DATA, the frontend_sess_id and backend_sess_id should be checked to determine whether the session (if it exists) 
 * is an application session.
 */
         if (!APP_SESSION_ID_VALID(frontend_sess_id) || !APP_SESSION_ID_VALID(backend_sess_id)){
            error_print("Both the frontend session ID and backend session ID in the proxy data message must pass the application session ID validation!");
            return BACKEND_PROXY_PROSESS_ERROR;
        }
        ret = backend_proxy_data_msg_prosess(frontend_sess_id, backend_sess_id, msg_ptr);
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


int backend_proxy_data_msg_prosess(uint32_t frontend_sess_id, uint32_t backend_sess_id, uint8_t *msg){
    int ret;

    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}

int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROSESS_OK;
}