#include "backend_proto.h"
#include "engine.h"
#include "message.h"

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



 /**
 * @brief Processes session messages in the backend proxy
 * @details This function serves as the core handler for session-related communication between front and backend proxies.
 *          It handles general session message processing by parsing the incoming message, coordinating with the specified
 *          frontend and backend sessions, and executing appropriate operations based on message content. 
 *          The processing follows a hierarchical call structure:
 *          backend_proxy_sess_msg_process
 *              |-> backend_proxy_sess_msg_process_ver1
 *                  |-> backend_proxy_sess_msg_process_create_ver1
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

/**
 * @brief Processes version 1 of session creation messages in the backend proxy
 * @details This function handles the processing logic for version 1 session creation messages.
 *          It typically parses the message payload, performs necessary validation, and executes
 *          session creation operations based on the provided parameters. It is responsible for
 *          coordinating frontend-backend session mapping for version 1 message format.
 * @param[in] frontend_sess_id 16-bit identifier of the frontend session, used to associate with the corresponding frontend request
 * @param[in] backend_sess_id 16-bit identifier of the backend session, used for backend-side session tracking and management
 * @param[in] ip_version 16-bit value indicating the IP protocol version (e.g., IPv4 or IPv6) used in the session
 * @param[in] payload_len 16-bit length of the message payload (in bytes), specifying the size of the data in msg_payload
 * @param[in] msg_payload Pointer to the message payload data, containing the detailed content of the version 1 session creation request
 * @return int Execution result: Typically returns BACKEND_PROXY_PROCESS_OK on successful processing,
 *         or aBACKEND_PROXY_PROCESS_ERROR if validation fails, payload is invalid, or session creation encounters issues.
 */
int backend_proxy_sess_msg_process_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
  
    return __backend_proxy_sess_msg_process_create_ver1(frontend_sess_id, backend_sess_id, ip_version, payload_len, msg_payload);
}


