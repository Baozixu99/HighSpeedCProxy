#include "engine.h"
#include "iniparser.h"
#include "poller.h"
#include "backend_proto.h"
#include "session.h"
#include "session_pool.h"

BackendEngine *p_g_bk_eng;
BackendEngine g_bk_eng;

HSDevSelector *p_hs_dev_sel;
HSDevSelector  hs_dev_sel[HS_DEV_SELECTOR_NUM];



 /*
  * Set of operational functions for the backend engine in high-speed network devices.
  *
  * @enable_hs_net_dev: Enables high-speed network devices according to the mask parameter.
  * @disable_hs_net_dev: Disables high-speed network devices according to the mask parameter.
  * @query_hs_net_dev: Queries all active high-speed network devices and records the result in the mask parameter.
  * @conf_hs_net_selector: Configure the algorithm for the high-speed network device selector by passing the selector ID.
  * @query_hs_net_selector: Query the selector ID of the active algorithm for the high-speed network device selector.
  *
  */

int enable_hs_net_dev(BackendEngine *eng, uint16_t mask){
    struct HighSpeedNetDeviceSet *set;
    uint32_t cnt;
    uint8_t bit_pos;

    if(NULL == eng || 0 == eng->dev_num){
        error_print("enable_hs_net_dev() returns an error because the engine pointer is NULL, or \
                     there is no high-speed network device configured in the INI file!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


    bit_pos = 0;
    while(bit_pos < MAX_HS_DEV_NUM){
    /*
     * Enable the high-speed network device by 
     */
        if (mask & (1u << bit_pos)) {
        }
    }

    return BACKEND_PROXY_PROCESS_OK;
}


int disable_hs_net_dev(BackendEngine *eng, uint16_t mask){
    struct HighSpeedNetDeviceSet *set;
    uint32_t cnt;

    if(NULL == eng){
        error_print("disable_hs_net_dev() returns an error because the engine pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


int query_hs_net_dev(BackendEngine *eng, uint16_t *mask){
    struct HighSpeedNetDeviceSet *set;
    uint32_t cnt;

    if(NULL == eng || NULL == mask){
        error_print("query_hs_net_dev() returns an error because the engine or mask pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


int choose_hs_net_dev(BackendEngine *eng, uint16_t *dev_id){
    struct HighSpeedNetDeviceSet *set;
    HSDevSelector *sel;
    int selector_id, ret;
    uint16_t target_id;

    if(NULL == eng || NULL == dev_id){
        error_print("choose_hs_net_dev() returns an error because the engine or dev_id pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    selector_id = eng->selector_id;
    sel = &hs_dev_sel[selector_id];

    if(NULL == sel->choose_dev){
        error_print("choose_hs_net_dev() returns an error because the choose_dev is not initialized!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
/*
 * Call the selector function to choose the most appropriate device, and return its dev ID.
 * If the selector function returns unsuccessfully (i.e., its return value does not indicate success) or the target device ID is out of bounds, return an error.
 */
    ret = sel->choose_dev(eng, &target_id);

    if(BACKEND_PROXY_PROCESS_OK != ret || target_id >= eng->dev_num){
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


int conf_hs_net_selector(BackendEngine *eng, uint16_t sel_id){
    return BACKEND_PROXY_PROCESS_OK;
}


int query_hs_net_selector(BackendEngine *eng, uint16_t *sel_id){
    return BACKEND_PROXY_PROCESS_OK;
}


struct BackendEngOps hs_backend_eng_ops  = {
    .enable_dev         =   enable_hs_net_dev,
    .disable_dev        =   disable_hs_net_dev,
    .query_dev          =   query_hs_net_dev,
    .choose_dev         =   choose_hs_net_dev,
    .conf_dev_selector  =   conf_hs_net_selector,
    .query_dev_sel_id   =   query_hs_net_selector
};


BackendEngine *get_global_backend_engine(){
    return p_g_bk_eng;
}


int engine_init_eng_ops(BackendEngine *eng){
    if(NULL == eng){
        error_print("engine_init_eng_ops() returns an error because the pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    eng->ops = &hs_backend_eng_ops;
    return BACKEND_PROXY_PROCESS_OK;
}

/*
 * Set of high-speed device selector functions.
 */

int choose_dev_round_robin(BackendEngine *eng, uint16_t *dev_id){
    uint32_t target_id = 0, active_mask;
    static int last_pos = -1;
    int cnt, dev_num;
    bool find_dev = false;
    struct HighSpeedNetDeviceSet *set;
    struct HighSpeedNetDevice *net_dev;


    if(NULL == eng || NULL == dev_id){
        error_print("choose_dev_round_robin() returns an error because at least one pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


 
    dev_num = eng->dev_num;
    if(0 == dev_num){
        error_print("choose_dev_round_robin() returns an error because there is no high-speed network device owned by the backend engine!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    active_mask = eng->active_mask;
    cnt = 0;
/*
 * If last_pos equals -1, it means the choose_dev_round_robin function is called for the first time.
 * Otherwise (for non-first calls), execute the regular logic for round-robin device selection
 */
    if(-1 == last_pos){
        while(cnt < dev_num){
            if(active_mask &= 1u << cnt){
                find_dev = true;
                last_pos = cnt;
                break;
            }
            cnt++;
        }
    }// if(-1 == last_pos)
    else{
        while(cnt< dev_num){
            last_pos++;
            if(dev_num == last_pos)
                last_pos = 0;

            if(active_mask &= 1u << last_pos){
                find_dev = true;
                break;
            }
            cnt++;
        }
    }// else

    if(false == find_dev){
        error_print("choose_dev_round_robin() returns an error because no active high-speed network device is found!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    set = eng->dev_set;
    net_dev = &set->hs_net_dev[last_pos];
    target_id = net_dev->dev_id;
    *dev_id = target_id;

    return BACKEND_PROXY_PROCESS_OK;
}

int choose_dev_latency_first(BackendEngine *eng, uint16_t *dev_id){
    int target_id = 0;

    if(NULL == eng || NULL == dev_id){
        error_print("choose_dev_latency_first() returns an error because at least one pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    *dev_id = target_id;
    return BACKEND_PROXY_PROCESS_OK;
}

int choose_dev_throughput_first(BackendEngine *eng, uint16_t *dev_id){
    int target_id = 0;

    if(NULL == eng || NULL == dev_id){
        error_print("choose_dev_throughput_first() returns an error because at least one pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    *dev_id = target_id;
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Initialize the selector for the backend engine
 * 
 * This function initializes the selector (a strategy component) within the BackendEngine structure. 
 * The selector is responsible for choosing a specific high-speed network device when creating new sessions, 
 * based on predefined strategies. This includes initializing strategy logic, relevant configuration, 
 * and data structures required for device selection.
 * 
 * @param eng [in/out] Pointer to a BackendEngine structure. The function will initialize members 
 *                     related to the selector within this structure.
 * 
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: Selector initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Initialization failed
 * 
 * @note 1. Ensure the eng pointer points to a valid BackendEngine instance before calling this function 
 *          to prevent null pointer access.
 *       2. This function may depend on the successful initialization of high-speed network devices 
 *          (e.g., via engine_init_hs_net_dev), as the selector needs valid devices to choose from.
 *       3. The specific selection strategy logic is determined by the backend engine's configuration.
 */
int engine_init_selector(BackendEngine *eng){
    HSDevSelector *sel;

    if(NULL == eng){
        error_print("engine_init_eng_ops() returns an error because the pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    memset(&hs_dev_sel, 0, sizeof(hs_dev_sel));

    sel = &hs_dev_sel[0];
    snprintf(sel->sel_name, strlen("round_robin") + 1, "round_robin");
    sel->choose_dev = choose_dev_round_robin;

    sel = &hs_dev_sel[1];
    snprintf(sel->sel_name, strlen("latency_first") + 1, "latency_first");
    sel->choose_dev = choose_dev_latency_first;

    sel = &hs_dev_sel[2];
    snprintf(sel->sel_name, strlen("throughput_first") + 1, "throughput_first");
    sel->choose_dev = choose_dev_throughput_first;

    p_hs_dev_sel = &hs_dev_sel[0];
    eng->selector_id = 0;

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Initialize the high-speed network device of the backend engine
 * 
 * This function is used to initialize resources related to the high-speed (HS) network device 
 * in the BackendEngine structure, including but not limited to device parameter configuration, 
 * and state initialization. It lays the foundation for subsequent interactions with the high-
 * -speed network device.
 * 
 * @param eng [in/out] Pointer to a BackendEngine structure. The function will initialize members 
 *                     related to the high-speed network device within this structure.
 * 
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: High-speed network device initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Initialization failed
 * 
 * @note 1. Before calling this function, ensure that the eng pointer points to a valid BackendEngine 
 *          instance to avoid null pointer access.
 *       2. This function may depend on the initialization of other basic components; it is 
 *          recommended to call it in the correct initialization sequence.
 *
 * The high-speed network device configuration item in the INI file must comply with the following format:
 * [Device Name]
 * ip_addr = IP address
 * dev_id = device ID
 * dev_type = device type
 * dev_status = device status
 * ns_id = namespace ID
 *
 * Example:
 * [ens01]
 * ip_addr =        192.168.10.10
 * dev_id  =        0
 * dev_type =       0
 * dev_status =     1
 * ns_id =          100
 */
int engine_init_hs_net_dev(BackendEngine *eng){
    struct HighSpeedNetDeviceSet *set = NULL;
    struct HighSpeedNetDevice *hs_dev;
    dictionary *ini;
    int dev_num, dev_name_len, cnt = 0;
    const char *dev_name, *ip_addr;
    int ip_type, dev_id, dev_type, dev_status, ns_id;
    struct in_addr in4_addr;
    struct in6_addr in6_addr;
    union IPAddress *ip_data;

    char dev_pro_item[MAX_DEV_NAME + MAX_DEVICE_PROPERTY_NAME_LENGTH];

    set = malloc(sizeof(struct HighSpeedNetDeviceSet));
    if(NULL == set){
        error_print("engine_init_hs_net_dev() returns an error, because of allocating memory for device set failed!\n");
        goto hs_net_error;
    }

//    ini = iniparser_load(HS_NET_DEV_CFG);
    ini = iniparser_load("hs_net_dev.ini");
    if (NULL == ini) {
        error_print("engine_init_hs_net_dev() returns an error, because of opening INI file failed!\n");
        goto hs_net_error;
    }

/*
 * Get the number of [Device Name] sections.
 */
    dev_num = iniparser_getnsec(ini);
    if(0 == dev_num){
        error_print("engine_init_hs_net_dev() returns an error because no high-speed network device information exists in the INI file!\n");
        goto hs_net_error;
    }


    if(dev_num > MAX_HS_DEV_NUM){
        error_print("engine_init_hs_net_dev() returns an error because the number of high-speed network device exceeds MAX_HS_DEV_NUM!\n");
        goto hs_net_error;
    }

/*
 * Initialize high speed network device one by one.
 */
    eng->active_mask = 0;
    for(; cnt < dev_num; cnt++){
        dev_name = iniparser_getsecname(ini, cnt);
        dev_name_len = strlen(dev_name);
/*
 * Make sure the length of the device name in the INI file does not exceed MAX_DEV_NAME.
 */
        if(dev_name_len > MAX_DEV_NAME){
            error_print("engine_init_hs_net_dev() returns an error because the high-speed network device names in the INI file are not valid!\n");
            goto hs_net_error;
        }

        hs_dev = &set->hs_net_dev[cnt];
        snprintf(hs_dev->name, dev_name_len, "%s", dev_name);

        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("ip_addr"), "%s:ip_addr", dev_name);
        ip_addr = iniparser_getstring(ini, dev_pro_item, NULL);
        if(NULL == ip_addr){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device without an IP address configured in the INI file!\n");
            goto hs_net_error;
        }


/*
 *Check if the string in the ip_addr item is valid.
 * If valid, convert it to an IPv4 or IPv6 address.
 */
        ip_type = DEV_IP_TYPE(ip_addr);
        if(SESS_NON_IP_PROTO == ip_type){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device with an invalid IP address string configured in the INI file!\n");
            goto hs_net_error;
        }else if(SESS_IPV4_PROTO == ip_type){
            inet_pton(AF_INET, ip_addr, &in4_addr);
            ip_data = &hs_dev->address;
            COPY_IN_TO_IPV4(&ip_data->ipv4_addr, &in4_addr);
        }else{
            inet_pton(AF_INET6, ip_addr, &in6_addr);
            ip_data = &hs_dev->address;
            COPY_IN6_TO_IPV6(&ip_data->ipv6_addr, &in6_addr);
        }

/*
 * Check if the content of the dev_id item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_id"), "%s:dev_id", dev_name);
        dev_id = iniparser_getint(ini, dev_pro_item, -1);

        if(-1 == dev_id){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev ID is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->dev_id = dev_id;

/*
 * Check if the content of the dev_type item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_type"), "%s:dev_type", dev_name);
        dev_type = iniparser_getint(ini, dev_pro_item, -1);

        if(!IS_VALID_HS_NET_DEV_TYPE(dev_type)){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev type is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->dev_type = dev_type;
/*
 * Check if the content of the dev_status item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_status"), "%s:dev_status", dev_name);
        dev_status = iniparser_getint(ini, dev_pro_item, -1);

        if(!IS_VALID_HS_NET_DEV_STATUS(dev_status)){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev status is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        if(HS_NET_DEV_ACTIVE == dev_status){
            eng->active_mask |= 1u << cnt;
        }

        hs_dev->dev_status = dev_status;
/*
 * Check if the content of the ns_id item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("ns_id"), "%s:ns_id", dev_name);
        ns_id = iniparser_getint(ini, dev_pro_item, -1);
        if(-1 == ns_id){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev ID is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->ns_id = ns_id;

        TAILQ_INIT(&hs_dev->conn_q);

    }//  for(; cnt < dev_num; cnt++)

    eng->dev_set = set;
    eng->dev_num = cnt;


/*
 * Reclaim memory resources.
 */
    iniparser_freedict(ini);


    return BACKEND_PROXY_PROCESS_OK;
hs_net_error:
    if(NULL != ini)
        iniparser_freedict(ini);

    if(NULL != set)
        free(set);

    return BACKEND_PROXY_PROCESS_ERROR;
}

/**
 * @brief Initialize the session pool of the backend engine
 * 
 * This function initializes the session pool component within the BackendEngine structure. 
 * The session pool manages a collection of reusable session objects to optimize resource usage, 
 * including pre-allocating session instances, setting up pool capacity limits, initializing 
 * session metadata, and establishing mechanisms for session acquisition and release during runtime.
 * It serves as a core component for efficient session lifecycle management in high-speed network interactions.
 * 
 * @param eng [in/out] Pointer to a BackendEngine structure. The function initializes members 
 *                     associated with the session pool (e.g., pool size, available sessions list) 
 *                     within this structure.
 * 
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Session pool initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Initialization failed (e.g., insufficient memory, invalid configuration)
 * 
 * @note 1. Ensure the eng pointer points to a valid BackendEngine instance before invocation to avoid null pointer issues.
 *       2. This function may depend on prior successful initialization of related components (e.g., 
 *          engine_init_selector, engine_init_hs_net_dev), as sessions in the pool typically interact 
 *          with high-speed network devices selected via the selector strategy.
 *       3. Session pool parameters (e.g., maximum capacity) are usually determined by the backend engine's configuration settings.
 */
int engine_init_sess_pool(BackendEngine *eng){
    struct BackendSessionPool *sess_pool = NULL;

/*
 * Initialize the session pool.
 */
    sess_pool = (struct BackendSessionPool*)malloc(sizeof(struct BackendSessionPool));
    if(NULL == sess_pool){
        error_print("engine_init_sess_pool() returns an error because there is not enough memory to allocate the session pool.\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    high_speed_init_pool(sess_pool);

    eng->sess_pool = high_speed_pool = sess_pool;

    return BACKEND_PROXY_PROCESS_OK;
}


int engine_init_shared_mem_pool(BackendEngine *eng){
    struct SharedMemoryPool *mem_pool;
    int ret;
/*
 * Initialize the shared memory pool.
 */
    mem_pool = malloc(sizeof(struct SharedMemoryPool));
    if(NULL == mem_pool){
        error_print("engine_init_sess_pool() returns an error because there is not enough memory to allocate the shared memory pool.\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    ret = init_shared_mem_pool(mem_pool);
    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_sess_pool() returns an error because it cannot initialize the shared memory pool successfully.\n");
        free(mem_pool);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    eng->mem_pool = mem_pool;

    return BACKEND_PROXY_PROCESS_OK;
}


int engine_init_shared_mem_pool_lock(BackendEngine *eng){
    struct SharedMemoryPoolLock *mem_pool_lock;
    struct SharedMemoryPool     *mem_pool;
    int ret;
/*
 * Initialize the shared memory pool lock.
 */
    mem_pool_lock = malloc(sizeof(struct SharedMemoryPoolLock));
    mem_pool      = eng->mem_pool;
    if(NULL == mem_pool_lock){
        error_print("engine_init_sess_pool() returns an error because there is not enough memory to allocate the shared memory pool lock.\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    ret = init_shared_mem_pool_lock(mem_pool_lock);
    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_sess_pool() returns an error because it cannot initialize the shared memory pool lock successfully.\n");
        free(mem_pool);
        free(mem_pool_lock);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


int engine_init_poller(BackendEngine *eng){


    return poller_init(&eng->poller);
};


void engine_init()
{
    int ret;

    p_g_bk_eng = &g_bk_eng;
    memset(p_g_bk_eng, 0, sizeof(BackendEngine));

    ret = engine_init_hs_net_dev(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_hs_net_dev() returns error!");
        return;
    }

    ret = engine_init_sess_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_sess_pool() returns error!");
        return;
    }

    ret = engine_init_shared_mem_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_shared_mem_pool() returns error!");
        return;
    }

    ret = engine_init_shared_mem_pool_lock(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_shared_mem_pool_lock() returns error!");
        return;
    }

    ret = engine_init_poller(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_poller() returns error!");
        return;
    }
}


/**
 * @brief Get data from the specified RX queue (residing in shared memory)
 * @param queue Pointer to the SharedMemoryPoolQueue (RX queue) to operate on
 * @param[out] buf_ptr Double pointer to store the address of data in shared memory
 *                     (points to actual data location in shared memory on success)
 * @param buf_max_len Maximum allowed length of data that can be retrieved (in bytes)
 * @param[out] out_len Pointer to store the actual length of obtained data (in bytes)
 * @return BACKEND_PROXY_PROCESS_OK if data is retrieved successfully;
 *         BACKEND_PROXY_PROCESS_ERROR if a system-level error occurs (e.g., invalid queue handle);
 *         BACKEND_PROXY_PROCESS_AGAIN if data is temporarily unavailable (e.g., queue is empty)
 * @note The caller is responsible for managing the lock of the shared memory pool 
 *       (lock once before multiple calls to reduce overhead)
 */
int backend_engine_rx_queue_get(struct SharedMemoryPoolQueue *queue, void **buf_ptr, 
                               size_t buf_max_len, size_t *out_len){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Send data through the specified TX queue (residing in shared memory)
 * @param queue Pointer to the SharedMemoryPoolQueue (TX queue) to operate on
 * @param[in] data_ptr Double pointer to the data in shared memory to be sent
 *                     (points to actual data location in shared memory)
 * @param data_len Length of the data to be sent (in bytes)
 * @param[out] sent_len Pointer to store the actual length of data sent (in bytes)
 * @return BACKEND_PROXY_PROCESS_OK if data is sent successfully;
 *         BACKEND_PROXY_PROCESS_ERROR if a system-level error occurs (e.g., queue access violation);
 *         BACKEND_PROXY_PROCESS_AGAIN if data cannot be sent temporarily (e.g., queue is full)
 * @note The caller is responsible for managing the lock of the shared memory pool
 *       (lock once before multiple calls to reduce overhead)
 */
int backend_engine_tx_queue_send(struct SharedMemoryPoolQueue *queue, const void **data_ptr, 
                                size_t data_len, size_t *sent_len){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Main loop function of the engine, handling message processing and data transmission cyclically
 * 
 * This function executes a continuous loop consisting of four main steps. After completing all steps,
 * it returns to the first step to implement the core operation of the backend engine.
 * 
 * @details The loop process is as follows:
 * 
 * 1. Read data from the RX queue of the shared memory queue owned by the BackendEngine instance,
 *    and process them sequentially through the backend proxy protocol stack.
 *    
 *    If any data is read, there are two cases:
 *    (a) For device messages, strategy messages, and session messages:
 *        The backend proxy protocol stack performs corresponding processing for each type, constructs
 *        response packets, and returns them to the frontend proxy through the shared memory's TX queue.
 *    (b) For sessions that receive data messages:
 *        These sessions are placed into the frontend-to-backend active queue for processing in step (2).
 *     
 *    If no data message is found, the procedure jumps to step (3).
 * 
 * 2. Access and process session instances in the frontend-to-backend active queue sequentially:
 *    Read data messages from these sessions, extract and process the payload, then send the processed
 *    payload through the socket corresponding to the session.
 * 
 * 3. Check sockets with events in the epoll list (focusing on those with received data), handling two scenarios:
 *    (a) If a socket close event is detected (e.g., TCP four-way handshake), construct a session close
 *        message and send it to the frontend proxy through the shared memory's TX queue.
 *    (b) If data is detected, read the data, construct a data message, place it into the send buffer of the
 *        session instance corresponding to the socket, and add the session instance to the
 *        backend-to-frontend active queue.
 *    
 *    If no data is found in the socket set managed by the epoll list, the function returns directly to step (1).
 * 
 * 4. Sequentially process sessions in the backend-to-frontend active queue:
 *    Read data messages from these sessions and send them to the frontend proxy through the shared memory's TX queue.
 * 
 * After completing the above four steps, the function returns to step (1) to continue the loop.
 */

void engine_run()
{
    BackendEngine                   *eng;
    struct SharedMemoryPoolQueue    *tx_queue, *rx_queue;
    struct BackendSessionQueue      *active_queue_f2b, *active_queue_b2f;
    uint8_t                         *proxy_msg;
    uint32_t                        msg_size;
    int                             ret;

    eng = get_global_backend_engine();

    if(NULL == eng){
        error_print("engine_run failed: the global backend engine is not initialized!");
        return ;
    }

    if(NULL == eng->rx_queue || NULL == eng->tx_queue){
        error_print("engine_run failed: The global backend engine's RX queue or TX queue has not been initialized!");
        return ;
    }

    rx_queue = eng->rx_queue;
    tx_queue = eng->tx_queue;
    
    BACKEND_ENGINE_GET_F2B_QUEUE(eng, active_queue_f2b);
    BACKEND_ENGINE_GET_B2F_QUEUE(eng, active_queue_b2f);

    if(NULL == active_queue_f2b || NULL == active_queue_b2f){
        error_print("engine_run failed: The global backend engine's f2b session queue or b2f session queue has not been initialized!");
        return ;
    }

    do{
/*
 * STEP (1)
 */
eng_run_step1:
/* 
 * Acquire access lock for the RX queue.
 */
    ret = SHARED_MEM_QUEUE_LOCK(rx_queue);

/* 
 * If returning BACKEND_PROXY_PROCESS_ERROR, it indicates a system-level error (e.g., invalid lock handle, shared memory pool corruption)
 * Failed to acquire the lock; print error message and exit the current flow.
 */
    if(BACKEND_PROXY_PROCESS_ERROR == ret){
        error_print("engine_run failed: failed to get the lock of the RX queue!");
        return;
    }

/* 
 * If returning BACKEND_PROXY_PROCESS_AGAIN, it indicates lock acquisition timed out (temporary unavailability, e.g., lock held by another process)
 * No error occurred; jump to eng_run_step3 to retry or proceed with alternative logic.
 */
    if(BACKEND_PROXY_PROCESS_AGAIN == ret){
        goto eng_run_step3;
    }

    do{
    /*
     * Retrieve data from the RX queue.
     */
        ret = backend_engine_rx_queue_get(rx_queue, &proxy_msg, PROXY_MSG_HDR_PLUS_MAX_SIZE, &msg_size);

    /*
     * If returning BACKEND_PROXY_PROCESS_ERROR, it indicates a system-level error (e.g., invalid queue handle, shared memory access exception, etc.)
     * Processing cannot continue; print error message and return directly.
     */
        if(BACKEND_PROXY_PROCESS_ERROR == ret){
            error_print("engine_run failed: failed to get data from RX queue!");
            return;
        }

    /*
     * If returning BACKEND_PROXY_PROCESS_AGAIN, it indicates temporary inability to retrieve data (e.g., empty queue, resource temporarily occupied, etc., non-error state)     
     * No error reporting needed; jump to eng_run_step2 to execute the next process.
     */
        if(BACKEND_PROXY_PROCESS_AGAIN == ret){
            goto eng_run_step2;
        }

    /*
     * Process the proxy message.
     */
        backend_proxy_msg_process(proxy_msg);

    }while(BACKEND_PROXY_PROCESS_OK == ret);

/*
 * STEP (2)
 */
eng_run_step2:

    BACKEND_ENGINE_GET_F2B_QUEUE(eng, active_queue_f2b);
    ;

/*
 * STEP (3)
 */
eng_run_step3:
    ;

/*
 * STEP (4)
 */
eng_run_step4:
    ;

    }while(1);
}