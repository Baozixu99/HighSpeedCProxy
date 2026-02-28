#include "backend_proto.h"
#include "engine.h"
#include "message.h"

uint8_t global_amp_tx_buf[HYPERAMP_MSG_HDR_PLUS_MAX_SIZE];

int backend_proxy_msg_process(uint8_t *msg){
    ProxyMsgHeader *proxy_msg_hdr;
    int proxy_proto_ver, msg_len, ret;
    ProxyMsgType msg_type;
    uint16_t frontend_sess_id, backend_sess_id;
    uint8_t *msg_ptr;

    struct BackendSession* sess;

    proxy_msg_hdr = (ProxyMsgHeader *)msg;

/*
 * Currently, the backend protocol stack does not differentiate the protocol version, we reserve the protocol version for future extensions.
 */
    proxy_proto_ver     = proxy_msg_hdr->version;
    frontend_sess_id    = proxy_msg_hdr->frontend_sess_id;
    backend_sess_id     = proxy_msg_hdr->backend_sess_id;
    msg_type            = proxy_msg_hdr->proxy_msg_type;
    msg_len             = proxy_msg_hdr->payload_len;

    utils_print("In %s, version = %d, frontend sess id = %d, backend sess id = %d, msg_type = %d, msg_len = %d\n", 
                __func__, proxy_proto_ver, frontend_sess_id, backend_sess_id, msg_type, msg_len);
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
#if 0
/*
 * If the frontend wants to establish a session between the frontend and backend, the backend_sess_id in the handover request should be 
 * FRONTEND_HANDOVER_SESSION_ID in proxy message.
 */
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
        ret = backend_proxy_data_msg_prosess(frontend_sess_id, backend_sess_id, msg_len, msg_ptr);
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
        error_print("backend_proxy_dev_msg_process() failed: the msg pointer is NULL!\n");
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
        error_print("backend_proxy_dev_msg_process() failed: the backend protocol stack only processes the device message of the ACTION_TYPE_COMMAND type!");
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

    utils_print("In %s, corr_len = %d, payload_len = %d\n", __func__, corr_len, payload_len);

    if(PROXY_MSG_INVALID_LEN == corr_len || corr_len != payload_len){
            error_print("backend_proxy_dev_msg_process_ver1() failed: the msg_type or payload length is not valid!");
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
    utils_print("In %s\n", __func__);

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
        error_print("backend_proxy_strgy_msg_process() failed: the msg pointer is NULL!\n");
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
        error_print("backend_proxy_strgy_msg_process() faild: the backend protocol stack only processes the strategy message of the ACTION_TYPE_COMMAND type!");
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

    utils_print("In %s, corr_len = %d, payload_len = %d\n", __func__, corr_len, payload_len);

    if(PROXY_MSG_INVALID_LEN == corr_len || corr_len != payload_len){
        error_print("backend_proxy_strgy_msg_process_ver1() failed: the msg_type or payload length is not valid!");
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
    utils_print("In %s\n", __func__);
    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_strgy_msg_process_query_ver1(uint16_t payload_len, uint8_t *msg_payload){
    utils_print("In %s\n", __func__);
    return BACKEND_PROXY_PROCESS_OK;
}

int backend_proxy_strgy_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}



 /**
 * @brief Processes session messages in the backend proxy
 * @details This function serves as the core handler for session-related communication between front and backend proxies.
 *          It handles general session message processing by parsing the incoming message, coordinating with the specified
 *          frontend and backend sessions, and executing appropriate operations based on message content. 
 *          The processing follows a hierarchical call structure:
 *          backend_proxy_sess_msg_process
 *              |-> backend_proxy_sess_msg_process_ver1
 *                  |-> backend_proxy_sess_msg_process_active_create_ver1
 *                  |-> backend_proxy_sess_msg_process_close_ver1
 * @param[in] frontend_sess_id 16-bit identifier of the frontend session, used to map the message to 
 *                             the corresponding frontend request context.
 * @param[in] backend_sess_id 16-bit identifier of the backend session, used to associate the message 
 *                            with the relevant backend session state and resources.
 * @param[in] msg Pointer to the session message data to be processed, containing the complete 
 *                message content (e.g., operation type, parameters, metadata). Must not be NULL.
 * @return int Execution result: Returns BACKEND_PROXY_PROCESS_OK on successful message processing;
 *         returns BACKEND_PROXY_PROCESS_ERROR if message parsing fails, session identifiers are invalid, or 
 *         the requested operation cannot be completed.
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

    utils_print("In %s\n", __func__);
    utils_print("In %s, version = %d, msg_type = %d, action type = %d, ip version = %d, payload len = %d, address = %p\n", 
                 __func__, version, msg_type, action_type, ip_version, payload_len, &sess_msg_hdr);
 #if 0
         if (frontend_sess_id != FRONTEND_HANDOVER_SESSION_ID || backend_sess_id != BACKEND_HANDOVER_SESSION_ID){
            error_print("The front end id in handover request message should be FRONTEND_HANDOVER_SESSION_ID， and the backend_sess_id should be BACKEND_HANDOVER_SESSION_ID!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
#endif

/*
 * The backend protocol stack only processes messages whose action_type is either  ACTION_TYPE_COMMAND or ACTION_TYPE_RESPONSE. Messages with other action_type values 
 * are not supported and will trigger an error.
 */
    if(ACTION_TYPE_COMMAND != action_type && ACTION_TYPE_RESPONSE != action_type){
        error_print("backend_proxy_sess_msg_process() error: unsupported action_type. Only ACTION_TYPE_COMMAND and ACTION_TYPE_RESPONSE are processed by the backend protocol stack.\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Before processing session messages, the protocol stack should check the validity of parameters.
 *
 * Currently, the protocol stack only processes Version 1 device messages.
 */
    
    SessParaIPv4 *debug_hdr = (SessParaIPv4 *)msg_data;
    SessIPv4Params *debug_hdr2 = (SessIPv4Params *)msg_data;
    utils_print("In %s, type is SessParaIPv4, dev_id = %d, trans_proto = %d, port = %d\n", __func__, debug_hdr->dev_id, debug_hdr->trans_proto, debug_hdr->port);
    utils_print("In %s, type is SessIPv4Params, devive_selection = %d, transport_layer_proto = %d\n", 
                __func__, debug_hdr2->device_selection, debug_hdr2->transport_layer_proto);

    if(PROXY_PROTO_SESS_VERSION_1 == version){
        ret = backend_proxy_sess_msg_process_ver1(frontend_sess_id, backend_sess_id, msg_type, action_type, ip_version, payload_len, msg_data);
    } 

    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_sess_msg_process_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t msg_type, 
                                        uint16_t action_type, uint16_t ip_version, uint16_t payload_len, 
                                        uint8_t *msg_payload){
    int                             corr_len;
    int                             ret = BACKEND_PROXY_PROCESS_ERROR;

/* 
 * Check whether the payload length matches the message type and signaling type.
 */

    utils_print("In %s\n", __func__);

    corr_len = SESS_MSG_PAYLOAD_LEN(msg_type, action_type, ip_version);

    utils_print("corr_len = %d, payload_len = %d\n", corr_len, payload_len);

    if(PROXY_MSG_INVALID_LEN == corr_len || corr_len != payload_len){
            error_print("backend_proxy_sess_msg_process_ver1() returns an error, because msg_type or payload length is not valid!");
            return BACKEND_PROXY_PROCESS_ERROR;
    }

    switch(msg_type) {
        case SESS_MSG_CREATE:
            if (BACKEND_HANDOVER_SESSION_ID != backend_sess_id){
/*
 * Passive session creation procedure.
 */
                if(action_type != ACTION_TYPE_RESPONSE){
                    error_print("backend_proxy_sess_msg_process_ver1 failed: If the backend_sess_id in a SESS_MSG_CREATE message has been previously allocated \ 
                                 (passive session creation), the action_type must be ACTION_TYPE_RESPONSE!\n");
                    ret = BACKEND_PROXY_PROCESS_ERROR;
                }else{
                    ret = backend_proxy_sess_msg_process_passive_create_ver1(frontend_sess_id, backend_sess_id, ip_version, payload_len, msg_payload);
                }
                
            }else{
                ret = backend_proxy_sess_msg_process_active_create_ver1(frontend_sess_id, backend_sess_id, ip_version, payload_len, msg_payload);
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

/**
 * @brief Processes version 1 of ACTIVE session creation messages in the backend proxy
 * @details This function handles the processing logic for version 1 session creation messages where the **frontend actively sends a request to the backend**.
 *          It typically parses the message payload, performs necessary validation, and executes session creation operations 
 *          for backend sessions initiated by frontend requests (i.e., the frontend proactively triggers session creation by sending a request to the backend).
 *          It is responsible for coordinating frontend-backend session mapping for version 1 message format, and will set the 
 *          `establish_type` attribute of the BackendSession struct to BACKEND_SESS_ESTABLISH_ACTIVE during session creation.
 * @param[in] frontend_sess_id 16-bit identifier of the frontend session, used to associate with the corresponding frontend request
 * @param[in] backend_sess_id 16-bit identifier of the backend session, used for backend-side session tracking and management
 * @param[in] ip_version 16-bit value indicating the IP protocol version (e.g., IPv4 or IPv6) used in the session
 * @param[in] payload_len 16-bit length of the message payload (in bytes), specifying the size of the data in msg_payload
 * @param[in] msg_payload Pointer to the message payload data, containing the detailed content of the version 1 active session creation request (initiated by frontend to backend)
 * @return int Execution result: Typically returns BACKEND_PROXY_PROCESS_OK on successful processing (active session created in response to frontend request),
 *         or BACKEND_PROXY_PROCESS_ERROR if validation fails, payload is invalid, or session creation triggered by frontend request encounters issues.
 */
int backend_proxy_sess_msg_process_active_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
  
    return __backend_proxy_sess_msg_process_active_create_ver1(frontend_sess_id, backend_sess_id, ip_version, payload_len, msg_payload);
}


int __backend_proxy_sess_msg_process_active_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
    struct BackendSessionPool *pool;
    struct BackendSession *sess;
    SessParaIPv4 *para_ipv4;
    SessParaIPv6 *para_ipv6;
    IPv4PortTuple *ipv4_port_tuple;
    IPv6PortTuple *ipv6_port_tuple;
    struct IPv4Address *ipv4_addr;
    struct IPv6Address *ipv6_addr;
    struct SessMsgPara sess_para;
    bool ip_ver_valid = true;
    int ret;

/*
 * The main body of the session creation procedure lies in the function which the create_sess pointer points to.
 * In __backend_proxy_sess_msg_active_process_create_ver1, this function parses the session parameters and calls the function pointed to by the create_sess pointer to establish a new session.
 */
    pool = get_backend_high_speed_pool();

    if(NULL == pool){
        error_print("__backend_proxy_sess_msg_process_active_create_ver1 faield: the high speed pool is not initialized!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess_para.frontend_sess_id  = frontend_sess_id;

    if(SESS_IPV4_PROTO == ip_version){
        if(payload_len != sizeof(SessParaIPv4)){
            error_print("__backend_proxy_sess_msg_process_active_create_ver1 failed: the payload length does not match the IPv4 handover message!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }

        para_ipv4                   = (SessParaIPv4 *)msg_payload;
        sess_para.dev_id            = para_ipv4->dev_id;
        utils_print("Before set trans_proto,  sess_para.trans_proto = %d, para_ipv4->trans_proto = %d\n", sess_para.trans_proto, para_ipv4->trans_proto);
        sess_para.trans_proto       = para_ipv4->trans_proto;
        utils_print("After set trans_proto,  sess_para.trans_proto = %d, para_ipv4->trans_proto = %d\n", sess_para.trans_proto, para_ipv4->trans_proto);
        sess_para.ip_version        = SESS_IPV4_PROTO;

        utils_print("In %s\n", __func__);
        DUMP_BUFFER_CONTENT(&para_ipv4->ipv4_addr, sizeof(struct IPv4Address), "%d");

        ipv4_port_tuple             = &sess_para.ip_port_tuple.ipv4_port_tuple;
        ipv4_addr                   = &para_ipv4->ipv4_addr;
        memcpy(&ipv4_port_tuple->ipv4_addr, ipv4_addr, sizeof(struct IPv4Address));
        ipv4_port_tuple->port       = para_ipv4->port;

        utils_print("The content of the ipv4_addr is: \n");
        DUMP_BUFFER_CONTENT(ipv4_addr, sizeof(struct IPv4Address), "%d");

        utils_print("The content of the ipv4_port_tuple->ipv4_addr is: \n");
        DUMP_BUFFER_CONTENT(&ipv4_port_tuple->ipv4_addr, sizeof(struct IPv4Address), "%d");
        
        utils_print("In sess_para, the dev_id = %d, trans_proto = %d,ip_version = %d\n", sess_para.dev_id, sess_para.trans_proto, sess_para.ip_version);
    }else if(SESS_IPV6_PROTO == ip_version){
        if(payload_len != sizeof(SessParaIPv6)){
            error_print("__backend_proxy_sess_msg_process_active_create_ver1 failed: payload length does not match the message type!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        para_ipv6                   = (SessParaIPv6 *)msg_payload;
        sess_para.dev_id            = para_ipv6->dev_id;
        sess_para.trans_proto       = para_ipv6->trans_proto;
        sess_para.ip_version        = SESS_IPV6_PROTO;

        ipv6_port_tuple             = &sess_para.ip_port_tuple.ipv6_port_tuple;
        ipv6_addr                   = &para_ipv6->ipv6_addr;
        memcpy(&ipv6_port_tuple->ipv6_addr, ipv6_addr, sizeof(struct IPv6Address));
        ipv6_port_tuple->port       = para_ipv6->port;

    }else{
        ip_ver_valid = false;
    }
    
    if(true == ip_ver_valid){
/*
 * If the session creation function does not exist, print an error message (indicating create_sess does not point to a valid create-session function) .
 * The create_sess pointer points to the function that actually creates a socket, a session object and binds them together.
 */

        utils_print("In %s, the address of the engine is %p, %p\n", __func__, pool->engine, get_global_backend_engine());

        if(!pool->ops->create_sess_active){
            error_print("__backend_proxy_sess_msg_process_active_create_ver1 failed: the create_sess does not point to a valid create-session function!\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        ret = pool->ops->create_sess_active(pool, &sess, &sess_para);
    }else{
        error_print("__backend_proxy_sess_msg_process_active_create_ver1 failed: the IP version is not valid!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return ret;
}


/**
 * @brief Processes version 1 of PASSIVE session creation messages in the backend proxy
 * @details This function handles the processing logic for version 1 session creation messages where the **backend passively accepts and processes a session creation request (triggered by external client connections)**.
 *          It typically parses the message payload, performs necessary validation, and executes session creation operations 
 *          for backend sessions initiated by passive connection establishment (i.e., the backend passively completes session creation in response to external client connections, rather than being triggered by proactive frontend requests).
 *          It is responsible for coordinating frontend-backend session mapping for version 1 message format, and will set the 
 *          `establish_type` attribute of the BackendSession struct to BACKEND_SESS_ESTABLISH_PASSIVE during session creation.
 * @param[in] frontend_sess_id 16-bit identifier of the frontend session, used to associate with the corresponding frontend response for passive session mapping
 * @param[in] backend_sess_id 16-bit identifier of the backend session, used for backend-side passive session tracking and management (pre-allocated for passive connection scenarios)
 * @param[in] ip_version 16-bit value indicating the IP protocol version (e.g., IPv4 or IPv6) used in the passive session
 * @param[in] payload_len 16-bit length of the message payload (in bytes), specifying the size of the data in msg_payload
 * @param[in] msg_payload Pointer to the message payload data, containing the detailed content of the version 1 passive session creation request (triggered by backend passive acceptance of external client connections)
 * @return int Execution result: Typically returns BACKEND_PROXY_PROCESS_OK on successful processing (passive session created in response to external client connection),
 *         or BACKEND_PROXY_PROCESS_ERROR if validation fails, payload is invalid, or session creation triggered by backend passive connection encounters issues.
 */
int backend_proxy_sess_msg_process_passive_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
    return __backend_proxy_sess_msg_process_passive_create_ver1(frontend_sess_id, backend_sess_id, ip_version, payload_len, msg_payload);
}


int __backend_proxy_sess_msg_process_passive_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
    struct BackendEngine_           *eng;
    struct BackendSessionPool       *s_pool;
    struct BackendSessionPoolOps    *ops;
    struct BackendSession           *sess;
    struct SharedMemoryPool         *mem_pool;
    int                             ret;



    utils_print("In %s\n", __func__);

    eng = get_global_backend_engine();

    if(NULL == eng || NULL == eng->sess_pool || NULL == eng->mem_pool || NULL == eng->sess_pool->ops){
        error_print("__backend_proxy_sess_msg_process_passive_create_ver1 failed: eng, eng->sess_pool, eng->mem_pool, or eng->sess_pool->ops is NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    s_pool      = eng->sess_pool;
    ops         = eng->sess_pool->ops;
    mem_pool    = eng->mem_pool;

    if(NULL == ops->search_sess){
        error_print("__backend_proxy_sess_msg_process_passive_create_ver1 failed: ops->search_sess (session searching function) is not initialized!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess = ops->search_sess(s_pool, backend_sess_id);

    if(NULL == sess){
        error_print("__backend_proxy_sess_msg_process_passive_create_ver1 failed: no backend session found for the specified backend_sess_id!");
        return BACKEND_PROXY_PROCESS_ERROR;   
    }

    if(SESS_ESTABLISH_PASSIVE != sess->establish_mode){
        error_print("__backend_proxy_sess_msg_process_passive_create_ver1 failed: backend session's establish_mode is not SESS_ESTABLISH_PASSIVE (mismatched session establishment mode)!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    utils_print("frontend id = %d, backend id = %d\n", sess->frontend_sess_id, sess->backend_sess_id);

/*
 * Perhaps the socket associated with this session instance should be added to the epoll interest list here? 
 * This operation requires further careful consideration:
 *     1) Handling logic after EPOLLIN event triggering (e.g., data reading, session state update);
 *     2) Resource management of the epoll instance (e.g., maximum file descriptor limit, duplicate registration avoidance);
 *     3) Exception handling for epoll registration failure (e.g., ret return value check and fallback strategy);
 *     4) Dynamic adjustment of monitored events (e.g., adding EPOLLOUT when writing data to the socket).
 */
    BACKEND_SESS_REGISTER_EPOLL(sess, EPOLLIN, &ret);

    return ret;
}


/**
 * @brief Processes version 1 of session close messages in the backend proxy
 * @details This function handles the processing logic for version 1 session close messages.
 *          It parses the message payload, validates the session identifiers, performs session closure operations,
 *          and cleans up associated resources. It is specifically designed for version 1 of the session close message format,
 *          coordinating frontend-backend session termination.
 * @param[in] frontend_sess_id 16-bit identifier of the frontend session, used to map the close request to the corresponding frontend context
 * @param[in] backend_sess_id 16-bit identifier of the backend session, specifying the backend session to be closed
 * @param[in] payload_len 16-bit length of the message payload (in bytes), indicating the size of the data in msg_payload
 * @param[in] msg_payload Pointer to the session close message payload, containing detailed parameters for the close operation. Must not be NULL.
 * @return int Execution result: Returns BACKEND_PROXY_PROCESS_OK on successful processing of the close message and session termination;
 *         Returns BACKEND_PROXY_PROCESS_ERROR if payload is invalid, session identifiers are incorrect, parsing fails, or closure operations encounter errors.
 */
int backend_proxy_sess_msg_process_close_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t payload_len, uint8_t *msg_payload){
    return __backend_proxy_sess_msg_process_close_ver1(frontend_sess_id, backend_sess_id, payload_len, msg_payload);
}


int __backend_proxy_sess_msg_process_close_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t payload_len, uint8_t *msg_payload){
    struct BackendSessionPool   *pool;
    struct BackendSession       *sess;
    int                         ret;

/*
 * The main body of the session closure procedure lies in the function which the delete_sess pointer points to.
 * In __backend_proxy_sess_msg_process_close_ver1, this function parses the session parameters and calls the function pointed to by the delete_sess pointer to establish a new session.
 */
    pool = get_backend_high_speed_pool();

    if(NULL == pool || NULL == pool->ops){
        error_print("__backend_proxy_sess_msg_process_close_ver1 failed: the high-speed pool is not initialized!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == pool->ops->search_sess || NULL == pool->ops->delete_sess){
        error_print("__backend_proxy_sess_msg_process_close_ver1 failed:  search_sess or delete_sess is NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * First of all, search for the session instance according to the backend session ID.
 */
    sess = pool->ops->search_sess(pool, backend_sess_id);

    if(NULL == sess){
        error_print("backend_proxy_sess_msg_process_close_ver1 failed: session not found by backend_sess_id!");
        goto sess_not_found;
    }
/*
 * Close the session.
 */
    ret = pool->ops->delete_sess(pool, sess);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("backend_proxy_sess_msg_process_close_ver1 failed: the session deletion procedure did not terminate gracefully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;

sess_not_found:
/*
 * Pending: Whether to respond to the frontend with an error message for the session-close command when the backend session ID has no corresponding session instance.
 * To be decided later.
 */
    return BACKEND_PROXY_PROCESS_ERROR;
}


int backend_proxy_sess_msg_response(uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Processes proxy data messages between frontend and backend sessions
 * This function handles the processing of data messages that need to be proxied
 * between a frontend session and a corresponding backend session. It likely
 * involves message routing, validation, or transformation based on the provided
 * session identifiers and message content.
 * @param frontend_sess_id Unique identifier of the frontend session (source/destination of the message)
 * @param backend_sess_id Unique identifier of the backend session (counterpart session for proxying)
 * @param data_len Length of the message data in bytes (specifies valid range of the msg buffer)
 * @param msg Pointer to the message data buffer (uint8_t array) to be processed/proxied
 * @return int Processing result status:
 * BACKEND_PROXY_PROCESS_OK: Message processed and proxied successfully
 * BACKEND_PROXY_PROCESS_ERROR: Failed to process or proxy the message (e.g., invalid session IDs,
 * invalid message format, or forwarding failure)
 * @note - The message buffer (msg) is assumed to contain valid data; its length may be determined by
 * context, protocol specifications, or additional metadata not explicitly passed as a parameter.
 * Callers must ensure frontend_sess_id and backend_sess_id refer to active, valid sessions
 * to avoid processing errors.
 * This function does not take ownership of the msg buffer; the caller is responsible for
 * managing its lifecycle.
 */
int backend_proxy_data_msg_prosess(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t data_len, uint8_t *msg){
 /*
  * STEP 1. Search for the destination backend session instance in the session pool using backend_sess_id. If it fails to find
  *         the appropriate session instance, backend_proxy_data_msg_process shall return BACKEND_PROXY_PROCESS_ERROR;
  *         otherwise, proceed to STEP 2.
  * STEP 2. Construct a struct SessMsgSeg object, bind the data message to this SessMsgSeg object, and then link this SessMsgSeg object
  *         to the msg_f2b queue of the session instance.
  * STEP 3. Link the backend session instance to the queue_f2b of the session pool instance to which the session instance belongs.
  * 
  * The backend proxy protocol will process all sessions in queue_f2b and forward all data messages in each msg_f2b queue after
  * it receives all the data messages in the shared-memory queue. This procedure exists outside backend_proxy_data_msg_process;
  * we just make a note here to help readers maintain a consistent understanding.
  */
    struct BackendEngine_           *eng;
    struct BackendSessionPool       *s_pool;
    struct BackendSessionPoolOps    *ops;
    struct BackendSession           *sess;
    struct SharedMemoryPool         *mem_pool;
    struct SessMsgSeg               *msg_seg;
    int ret;

    utils_print("In %s\n", __func__);

    eng = get_global_backend_engine();

    if(NULL == eng || NULL == eng->sess_pool || NULL == eng->mem_pool || NULL == eng->sess_pool->ops){
        error_print("backend_proxy_data_msg_process failed: eng, eng->sess_pool, eng->mem_pool, or eng->sess_pool->ops is NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    s_pool      = eng->sess_pool;
    ops         = eng->sess_pool->ops;
    mem_pool    = eng->mem_pool;

    if(NULL == ops->search_sess){
        error_print("backend_proxy_data_msg_process failed: ops->search_sess (session searching function) is not initialized!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess = ops->search_sess(s_pool, backend_sess_id);

    if(NULL == sess){
        error_print("backend_proxy_data_msg_process failed: no backend session found for the specified backend_sess_id!");
        return BACKEND_PROXY_PROCESS_ERROR;   
    }

    utils_print("frontend id = %d, backend id = %d\n", sess->frontend_sess_id, sess->backend_sess_id);

    msg_seg = sess_msg_seg_alloc(data_len, SESS_MSG_SEG_SHARED_MEM, msg, mem_pool);

    if(NULL == msg_seg){
        error_print("backend_proxy_data_msg_process failed: insurficient memory resource for allocing message segment!");
        return BACKEND_PROXY_PROCESS_ERROR;   
    }

/*
 * Insert the message segment into the front-to-end message queue.
 */

    utils_print("Before SESS_MSG_SEG_INSERT_QUEUE, TAILQ_EMPTY returns %d\n", TAILQ_EMPTY(&sess->msg_f2b));
    SESS_MSG_SEG_INSERT_QUEUE(sess, msg_seg, f2b);
    utils_print("After SESS_MSG_SEG_INSERT_QUEUE, TAILQ_EMPTY returns %d\n", TAILQ_EMPTY(&sess->msg_f2b));

/*
 * Insert the session into the front-end to back-end active session queue. The backend proxy protocol will process all sessions in the active session queue,
 * detach them one by one, and process all message segments in the selected session.
 */
//    s_pool->queue_f2b;

    utils_print("Before BACKEND_SESS_LINK_TO_QUEUE, state_f2b is %d, TAILQ_EMPTY returns %d\n", sess->state_f2b, TAILQ_EMPTY(&s_pool->queue_f2b));
    BACKEND_SESS_LINK_TO_QUEUE(sess, f2b);
    utils_print("After BACKEND_SESS_LINK_TO_QUEUE, state_f2b is %d, TAILQ_EMPTY returns %d\n", sess->state_f2b, TAILQ_EMPTY(&s_pool->queue_f2b));

    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}


int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}


/*
 *  Functions for building sub-type proxy messages.
 *
 * - build_proxy_dev_message   : Builds a device-specific proxy message
 * - build_proxy_strgy_message : Builds a strategy-specific proxy message
 * - build_proxy_sess_message  : Builds a session-specific proxy message
 * - build_proxy_data_message  : Builds a data-specific proxy message
 */

/**
 * @brief Builds a complete proxy device message by combining the device header and payload.
 * Builds a complete proxy device message by combining the device message header and payload.
 * The function will allocate memory for the output message (caller is responsible for freeing it).
 * @param[in] dev_hdr Pointer to a DevMsgHeader structure specifying the device message header.
 * Must not be NULL.
 * @param[in] payload Pointer to the const uint8_t buffer containing the device message payload.
 * Can be NULL only if payload_len is 0.
 * @param[in] payload_len Length of the payload in bytes. Must be non-negative and match
 * dev_hdr->payload_len (if header contains payload length field) for consistency.
 * @param[out] result_msg Double pointer to receive the address of the constructed proxy device message.
 * On success, points to a newly allocated buffer containing the complete device message.
 * Caller must free this memory with appropriate function when done.
 * Must not be NULL.
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful message construction;
 * Returns BACKEND_PROXY_PROCESS_ERROR if any parameter is invalid (e.g., NULL pointers,
 * mismatched lengths) or memory allocation fails.
 */
 int build_proxy_dev_message(DevMsgHeader *dev_hdr, const uint8_t *payload, size_t payload_len, uint8_t **result_msg){
    DevMsgHeader *header;
    size_t corr_len;
    uint8_t *dev_msg;

    
    corr_len = DEV_MSG_HEADER_PAYLOAD_LEN(dev_hdr);
    utils_print("corr_len = %d, payload_len = %d\n", corr_len, payload_len);

    if(payload_len != corr_len){
        error_print("build_proxy_dev_message failed: payload length does not match expected value based on message type and action type!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(payload_len != dev_hdr->payload_len){
        error_print("build_proxy_dev_message failed: payload length does not match the payload_len field in DevMsgHeader!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    dev_msg                 = *result_msg;
    header                  = (DevMsgHeader *)dev_msg;
    header->version         = dev_hdr->version;
    header->msg_type        = dev_hdr->msg_type;
    header->msg_id          = dev_hdr->msg_id;
    header->action_type     = dev_hdr->action_type;
    header->payload_len     = payload_len;

    dev_msg += sizeof(DevMsgHeader);

    utils_print("In %s, before memcpy\n", __func__);
    memcpy(dev_msg, payload, payload_len);

    return BACKEND_PROXY_PROCESS_OK;
 }


/**
 * @brief Builds a complete proxy strategy message by combining the strategy header and payload.
 * Builds a complete proxy strategy message by combining the strategy message header and payload.
 * The function will allocate memory for the output message (caller is responsible for freeing it).
 * @param[in] strgy_hdr Pointer to a StrgyMsgHeader structure specifying the strategy message header.
 * Must not be NULL.
 * @param[in] payload Pointer to the const uint8_t buffer containing the strategy message payload.
 * Can be NULL only if payload_len is 0.
 * @param[in] payload_len Length of the payload in bytes. Must be non-negative and match
 * strgy_hdr->payload_len (if header contains payload length field) for consistency.
 * @param[out] result_msg Double pointer to receive the address of the constructed proxy strategy message.
 * On success, points to a newly allocated buffer containing the complete strategy message.
 * Caller must free this memory with appropriate function when done.
 * Must not be NULL.
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful message construction;
 * Returns BACKEND_PROXY_PROCESS_ERROR if any parameter is invalid (e.g., NULL pointers,
 * mismatched lengths) or memory allocation fails.
 */
int build_proxy_strgy_message(StrgyMsgHeader *strgy_hdr, const uint8_t *payload, size_t payload_len, uint8_t **result_msg){
    StrgyMsgHeader *header;
    size_t corr_len;
    uint8_t *strgy_msg;

    corr_len = STRGY_MSG_HEADER_PAYLOAD_LEN(strgy_hdr);

    if(payload_len != corr_len){
        error_print("build_proxy_strgy_message failed: payload length does not match expected value based on message type and action type!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(payload_len != strgy_hdr->payload_len){
        error_print("build_proxy_strgy_message failed: payload length does not match the payload_len field in StrgyMsgHeader!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    strgy_msg               = *result_msg;
    header                  = (StrgyMsgHeader *)strgy_msg;
    header->version         = strgy_hdr->version;
    header->msg_type        = strgy_hdr->msg_type;
    header->msg_id          = strgy_hdr->msg_id;
    header->action_type     = strgy_hdr->action_type;
    header->payload_len     = payload_len;

    strgy_msg += sizeof(StrgyMsgHeader);
    memcpy(strgy_msg, payload, payload_len);

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Builds a complete proxy session message by combining the session header and payload.
 * Builds a complete proxy session message by combining the session message header and payload.
 * The function will allocate memory for the output message (caller is responsible for freeing it).
 * @param[in] sess_hdr Pointer to a SessMsgHeader structure specifying the session message header.
 * Must not be NULL.
 * @param[in] payload Pointer to the const uint8_t buffer containing the session message payload.
 * Can be NULL only if payload_len is 0.
 * @param[in] payload_len Length of the payload in bytes. Must be non-negative and match
 * sess_hdr->payload_len (if header contains payload length field) for consistency.
 * @param[out] result_msg Double pointer to receive the address of the constructed proxy session message.
 * On success, points to a newly allocated buffer containing the complete session message.
 * Caller must free this memory with appropriate function when done.
 * Must not be NULL.
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful message construction;
 * Returns BACKEND_PROXY_PROCESS_ERROR if any parameter is invalid (e.g., NULL pointers,
 * mismatched lengths) or memory allocation fails.
 */
int build_proxy_sess_message(SessMsgHeader *sess_hdr, const uint8_t *payload, size_t payload_len, uint8_t **result_msg){
    SessMsgHeader *header;
    size_t corr_len;
    uint8_t *sess_msg;

    corr_len = SESS_MSG_HEADER_PAYLOAD_LEN(sess_hdr);

    if(payload_len != corr_len){
        error_print("build_proxy_sess_message failed: payload length does not match expected value based on message type, action type and IP version!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(payload_len != sess_hdr->payload_len){
        error_print("build_proxy_sess_message failed: payload length does not match the payload_len field in SessMsgHeader!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess_msg = *result_msg;
    header = (SessMsgHeader *)sess_msg;
    header->version = sess_hdr->version;
    header->msg_type = sess_hdr->msg_type;
    header->action_type = sess_hdr->action_type;
    header->ip_version = sess_hdr->ip_version;
    header->payload_len = sess_hdr->payload_len;

    utils_print("In %s, version = %d, msg_type = %d, action_type = %d, ip_version = %d, payload_len = %d, address = %p\n", 
                __func__, header->version, header->msg_type, header->action_type, header->ip_version, header->payload_len, &header);
    
    sess_msg += sizeof(SessMsgHeader);
    memcpy(sess_msg, payload, payload_len);

    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Builds a complete proxy data message by combining the proxy message header and payload.
 * Builds a complete proxy data message by combining the proxy message header and payload.
 * The function will allocate memory for the output message (caller is responsible for freeing it).
 * @param[in] proxy_msg_hdr Pointer to a ProxyMsgHeader structure specifying the proxy data message header.
 * Must not be NULL.
 * @param[in] payload Pointer to the const uint8_t buffer containing the data message payload.
 * Can be NULL only if payload_len is 0.
 * @param[in] payload_len Length of the payload in bytes. Must be non-negative and match
 * proxy_msg_hdr->payload_len (if header contains payload length field) for consistency.
 * @param[out] result_msg Double pointer to receive the address of the constructed proxy data message.
 * On success, points to a newly allocated buffer containing the complete data message.
 * Caller must free this memory with appropriate function (e.g., free()) when done.
 * Must not be NULL.
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful message construction;
 * Returns BACKEND_PROXY_PROCESS_ERROR if any parameter is invalid (e.g., NULL pointers,
 * mismatched lengths) or memory allocation fails.
*/
int build_proxy_data_message(ProxyMsgHeader *proxy_msg_hdr, const uint8_t *payload, size_t payload_len, uint8_t **result_msg){
    uint8_t *data_msg;
    utils_print("In %s, payload_len = %d, proxy_msg_hdr->payload_len = %d\n", __func__, payload_len, proxy_msg_hdr->payload_len);
    utils_print("frontend_sess_id = %d, frontend_sess_id =%d\n", proxy_msg_hdr->frontend_sess_id, proxy_msg_hdr->backend_sess_id);

#if 0
    if(payload_len != proxy_msg_hdr->payload_len){
        error_print("build_proxy_data_message failed: payload length does not match expected value based on message type, action type and IP version!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
#endif

    utils_print("Address of proxy data header = %p, content  =%p, size of ProxyMsgHeader = %d\n", proxy_msg_hdr, *result_msg, sizeof(ProxyMsgHeader));

    data_msg = *result_msg;
    memcpy(data_msg, payload, payload_len);

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Builds a complete message by combining the general header and payload.
 * 
 * Builds a complete proxy general message by combining the general header and payload.
 * The function will allocate memory for the output message (caller is responsible for freeing it).
 * 
 * Constructs a complete proxy general message by integrating the provided header and payload.
 * The memory for the output message is managed based on the specified allocation mode:
 * For MEMORY_ALLOC_SHARED: Memory is allocated within shared memory, which is organized in a FIFO RING buffer.
 * The caller does not need to handle memory deallocation, as the shared memory is managed by the FIFO RING buffer mechanism.
 * For MEMORY_ALLOC_CALLER: Memory must be pre-allocated by the caller. The function will directly populate the
 * provided buffer without checking its size; the caller is solely responsible for ensuring the buffer is large
 * enough to hold the complete message (header + payload).
 * 
 * 
 * @param[in]  engine            Pointer to a BackendEngine object containing backend proxy's global context,
 *                               such as runtime configuration, memory allocator handles, or system resources.
 *                               Used for accessing backend-specific settings or memory management during message construction.
 *                               Must not be NULL.
 * @param[in]  header            Pointer to a GeneralProxyMsgHeader structure specifying the message header.
 *                               Must not be NULL.
 * @param[in]  payload           Pointer to the const uint8_t buffer containing the message payload.
 *                               Can be NULL only if payload_len is 0.
 * @param[in]  payload_len       Length of the payload in bytes. Must be non-negative and match
 *                               header->payload_len (if header contains payload length field) for consistency.

 * @param[out] result_msg        Double pointer to receive the address of the constructed proxy message.
                                 For MEMORY_ALLOC_SHARED: On success, points to the message location within the shared FIFO RING buffer. 
                                 No caller action is needed for deallocation.
                                 For MEMORY_ALLOC_CALLER: Must point to a pre-allocated buffer. On success, the buffer is populated with 
                                 the complete message. Caller must ensure sufficient size.Must not be NULL.
 * @param[in] ring_buf           Pointer to a struct SharedMemoryPoolQueue. Required and must not be NULL when alloc_mode is MEMORY_ALLOC_SHARED 
                                 (used for FIFO RING buffer operations).
                                 Ignored when alloc_mode is MEMORY_ALLOC_CALLER (can be NULL).
 * @return int                   Returns BACKEND_PROXY_PROCESS_OK on successful message construction;
 *                               Returns BACKEND_PROXY_PROCESS_ERROR if any parameter is invalid (e.g., NULL pointers,
 *                               mismatched lengths) or memory allocation fails (for MEMORY_ALLOC_SHARED)
 */
int build_proxy_general_message(BackendEngine *engine, GeneralProxyMsgHeader *header, 
                                const uint8_t *payload, size_t payload_len, uint8_t **result_msg, 
                                MemoryAllocMode alloc_mode, struct SharedMemoryPoolQueue *ring_buf){
    uint8_t         *msg_buf;
    uint64_t        mem_addr;
    uint16_t        proxy_msg_payload_len;
    ProxyMsgType    outer_msg_type;
    ProxyMsgHeader  *proxy_msg_hdr;
    DevMsgHeader    *dev_hdr;
    StrgyMsgHeader  *strgy_hdr;
    SessMsgHeader   *sess_hdr;
    int             ret, alloc_size;

/*
 * Check the validity of the input parameters.
 */
    if(NULL == header || NULL == payload || NULL == result_msg){
        error_print("build_proxy_general_message failed: input(s) for generating proxy message is/are NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


    if(NULL == engine || NULL == engine->mem_pool){
        error_print("build_proxy_general_message failed: backend engine is NULL or its memory pool is uninitialized!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
/*
 * Allocate shared-memory for storing the proxy message.
 */
    if(MEMORY_ALLOC_SHARED == alloc_mode){
        if(NULL == ring_buf){
            error_print("build_proxy_general_message failed: MEMORY_ALLOC_SHARED mode requires a non-NULL ring buffer (FIFO queue)!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }

        utils_print("before SHM_POOL_QUEUE_ALLOC_FROM_HEADER, header = %d, tail = %d, virt addr = %lld\n", ring_buf->header, ring_buf->tail, ring_buf->virt_addr1);
        SHM_POOL_QUEUE_ALLOC_FROM_HEADER(ring_buf, &mem_addr);
        utils_print("after SHM_POOL_QUEUE_ALLOC_FROM_HEADER, header = %d, tail = %d, memaddr = %lld\n", ring_buf->header, ring_buf->tail, mem_addr);

        if(ERROR_SHARED_MEM_ADDR == mem_addr){
            error_print("build_proxy_general_message failed: shared memory FIFO queue is full, cannot allocate new block!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }

        msg_buf         = (uint8_t *)mem_addr;
        *result_msg     = msg_buf;

//        SHM_POOL_QUEUE_LOOKUP_VIRTADDR(ring_buf, 1, 1, &mem_addr);
//        SHM_POOL_QUEUE_ALLOC_FROM_HEADER(ring_buf, &mem_addr);
    }else if(MEMORY_ALLOC_CALLER == alloc_mode){
        /*
 * Frontend protocol should allocate memory dynamically.
 */
        alloc_size = sizeof(ProxyMsgHeader) + header->outer_header.payload_len;
        msg_buf         = malloc(alloc_size);

        if(NULL == msg_buf){
            error_print("build_proxy_general_message failed: insufficient memory for allocation!\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
    }else if(MEMORY_ALLOC_AMPQUEUE  == alloc_mode){
        if(NULL == engine->hyper_tx_queue){
            error_print("build_proxy_general_message failed: HyperAMP TX queue is not initialized!\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        memset(global_amp_tx_buf, 0, sizeof(global_amp_tx_buf));
        msg_buf         = global_amp_tx_buf;
        *result_msg     = msg_buf;
    }else{
        error_print("build_proxy_general_message failed: unsupported allocte mode!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Fill the proxy message header.
 */

    proxy_msg_hdr                       = (ProxyMsgHeader *)msg_buf;
    proxy_msg_hdr->version              = header->outer_header.version;
    proxy_msg_hdr->proxy_msg_type       = outer_msg_type = header->outer_header.proxy_msg_type;
    proxy_msg_hdr->frontend_sess_id     = header->outer_header.frontend_sess_id;
    proxy_msg_hdr->backend_sess_id      = header->outer_header.backend_sess_id;
    
    utils_print("In %s, version = %d, proxy_msg_type = %d, frontend_sess_id = %d, backend_sess_id = %d, payload_len = %d\n", __func__,
                proxy_msg_hdr->version, proxy_msg_hdr->proxy_msg_type, proxy_msg_hdr->frontend_sess_id, proxy_msg_hdr->backend_sess_id, proxy_msg_hdr->payload_len);
    msg_buf += sizeof(ProxyMsgHeader);
    switch(outer_msg_type) {
        case PROXY_MSG_TYPE_DEV:
            dev_hdr               = &header->inner_header.dev_hdr;
            proxy_msg_payload_len = sizeof(DevMsgHeader);
            utils_print("In %s, before enter build_proxy_dev_message\n", __func__);
            ret = build_proxy_dev_message(dev_hdr, payload, payload_len, &msg_buf);
            break;
        case PROXY_MSG_TYPE_STRGY:
            strgy_hdr             = &header->inner_header.strgy_hdr;
            proxy_msg_payload_len = sizeof(StrgyMsgHeader);
            ret = build_proxy_strgy_message(strgy_hdr, payload, payload_len, &msg_buf);
            break;
        case PROXY_MSG_TYPE_SESS:
            sess_hdr              = &header->inner_header.sess_hdr;
            proxy_msg_payload_len = sizeof(SessMsgHeader);
            utils_print("In %s, ip version = %d\n",  __func__, sess_hdr->ip_version);
            ret = build_proxy_sess_message(sess_hdr, payload, payload_len, &msg_buf);
            break;
        case PROXY_MSG_TYPE_DATA:
            proxy_msg_payload_len = 0;
            ret = build_proxy_data_message(proxy_msg_hdr, payload, payload_len, &msg_buf);
            utils_print("In %s, after build_proxy_data_message, the return value is %d\n", __func__, ret);
            break;
        default:
/*
 * Message type is not supported!.
 */
            error_print("build_proxy_general_message failed: message type is not supported!");
            free_shared_mem(engine->mem_pool, mem_addr);
            return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("build_proxy_general_message failed: failed to build proxy message!");
//        free_shared_mem(engine->mem_pool, mem_addr);
        SHM_POOL_QUEUE_HEAD_ROLLBACK(ring_buf);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Compute the payload length and fill it into the corresponding field of the proxy message header.
 */
    proxy_msg_hdr->payload_len = payload_len + proxy_msg_payload_len;

/*
 * In MEMORY_ALLOC_SHARED mode, the build_proxy_general_message function is responsible for
 * enqueuing the constructed message into the shared memory FIFO queue (ring_buf)
 */
#if 0
    if(MEMORY_ALLOC_SHARED == alloc_mode){
        utils_print("before SHMP_QUEUE_ENQUEUE, header = %d, tail = %d\n", ring_buf->header, ring_buf->tail);
        SHMP_QUEUE_ENQUEUE(ring_buf, ret);
        utils_print("after SHMP_QUEUE_ENQUEUE, header = %d, tail = %d\n", ring_buf->header, ring_buf->tail);

        SHM_POOL_QUEUE_HEAD_ROLLBACK(ring_buf);
        utils_print("after SHM_POOL_QUEUE_HEAD_ROLLBACK, header = %d, tail = %d\n", ring_buf->header, ring_buf->tail);
        return ret;
    }
#endif


#if 0
    int debug_cnt;
    utils_print("shared queue capacity = %d, header = %d, tail = %d, block_size = %d\n", ring_buf->capacity, ring_buf->header, ring_buf->tail, ring_buf->block_size);

    debug_cnt = ring_buf->header;

    utils_print("Debug shared memory I/O\n");
    utils_print("virt addr = %lld\n", ring_buf->virt_addr1);
    while(debug_cnt < ring_buf->capacity + 10){
        uint64_t  debug_mem_addr;
        SHM_POOL_QUEUE_ALLOC_FROM_HEADER(ring_buf, &debug_mem_addr);
        utils_print("shared queue header = %d, tail = %d, addr = %lld, diff = %d\n", ring_buf->header, ring_buf->tail, debug_mem_addr, debug_mem_addr - ring_buf->virt_addr1);
        debug_cnt++;
    }
#endif

/*
 * In MEMORY_ALLOC_AMPQUEUE mode, the created message should be pushed into the HyperAMP shared queue.
 */
    if(MEMORY_ALLOC_AMPQUEUE == alloc_mode){
        msg_buf -= sizeof(ProxyMsgHeader);
        ret = hyperamp_queue_enqueue(engine->hyper_tx_queue, HYPERAMP_ZONE_ID_Linux, msg_buf, payload_len + proxy_msg_payload_len + sizeof(ProxyMsgHeader), engine->hyper_amp_data_region);

        if(HYPERAMP_OK == ret){
            return BACKEND_PROXY_PROCESS_OK;
        }else if(HYPERAMP_AGAIN == ret){
            return BACKEND_PROXY_PROCESS_AGAIN;
        }else{
            error_print("build_proxy_general_message failed: faild to push message into the HyperAmp queue!\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
    }
    return BACKEND_PROXY_PROCESS_OK;
}



/**
 * @brief Receive data message via shared memory for backend proxy (zero-copy).
 * 
 * @param sess Pointer to BackendSession instance.
 * @param msg Double pointer to store the address of received data in shared memory.
 *            The function sets *msg to point directly to the data location in shared memory
 *            (no data copy occurs). Caller should not free this pointer; memory management
 *            is handled by the shared memory pool.
 * @return BACKEND_PROXY_PROCESS_OK on success; BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int backend_proxy_shmem_data_msg_recv(struct BackendSession *sess, uint8_t **msg){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Send data message via shared memory for backend proxy (zero-copy).
 * 
 * @param sess Pointer to BackendSession instance.
 * @param msg Double pointer to the data in shared memory to be sent. *msg must point to
 *            a location within the shared memory pool (no data copy occurs; the deque
 *            stores the pointer directly). The const qualifier ensures the data is not
 *            modified during transmission.
 * @return BACKEND_PROXY_PROCESS_OK on success; BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int backend_proxy_shmem_data_msg_send(struct BackendSession *sess, const uint8_t **msg){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Receive data message via socket for backend proxy.
 * @param sess Pointer to BackendSession instance.
 * @param msg Buffer to store received data message.
 * @return BACKEND_PROXY_PROCESS_OK on success; BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int backend_proxy_sock_data_msg_recv(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * Send data message via socket for backend proxy.
 * @param sess Pointer to BackendSession instance.
 * @param msg Buffer containing data message to send.
 * @return BACKEND_PROXY_PROCESS_OK on success; BACKEND_PROXY_PROCESS_ERROR on failure.
 */
 int backend_proxy_sock_data_msg_send(struct BackendSession *sess, uint8_t *msg){
    return BACKEND_PROXY_PROCESS_OK;
 }


/**
 * @brief Generate a "device enable response" message for backend proxy (zero-copy).
 * 
 * This function creates a response message indicating the result of a device enable request.
 * It leverages zero-copy by directly allocating/accessing memory in the shared memory pool (via the backend engine),
 * avoiding redundant data duplication.
 * 
 * @param eng Pointer to the associated BackendEngine_ instance. Provides context (e.g., device state, shared memory resources)
 *            required to generate the response. Must not be NULL.
 * @param msg_id 16-bit unique message ID. Used to match this response to the corresponding device enable request,
 *               ensuring correct request-response association in asynchronous communication.
 * @param msg Double pointer to store the address of the generated response message in shared memory.
 *            The function sets *msg to point directly to the message’s location in shared memory (no data copy occurs).
 *            Caller must NOT pre-allocate a buffer or manually free this pointer—memory is managed by the backend engine’s shared memory pool.
 * 
 * @return BACKEND_PROXY_PROCESS_OK on successful message generation;
 *         BACKEND_PROXY_PROCESS_ERROR on failure (e.g., invalid eng pointer, shared memory allocation failed, illegal msg_id).
 */
int backend_proxy_generate_dev_msg_enable_response(struct BackendEngine_ *eng, uint16_t msg_id, uint8_t **msg){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Generate a "device disable response" message for backend proxy (zero-copy).
 * 
 * This function creates a response message indicating the result of a device disable request.
 * It leverages zero-copy by directly allocating/accessing memory in the shared memory pool (via the backend engine),
 * avoiding redundant data duplication.
 * 
 * @param eng Pointer to the associated BackendEngine_ instance. Provides context (e.g., device state, shared memory resources)
 *            required to generate the response. Must not be NULL.
 * @param msg_id 16-bit unique message ID. Used to match this response to the corresponding device disable request,
 *               ensuring correct request-response association in asynchronous communication.
 * @param msg Double pointer to store the address of the generated response message in shared memory.
 *            The function sets *msg to point directly to the message’s location in shared memory (no data copy occurs).
 *            Caller must NOT pre-allocate a buffer or manually free this pointer—memory is managed by the backend engine’s shared memory pool.
 * 
 * @return BACKEND_PROXY_PROCESS_OK on successful message generation;
 *         BACKEND_PROXY_PROCESS_ERROR on failure (e.g., invalid eng pointer, shared memory allocation failed, illegal msg_id).
 */
int backend_proxy_generate_dev_msg_disable_response(struct BackendEngine_ *eng, uint16_t msg_id, uint8_t **msg){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Generate a "device query response" message for backend proxy (zero-copy).
 * 
 * This function creates a response message containing the result of a device state query request (e.g., device online status, resource usage).
 * It leverages zero-copy by directly allocating/accessing memory in the shared memory pool (via the backend engine),
 * avoiding redundant data duplication.
 * 
 * @param eng Pointer to the associated BackendEngine_ instance. Provides context (e.g., current device state, shared memory resources)
 *            required to generate the response (queries device state from the engine). Must not be NULL.
 * @param msg_id 16-bit unique message ID. Used to match this response to the corresponding device query request,
 *               ensuring correct request-response association in asynchronous communication.
 * @param msg Double pointer to store the address of the generated response message in shared memory.
 *            The function sets *msg to point directly to the message’s location in shared memory (no data copy occurs).
 *            Caller must NOT pre-allocate a buffer or manually free this pointer—memory is managed by the backend engine’s shared memory pool.
 * 
 * @return BACKEND_PROXY_PROCESS_OK on successful message generation;
 *         BACKEND_PROXY_PROCESS_ERROR on failure (e.g., invalid eng pointer, shared memory allocation failed, illegal msg_id, device state query failed).
 */
int backend_proxy_generate_dev_msg_query_response(struct BackendEngine_ *eng, uint16_t msg_id, uint8_t **msg){
    return BACKEND_PROXY_PROCESS_OK;
}



/**
 * @brief Generates response messages for session creation or closure operations
 * This function generates corresponding response messages for session creation or closure operations
 * based on the session object, message type, and operation results, and returns the message data using zero-copy.
 * @param sess Pointer to the BackendSession object, containing session-related context information
 * @param sess_msg_type Message type, specifying whether to generate a "create" or "response" message
 * @param op_resp Pointer to SessOpRespData structure, which includes a status code and a reason description.
 * It indicates whether the create or close operation was successful and the reason for failure if applicable.
 * @param msg Double pointer to the message. The generated message is returned via zero-copy (no data copying).
 * The caller should handle memory management appropriately.
 * @return int Execution result: BACKEND_PROXY_PROCESS_OK on success, or BACKEND_PROXY_PROCESS_ERROR on failure
 */
int backend_proxy_generate_sess_msg_create_close_response(struct BackendSession *sess, int sess_msg_type, SessOpRespData *op_resp, uint8_t **msg){
    utils_print("In %s\n", __func__);
    struct BackendEngine_   *eng;
    GeneralProxyMsgHeader   header;
    SessMsgHeader           *sess_hdr;
    uint8_t                 *payload_data;
    int                     ret;

    if(NULL == op_resp || NULL == sess || NULL == sess->eng){
        error_print("backend_proxy_generate_sess_msg_create_response failed: input(s) for generating the session create/close-response message is/are NULL, \
                     or the session is not initialized correctly!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(!IS_VALID_SESS_MSG_TYPE(sess_msg_type)){
        error_print("backend_proxy_generate_sess_msg_create_close_response failed: invalid sess_msg_type. Valid types are SESS_MSG_CREATE or SESS_MSG_CLOSE response types!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    eng = sess->eng;
/*
 * Fills information for generating session create/close-response message.
 * It is not necessary to fill the payload_len field.
 */
    header.outer_header.version             = PROXY_PROTO_VERSION_1;
    header.outer_header.proxy_msg_type      = PROXY_MSG_TYPE_SESS;
    header.outer_header.frontend_sess_id    = sess->frontend_sess_id;
    header.outer_header.backend_sess_id     = sess->backend_sess_id;

//    header.outer_header.payload_len;
    sess_hdr                                = &header.inner_header.sess_hdr;
    sess_hdr->version                       = PROXY_PROTO_SESS_VERSION_1;
    sess_hdr->msg_type                      = (sess_msg_type == SESS_MSG_CREATE) ? SESS_MSG_CREATE : SESS_MSG_CLOSE;
    sess_hdr->action_type                   = ACTION_TYPE_RESPONSE;
    sess_hdr->ip_version                    = sess->ip_version;
    sess_hdr->payload_len                   = sizeof(SessOpRespData);

    payload_data                            = (uint8_t *)op_resp;

//    ret = build_proxy_general_message(eng, &header, payload_data, sizeof(SessOpRespData), msg, MEMORY_ALLOC_SHARED, eng->tx_queue);
    utils_print("version = %d, proxy_msg_type = %d, frontend_sess_id = %d, backend_sess_id = %d\n", 
                header.outer_header.version, header.outer_header.proxy_msg_type, header.outer_header.frontend_sess_id, header.outer_header.backend_sess_id);
    utils_print("sess msg version = %d, sess msg type = %d, sess action type = %d, sess ip version = %d\n", 
                sess_hdr->version, sess_hdr->msg_type, sess_hdr->action_type, sess_hdr->ip_version);
    ret = build_proxy_general_message(eng, &header, payload_data, sizeof(SessOpRespData), msg, MEMORY_ALLOC_AMPQUEUE, NULL);

//    ret = build_proxy_sess_message(&header, payload_data, sizeof(SessOpRespData), msg);
    return ret;
}


 /**
 * @brief Generates standalone response messages for session creation/closure when session allocation fails
 * @details This function generates response messages for session creation or closure operations 
 *          in scenarios where session allocation has failed (i.e., no valid BackendSession object exists).
 *          It requires explicit core context information (engine, session IDs, etc.) and returns the 
 *          generated message via zero-copy mechanism (no extra data copying). The caller is responsible 
 *          for proper memory management of the returned message.
 * @param[in] eng Pointer to a BackendEngine_ object containing engine-level context information 
 *                (e.g., shared memory handles, runtime configuration). Must not be NULL.
 * @param[in] frontend_sess_id 16-bit identifier for the frontend session, used to map the response 
 *                             to the corresponding frontend request.
 * @param[in] backend_sess_id 16-bit identifier of the backend session, used to fill the corresponding field
 *                            in the message and associate it with the backend session.
 * @param[in] ip_version IP protocol version (e.g., IPv4 or IPv6) associated with the session operation.
 * @param[in] sess_msg_type Type of the session message, specifying whether to generate a response for 
 *                          a "create" or "close" operation (must be a valid session message type).
 * @param[in] op_resp Pointer to a SessOpRespData structure containing the operation's status code 
 *                    and failure reason (if applicable). Used to populate response details. Must not be NULL.
 * @param[out] msg Double pointer to the generated message. The message is returned via zero-copy 
 *                 (address of message data is passed directly without content copying). Caller must 
 *                 handle memory release as per system conventions.
 * @return int Execution result: BACKEND_PROXY_PROCESS_OK on successful message generation, or BACKEND_PROXY_PROCESS_ERROR on failure .
 */
int backend_proxy_generate_sess_msg_create_close_response_standalone(struct BackendEngine_ *eng, uint16_t frontend_sess_id, uint16_t backend_sess_id,
                                                                     int ip_version, int sess_msg_type, SessOpRespData *op_resp, uint8_t **msg){
    GeneralProxyMsgHeader   header;
    SessMsgHeader           *sess_hdr;
    uint8_t                 *payload_data;
    int                     ret;

    if(NULL == eng || NULL == op_resp){
        error_print("backend_proxy_generate_sess_msg_create_close_response_standalone failed: input(s) for generating the session create/close-response message is/are NULL, \
                     or the session is not initialized correctly!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(!IS_VALID_SESS_MSG_TYPE(sess_msg_type)){
        error_print("backend_proxy_generate_sess_msg_create_close_response failed: invalid sess_msg_type. Valid types are SESS_MSG_CREATE or SESS_MSG_CLOSE response types!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Fills information for generating session create/close-response message.
 * Because the session instance has not been successfully created, the backend_sess_id cannot be registered. 
 * The backend proxy protocol will fill the backend session ID field with BACKEND_HANDOVER_SESSION_ID.
 *
 * It is not necessary to fill the payload_len field.
 */
    header.outer_header.version             = PROXY_PROTO_VERSION_1;
    header.outer_header.proxy_msg_type      = PROXY_MSG_TYPE_SESS;
    header.outer_header.frontend_sess_id    = frontend_sess_id;
    header.outer_header.backend_sess_id     = backend_sess_id;

//    header.outer_header.payload_len;
    sess_hdr                                = &header.inner_header.sess_hdr;
    sess_hdr->version                       = PROXY_PROTO_SESS_VERSION_1;
    sess_hdr->msg_type                      = (sess_msg_type == SESS_MSG_CREATE) ? SESS_MSG_CREATE : SESS_MSG_CLOSE;
    sess_hdr->action_type                   = ACTION_TYPE_RESPONSE;
    sess_hdr->ip_version                    = ip_version;
    sess_hdr->payload_len                   = sizeof(SessOpRespData);

    payload_data                            = (uint8_t *)op_resp;

//    ret = build_proxy_general_message(eng, &header, payload_data, sizeof(SessOpRespData), msg, MEMORY_ALLOC_SHARED, eng->tx_queue);
    ret = build_proxy_general_message(eng, &header, payload_data, sizeof(SessOpRespData), msg, MEMORY_ALLOC_AMPQUEUE, NULL);

    return ret;
}


/**
 * @brief Generates a standalone session message and sends it to the frontend proxy via shared memory
 * @details This function first calls backend_proxy_generate_sess_msg_create_close_response_standalone()
 *          to generate a standalone session response message (for scenarios where session allocation failed),
 *          then sends the generated message to the frontend proxy through shared memory. It handles the entire
 *          process from message construction to inter-proxy communication via shared memory.
 * @param[in] eng Pointer to a BackendEngine_ object containing engine-level context information
 *                (e.g., shared memory handles, runtime configuration). Must not be NULL.
 * @param[in] frontend_sess_id 16-bit identifier of the frontend session, used to map the message to the
 *                             corresponding frontend request.
 * @param[in] backend_sess_id 16-bit identifier of the backend session, used to fill the corresponding field
 *                            in the message and associate it with the backend session.
 * @param[in] ip_version IP protocol version (e.g., IPv4 or IPv6) associated with the session operation.
 * @param[in] sess_msg_type Type of the session message, specifying whether it's a "create" or "close" response.
 *                          Must be a valid session message type (e.g., SESS_MSG_CREATE).
 * @param[in] op_resp Pointer to a SessOpRespData structure containing the operation's status code
 *                    and failure reason (if applicable). Used to populate the message content. Must not be NULL.
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful message generation and transmission;
 *         Returns BACKEND_PROXY_PROCESS_ERROR if message generation fails, shared memory
 *         access fails, or any input parameter is invalid.
 */
int backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem(struct BackendEngine_ *eng, uint16_t frontend_sess_id, uint16_t backend_sess_id,
                                                                 int ip_version, int sess_msg_type, SessOpRespData *op_resp){
    utils_print("In %s, frontend_sess_id = %d, backend_sess_id = %d\n", __func__, frontend_sess_id, backend_sess_id);

    int             ret;
    uint8_t         *msg;
    ProxyMsgHeader  *msg_header;
    struct SharedMemoryPoolQueue *tx_queue;


    if (NULL == eng || NULL == eng->tx_queue || NULL == eng->mem_pool || NULL == op_resp) {
        error_print("backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem failed: invalid parameters - eng, eng->tx_queue, eng->mem_pool, or op_resp are NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    ret = backend_proxy_generate_sess_msg_create_close_response_standalone(eng, frontend_sess_id, BACKEND_HANDOVER_SESSION_ID, ip_version, sess_msg_type, op_resp, &msg);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem failed: the session creation/close-response message cannot be constructed successfully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

#if 0
    tx_queue    = eng->tx_queue;
    msg_header  = (ProxyMsgHeader *)msg;
    
    ret = shared_mem_pool_queue_send_oc(tx_queue, msg, PROXY_MSG_TOTAL_SIZE(msg_header));

    if(BACKEND_PROXY_PROCESS_OK != ret){
        free_shared_mem(eng->mem_pool, (uint64_t)msg);
        error_print("backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem failed: failed to push the message into the tx queue!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
#endif
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Generates a session message and sends it to the frontend proxy via shared memory
 * @details This function first calls a message generation function (e.g., build_proxy_general_message())
 * to generate a standard session message (for scenarios involving normal session operations),
 * then sends the generated message to the frontend proxy through shared memory. It handles the entire
 * process from message construction to inter-proxy communication via shared memory, utilizing
 * session context from the provided BackendSession object.
 *
 * @param[in] sess Pointer to a BackendSession structure containing session-specific context
 * (e.g., session IDs, IP version, and so on). Used to populate session-related
 * message fields. Must not be NULL.
 * @param[in] sess_msg_type Type of the session message, specifying whether it's a "create" or "close" response.
 *  Must be a valid session message type (e.g., SESS_MSG_CREATE).
 * @param[in] op_resp Pointer to a SessOpRespData structure containing the operation's status code
 * and relevant data (if applicable). Used to populate the message content. Must not be NULL.
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful message generation and transmission;
 * Returns BACKEND_PROXY_PROCESS_ERROR if message generation fails, shared memory
 * access fails, or any input parameter is invalid.
 */
int backend_proxy_send_sess_msg_to_frontend_via_shmem(struct BackendSession *sess, int sess_msg_type, SessOpRespData *op_resp){
    int                             ret;
    uint8_t                         *msg;
    ProxyMsgHeader                  *msg_header;
    BackendEngine                   *eng;

    struct SharedMemoryPoolQueue    *tx_queue;

    if (NULL == sess || NULL == sess->eng || NULL == sess->eng->tx_queue || NULL == sess->eng->mem_pool || NULL == op_resp) {
        error_print("backend_proxy_send_sess_msg_to_frontend_via_shmem failed: invalid parameters - sess, sess->eng, \
                     sess->eng->tx_queue, sess->eng->mem_pool, or op_resp are NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    ret = backend_proxy_generate_sess_msg_create_close_response(sess, sess_msg_type, op_resp, &msg);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("backend_proxy_send_sess_msg_to_frontend_via_shmem failed: the session creation/close-response message cannot be constructed successfully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
#if 0
    eng = sess->eng;
    tx_queue = sess->eng->tx_queue;

    ret = shared_mem_pool_queue_send_oc(tx_queue, msg, PROXY_MSG_TOTAL_SIZE(msg_header));

    if(BACKEND_PROXY_PROCESS_OK != ret){
        free_shared_mem(eng->mem_pool, (uint64_t)msg);
        error_print("backend_proxy_send_sess_msg_to_frontend_via_shmem failed: failed to push the message into the tx queue!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
#endif
    return BACKEND_PROXY_PROCESS_OK;
}