int __backend_proxy_sess_msg_process_create_ver1(uint16_t frontend_sess_id, uint16_t backend_sess_id, uint16_t ip_version, uint16_t payload_len, uint8_t *msg_payload){
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
 * In __backend_proxy_sess_msg_process_create_ver1, this function parses the session parameters and calls the function pointed to by the create_sess pointer to establish a new session.
 */
    pool = get_backend_high_speed_pool();

    if(NULL == pool){
        error_print("__backend_proxy_sess_msg_process_create_ver1 returns an error because the high speed pool is not initialized!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess_para.frontend_sess_id  = frontend_sess_id;

    if(SESS_IPV4_PROTO == ip_version){
        if(payload_len != sizeof(SessParaIPv4)){
            error_print("__backend_proxy_sess_msg_process_create_ver1 returns an error because the payload length does not match the IPv4 handover message!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }

        para_ipv4                   = (SessParaIPv4 *)msg_payload;
        sess_para.dev_id            = para_ipv4->dev_id;
        sess_para.trans_proto       = para_ipv4->trans_proto;
        sess_para.ip_version        = SESS_IPV4_PROTO;

        ipv4_port_tuple             = &sess_para.ip_port_tuple.ipv4_port_tuple;
        ipv4_addr                   = &para_ipv4->ipv4_addr;
        memcpy(&ipv4_port_tuple->ipv4_addr, &ipv4_addr, sizeof(struct IPv4Address));
        ipv4_port_tuple->port       = para_ipv4->port;
        

    }else if(SESS_IPV6_PROTO == ip_version){
        if(payload_len != sizeof(SessParaIPv6)){
            error_print("__backend_proxy_sess_msg_process_create_ver1 failed: the high-speed pool is not initialized!");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        para_ipv6                   = (SessParaIPv6 *)msg_payload;
        sess_para.dev_id            = para_ipv6->dev_id;
        sess_para.trans_proto       = para_ipv6->trans_proto;
        sess_para.ip_version        = SESS_IPV6_PROTO;

        ipv6_port_tuple             = &sess_para.ip_port_tuple.ipv6_port_tuple;
        ipv6_addr                   = &para_ipv6->ipv6_addr;
        memcpy(&ipv6_port_tuple->ipv6_addr, &ipv6_addr, sizeof(struct IPv6Address));
        ipv6_port_tuple->port       = para_ipv6->port;

    }else{
        ip_ver_valid = false;
    }
    
    if(true == ip_ver_valid){
/*
 * If the session creation function does not exist, print an error message (indicating create_sess does not point to a valid create-session function) .
 * The create_sess pointer points to the function that actually creates a socket, a session object and binds them together.
 */

        if(!pool->ops->create_sess){
            error_print("__backend_proxy_sess_msg_process_create_ver1 failed: the create_sess does not point to a valid create-session function!\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        ret = pool->ops->create_sess(pool, &sess, &sess_para);
    }else{
        error_print("__backend_proxy_sess_msg_process_create_ver1 failed: the IP version is not valid!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

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

    msg_seg = sess_msg_seg_alloc(data_len, SESS_MSG_SEG_SHARED_MEM, msg, mem_pool);

    if(NULL == msg_seg){
        error_print("backend_proxy_data_msg_process failed: insurficient memory resource for allocing message segment!");
        return BACKEND_PROXY_PROCESS_ERROR;   
    }

/*
 * Insert the message segment into the front-to-end message queue.
 */
    SESS_MSG_SEG_INSERT_QUEUE(sess, msg_seg, f2b);

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

    if(payload_len != proxy_msg_hdr->payload_len){
        error_print("build_proxy_data_message failed: payload length does not match expected value based on message type, action type and IP version!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

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
 *                               On success, points to a newly allocated buffer containing the complete message.
 *                               Caller must free this memory with appropriate function (e.g., free()) when done.
 *                               Must not be NULL.
 * @return int                   Returns BACKEND_PROXY_PROCESS_OK (0) on successful message construction;
 *                               Returns BACKEND_PROXY_PROCESS_ERROR (-1) if any parameter is invalid (e.g., NULL pointers,
 *                               mismatched lengths) or memory allocation fails
 */
int build_proxy_general_message(BackendEngine *engine, GeneralProxyMsgHeader *header, const uint8_t *payload, size_t payload_len, uint8_t **result_msg){
    uint8_t         *msg_buf;
    uint64_t        mem_addr;
    uint16_t        proxy_msg_payload_len;
    ProxyMsgType    outer_msg_type;
    ProxyMsgHeader  *proxy_msg_hdr;
    DevMsgHeader    *dev_hdr;
    StrgyMsgHeader  *strgy_hdr;
    SessMsgHeader   *sess_hdr;
    int             ret;

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
    mem_addr = alloc_shared_mem(engine->mem_pool);
    if(ERROR_SHARED_MEM_ADDR == mem_addr){
        error_print("build_proxy_general_message failed: failed to allocate shared memory!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    msg_buf         = (uint8_t *)mem_addr;
    *result_msg     = msg_buf;

/*
 * Fill the proxy message header.
 */
    proxy_msg_hdr                       = (ProxyMsgHeader *)msg_buf;
    proxy_msg_hdr->version              = header->outer_header.version;
    proxy_msg_hdr->proxy_msg_type       = outer_msg_type = header->outer_header.proxy_msg_type;
    proxy_msg_hdr->frontend_sess_id     = header->outer_header.frontend_sess_id;
    proxy_msg_hdr->backend_sess_id      = header->outer_header.backend_sess_id;
    
    msg_buf += sizeof(ProxyMsgHeader);
    switch(outer_msg_type) {
        case PROXY_MSG_TYPE_DEV:
            dev_hdr               = &header->inner_header.dev_hdr;
            proxy_msg_payload_len = sizeof(DevMsgHeader);
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
            ret = build_proxy_sess_message(sess_hdr, payload, payload_len, &msg_buf);
            break;
        case PROXY_MSG_TYPE_DATA:
            proxy_msg_payload_len = 0;
            ret = build_proxy_data_message(proxy_msg_hdr, payload, payload_len, &msg_buf);
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
        free_shared_mem(engine->mem_pool, mem_addr);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Compute the payload length and fill it into the corresponding field of the proxy message header.
 */
    proxy_msg_hdr->payload_len = payload_len + proxy_msg_payload_len;

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

    ret = build_proxy_general_message(eng, &header, payload_data, sizeof(SessOpRespData), msg);
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

    ret = build_proxy_general_message(eng, &header, payload_data, sizeof(SessOpRespData), msg);

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

    tx_queue    = eng->tx_queue;
    msg_header  = (ProxyMsgHeader *)msg;
    
    ret = shared_mem_pool_queue_send(tx_queue, &msg, PROXY_MSG_TOTAL_SIZE(msg_header));

    if(BACKEND_PROXY_PROCESS_OK != ret){
        free_shared_mem(eng->mem_pool, (uint64_t)msg);
        error_print("backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem failed: failed to push the message into the tx queue!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

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

    eng = sess->eng;
    tx_queue = sess->eng->tx_queue;

    ret = shared_mem_pool_queue_send(tx_queue, &msg, PROXY_MSG_TOTAL_SIZE(msg_header));

    if(BACKEND_PROXY_PROCESS_OK != ret){
        free_shared_mem(eng->mem_pool, (uint64_t)msg);
        error_print("backend_proxy_send_sess_msg_to_frontend_via_shmem failed: failed to push the message into the tx queue!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}