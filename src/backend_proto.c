#include "backend_proto.h"
#include "engine.h"


int backend_proxy_msg_process(uint8_t *msg){
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
        return BACKEND_PROXY_PROCESS_ERROR;
    }// Unsupported message type.

/*
 * Check the validity of the message length.
 */
    if(!PROXY_MSG_LEN_VALID(msg_type)){
        error_print("Message length error!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }// Unsupported message type.

    msg_ptr = (uint8_t *)proxy_msg_hdr;
    msg_ptr += PROXY_MSG_HDR_SIZE;
    if(PROXY_MSG_TYPE_DEV == msg_type){
    /* 
     * The frontend proxy delivers device messages from the frontend admin session to the backend proxy for the backend admin session.
     */
        if (frontend_sess_id != FRONTEND_ADMIN_SESSION_ID || backend_sess_id != BACKEND_ADMIN_SESSION_ID){
            error_print("Only admin sessions can deliver and process device messages!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        ret = backend_proxy_dev_msg_process(msg_ptr);
    }else if(PROXY_MSG_TYPE_STRGY == msg_type){
    /* 
     * The frontend proxy delivers strategy messages from the frontend admin session to the backend proxy for the backend admin session.
     */
        if (frontend_sess_id != FRONTEND_ADMIN_SESSION_ID || backend_sess_id != BACKEND_ADMIN_SESSION_ID){
            error_print("Only admin sessions can deliver and process strategy messages!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        ret = backend_proxy_strgy_msg_process(msg_ptr);
    }else if(PROXY_MSG_TYPE_SESS == msg_type){
/*
 * If the frontend wants to establish a session between the frontend and backend, the frontend_sess_id in the handover request should be 
 * FRONTEND_HANDOVER_SESSION_ID, and the backend_sess_id should be BACKEND_HANDOVER_SESSION_ID in proxy message.
 */
         if (frontend_sess_id != FRONTEND_HANDOVER_SESSION_ID || backend_sess_id != BACKEND_HANDOVER_SESSION_ID){
            error_print("The front end id in handover request message should be FRONTEND_HANDOVER_SESSION_ID， and the backend_sess_id should be BACKEND_HANDOVER_SESSION_ID!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        ret = backend_proxy_sess_msg_process(msg_ptr);
    }else{
/*
 * When msg_type is PROXY_MSG_TYPE_DATA, the frontend_sess_id and backend_sess_id should be checked to determine whether the session (if it exists) 
 * is an application session.
 */
         if (!APP_SESSION_ID_VALID(frontend_sess_id) || !APP_SESSION_ID_VALID(backend_sess_id)){
            error_print("Both the frontend session ID and backend session ID in the proxy data message must pass the application session ID validation!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        ret = backend_proxy_data_msg_prosess(frontend_sess_id, backend_sess_id, msg_ptr);
    }

    return BACKEND_PROXY_PROCESS_OK;
}



int backend_proxy_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}

/*
 * Device message processing functions.
 * backend_proxy_dev_msg_process
 *     |->backend_proxy_dev_msg_process_ver1
 *         |->backend_proxy_dev_msg_process_disable_ver1
 *         |->backend_proxy_dev_msg_process_enable_ver1
 *         |->backend_proxy_dev_msg_process_query_ver1
 */
int backend_proxy_dev_msg_process(uint8_t *msg){
    DevMsgHeader *dev_msg_hdr;
    uint16_t version, msg_id, payload_len;
    DevMsgType msg_type;
    ActionType action_type;
    int ret;
    uint8_t *msg_data;

    ret = BACKEND_PROXY_PROCESS_ERROR;

    dev_msg_hdr = (DevMsgHeader *)msg;

    if(NULL == dev_msg_hdr){
        error_print("backend_proxy_dev_msg_process() returns an error because the msg pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    version = dev_msg_hdr->version;
    msg_type = dev_msg_hdr->msg_type;
    msg_id = dev_msg_hdr->msg_id;
    action_type = dev_msg_hdr->action_type;
    payload_len = dev_msg_hdr->payload_len;
    msg_data = msg + sizeof(DevMsgHeader);

/*    
 * The backend protocol stack only processes messages where the action_type is ACTION_TYPE_COMMAND.
 */
    if(ACTION_TYPE_COMMAND != action_type){
        error_print("Backend protocol stack only process the device message which the signaling type!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Before processing device messages, the protocol stack should check the validity of parameters.
 *
 * Currently, the protocol stack only processes Version 1 device messages.
 */
    if(PROXY_PROTO_DEV_VERSION_1 == version){
        ret = backend_proxy_dev_msg_process_ver1(msg_type, msg_id, action_type, payload_len, msg_data);
    } 

    return ret;
}


int backend_proxy_dev_msg_process_ver1(uint32_t msg_type, uint32_t msg_id, uint32_t action_type, uint32_t payload_len, uint8_t *msg_payload){  
    int corr_len;
    int ret = BACKEND_PROXY_PROCESS_ERROR;
    uint16_t dev_msg_resp;


/* 
 * Check whether the payload length matches the message type and signaling type.
 */
    corr_len = DEV_MSG_PAYLOAD_LEN(msg_type, action_type);

    if(corr_len != (int)payload_len){
            error_print("payload len is not valid!");
            return BACKEND_PROXY_PROCESS_ERROR;
    }

    switch(msg_type) {
        case DEV_MSG_DISABLE:
            ret = backend_proxy_dev_msg_process_disable_ver1(payload_len, msg_payload);
            break;
        case DEV_MSG_ENABLE:
            ret = backend_proxy_dev_msg_process_enable_ver1(payload_len, msg_payload);
            break;
        case DEV_MSG_QUERY:
            ret = backend_proxy_dev_msg_process_query_ver1(payload_len, msg_payload);
            break;
        default:
/*
 * Nothing to do, because the validation of the msg_type is checked in DEV_MSG_PAYLOAD_LEN.
 */
            break;
    }

    return ret;
}


int backend_proxy_dev_msg_process_enable_ver1(uint32_t payload_len,  uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}
int backend_proxy_dev_msg_process_disable_ver1(uint32_t payload_len, uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}
int backend_proxy_dev_msg_process_query_ver1(uint32_t payload_len, uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_dev_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}
/*
 * Strategy message processing functions.
 * backend_proxy_strgy_msg_process
 *     |->backend_proxy_strgy_msg_process_ver1
 *         |->backend_proxy_strgy_msg_process_set_ver1
 *         |->backend_proxy_strgy_msg_process_query_ver1
 */
int backend_proxy_strgy_msg_process(uint8_t *msg){
    StrgyMsgHeader *strgymsg_hdr;
    uint16_t version, msg_id, payload_len;
    StrgyMsgType msg_type;
    ActionType action_type;
    int ret;
    uint8_t *msg_data;

    ret = BACKEND_PROXY_PROCESS_ERROR;

    strgymsg_hdr = (StrgyMsgHeader *)msg;
#if 0
    if(NULL == dev_msg_hdr)
        return BACKEND_PROXY_PROCESS_ERROR;

    version = dev_msg_hdr->version;
    msg_type = dev_msg_hdr->msg_type;
    msg_id = dev_msg_hdr->msg_id;
    action_type = dev_msg_hdr->action_type;
    payload_len = dev_msg_hdr->payload_len;
    msg_data = msg + sizeof(DevMsgHeader);
#endif

    return ret;
}

int backend_proxy_strgy_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}

int backend_proxy_sess_msg_process(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}

int backend_proxy_sess_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_data_msg_prosess(uint32_t frontend_sess_id, uint32_t backend_sess_id, uint8_t *msg){
    int ret;

    return BACKEND_PROXY_PROCESS_OK;
}

int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}

int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}