#include "backend_proto.h"
#include "engine.h"
#include "message.h"

int backend_proxy_msg_process(uint8_t *msg){
    ProxyMsgHeader *proxy_msg_hdr;
    int proxy_proto_ver, msg_type, msg_len, ret;
    uint16_t frontend_sess_id, backend_sess_id;
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
 * If the frontend wants to establish a session between the frontend and backend, the backend_sess_id in the handover request should be 
 * FRONTEND_HANDOVER_SESSION_ID in proxy message.
 */

 #if 0
         if (frontend_sess_id != FRONTEND_HANDOVER_SESSION_ID || backend_sess_id != BACKEND_HANDOVER_SESSION_ID){
            error_print("The front end id in handover request message should be FRONTEND_HANDOVER_SESSION_ID， and the backend_sess_id should be BACKEND_HANDOVER_SESSION_ID!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
#endif
        ret = backend_proxy_sess_msg_process(frontend_sess_id, backend_sess_id, msg_ptr);
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
        error_print("backend_proxy_dev_msg_process() returns an error, returns an error, \
                     because the backend protocol stack only processes the device message of the signaling type!");
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


int backend_proxy_dev_msg_process_ver1(uint16_t msg_type, uint16_t msg_id, uint16_t action_type, uint16_t payload_len, uint8_t *msg_payload){  
    int corr_len;
    int ret = BACKEND_PROXY_PROCESS_ERROR;

/* 
 * Check whether the payload length matches the message type and signaling type.
 */
    corr_len = DEV_MSG_PAYLOAD_LEN(msg_type, action_type);

    if(PROXY_MSG_INVALID_LEN == corr_len || corr_len != payload_len){
            error_print("backend_proxy_dev_msg_process_ver1() returns an error, because msg_type or payload length is not valid!");
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
 * Nothing to do, because the validation of the msg_type is checked before.
 */
            break;
    }

    return ret;
}


int backend_proxy_dev_msg_process_enable_ver1(uint16_t payload_len,  uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}
int backend_proxy_dev_msg_process_disable_ver1(uint16_t payload_len, uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}
int backend_proxy_dev_msg_process_query_ver1(uint16_t payload_len, uint8_t *msg_payload){
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

    if(NULL == strgymsg_hdr){
        error_print("backend_proxy_strgy_msg_process() returns an error because the msg pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    version = strgymsg_hdr->version;
    msg_type = strgymsg_hdr->msg_type;
    msg_id = strgymsg_hdr->msg_id;
    action_type = strgymsg_hdr->action_type;
    payload_len = strgymsg_hdr->payload_len;
    msg_data = msg + sizeof(StrgyMsgHeader);

/*    
 * The backend protocol stack only processes messages where the action_type is ACTION_TYPE_COMMAND.
 */
    if(ACTION_TYPE_COMMAND != action_type){
        error_print("backend_proxy_strgy_msg_process() returns an error, returns an error, \
                     because the backend protocol stack only processes the device message of the signaling type!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Before processing device messages, the protocol stack should check the validity of parameters.
 *
 * Currently, the protocol stack only processes Version 1 device messages.
 */
    if(PROXY_PROTO_STRGY_VERSION_1 == version){
        ret = backend_proxy_strgy_msg_process_ver1(msg_type, msg_id, action_type, payload_len, msg_data);
    } 

    return ret;
}


int backend_proxy_strgy_msg_process_ver1(uint16_t msg_type, uint16_t msg_id, uint16_t action_type, uint16_t payload_len, uint8_t *msg_payload){
    int corr_len;
    int ret = BACKEND_PROXY_PROCESS_ERROR;

    /* 
 * Check whether the payload length matches the message type and signaling type.
 */
    corr_len = STRGY_MSG_PAYLOAD_LEN(msg_type, action_type);

    if(PROXY_MSG_INVALID_LEN == corr_len || corr_len != payload_len){
            error_print("backend_proxy_strgy_msg_process_ver1() returns an error, because msg_type or payload length is not valid!");
            return BACKEND_PROXY_PROCESS_ERROR;
    }


    switch(msg_type) {
        case STRGY_MSG_SET:
            ret = backend_proxy_strgy_msg_process_set_ver1(payload_len, msg_payload);
            break;
        case STRGY_MSG_QUERY:
            ret = backend_proxy_strgy_msg_process_query_ver1(payload_len, msg_payload);
            break;
        default:
/*
 * Nothing to do, because the validation of the msg_type is checked before.
 */
            break;
    }

    return ret;
}


int backend_proxy_strgy_msg_process_set_ver1(uint16_t payload_len, uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_strgy_msg_process_query_ver1(uint16_t payload_len, uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}

int backend_proxy_strgy_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}

/*
 * Session message processing functions.
 * backend_proxy_sess_msg_process
 *     |->backend_proxy_sess_msg_process_ver1
 *         |->backend_proxy_sess_msg_process_create_ver1
 *         |->backend_proxy_sess_msg_process_close_ver1
 */

int backend_proxy_sess_msg_process(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint8_t *msg){
    SessMsgHeader *sess_msg_hdr;
    uint16_t version, payload_len;
    SessMsgType msg_type;
    ActionType action_type;
    SessIpProtoVersion ip_version;
    int ret;
    uint8_t *msg_data;

    ret = BACKEND_PROXY_PROCESS_ERROR;

    sess_msg_hdr = (SessMsgHeader *)msg;

    if(NULL == sess_msg_hdr){
        error_print("backend_proxy_sess_msg_process() returns an error because the msg pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    version = sess_msg_hdr->version;
    msg_type = sess_msg_hdr->msg_type;
    action_type = sess_msg_hdr->action_type;
    ip_version = sess_msg_hdr->ip_version;
    payload_len = sess_msg_hdr->payload_len;
    msg_data = msg + sizeof(SessMsgHeader);
 #if 0
         if (frontend_sess_id != FRONTEND_HANDOVER_SESSION_ID || backend_sess_id != BACKEND_HANDOVER_SESSION_ID){
            error_print("The front end id in handover request message should be FRONTEND_HANDOVER_SESSION_ID， and the backend_sess_id should be BACKEND_HANDOVER_SESSION_ID!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
#endif

/*    
 * The backend protocol stack only processes messages where the action_type is ACTION_TYPE_COMMAND.
 */
    if(ACTION_TYPE_COMMAND != action_type){
        error_print("backend_proxy_sess_msg_process() returns an error, returns an error, \
                     because the backend protocol stack only processes the device message of the signaling type!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Before processing device messages, the protocol stack should check the validity of parameters.
 *
 * Currently, the protocol stack only processes Version 1 device messages.
 */
    if(PROXY_PROTO_SESS_VERSION_1 == version){
        ret = backend_proxy_sess_msg_process_ver1(frontend_sess_id, backend_sess_id, msg_type, ip_version, action_type, payload_len, msg_data);
    } 

    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_sess_msg_process_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t msg_type, 
                                        uint16_t action_type, uint16_t ip_version, uint16_t payload_len, 
                                        uint8_t *msg_payload){
    int corr_len;
    int ret = BACKEND_PROXY_PROCESS_ERROR;

/* 
 * Check whether the payload length matches the message type and signaling type.
 */
    corr_len = SESS_MSG_PAYLOAD_LEN(msg_type, action_type, ip_version);

    if(PROXY_MSG_INVALID_LEN == corr_len || corr_len != payload_len){
            error_print("backend_proxy_sess_msg_process_ver1() returns an error, because msg_type or payload length is not valid!");
            return BACKEND_PROXY_PROCESS_ERROR;
    }

    switch(msg_type) {
        case SESS_MSG_CREATE:
            if (BACKEND_HANDOVER_SESSION_ID != backend_sess_id){
                error_print("The backend_sess_id should be BACKEND_HANDOVER_SESSION_ID in a session message whose type is SESS_MSG_CREATE!");
                ret = BACKEND_PROXY_PROCESS_ERROR;
            }else{
                ret = backend_proxy_sess_msg_process_create_ver1(frontend_sess_id, backend_sess_id, ip_version, payload_len, msg_payload);
            }
            break;
        case SESS_MSG_CLOSE:
            ret = backend_proxy_sess_msg_process_close_ver1(frontend_sess_id, backend_sess_id, payload_len, msg_payload);
            break;
        default:
/*
 * Nothing to do, because the validation of the msg_type is checked before.
 */
            break;
    }


    return ret;
}


int backend_proxy_sess_msg_process_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
  
    return __backend_proxy_sess_msg_process_create_ver1(frontend_sess_id, backend_sess_id, ip_version, payload_len, msg_payload);
}


int __backend_proxy_sess_msg_process_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
    struct BackendSessionPool *pool;
    SessParaIPv4 *para_ipv4;
    SessParaIPv6 *para_ipv6;
    struct SessMsgPara sess_para;

/*
 * The main body of the session creation procedure lies in the function which the create_sess pointer points to.
 * In __backend_proxy_sess_msg_process_create_ver1, this function parses the session parameters and calls the function pointed to by the create_sess pointer to establish a new session.
*/
    pool = get_backend_high_speed_pool();


    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_sess_msg_process_close_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t payload_len, uint8_t *msg_payload){
    return BACKEND_PROXY_PROCESS_OK;
}



int backend_proxy_sess_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}



int backend_proxy_data_msg_prosess(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint8_t *msg){
    int ret;

    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}