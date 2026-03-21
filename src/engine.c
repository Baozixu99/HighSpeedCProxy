#include "engine.h"
#include "iniparser.h"
#include "poller.h"
#include "backend_proto.h"
#include "session.h"
#include "session_pool.h"
#include "netns_socket.h"

BackendEngine *p_g_bk_eng;
BackendEngine g_bk_eng;

HSDevSelector *p_hs_dev_sel;
HSDevSelector  hs_dev_sel[HS_DEV_SELECTOR_NUM];

/**
 * Configuration structure for the high-speed network receive queue.
 * 
 * This global instance initializes parameters for managing the receive queue's shared memory,
 * including memory mapping mode, addresses, capacity, and block size.
 */
SharedMemoryPoolQueueConfig high_speed_net_rx_queue_config =     {
    .pool = NULL,
    .map_mode       = SHARE_MEM_MAP_MODE_CONTIGUOUS_BOTH,  // Assumes virtual address mapping mode
    .phy_addr       = HSNET_RX_PHY_ADDR_BASE,              // 64-bit unsigned integer zero value
    .virt_addr      = 0ULL,                                // 64-bit unsigned integer zero value
    .capacity       = MAX_MAP_TABLE_ENTRY_COUNT,           // Total number of element 
    .block_size     = 4096                                 // 4096 bytes per element block
};





/**
 * @brief Custom handler for SIGINT signal (triggered by Ctrl+C)
 * 
 * This function is registered to handle the SIGINT signal (signal number 2) 
 * which is generated when the user presses Ctrl+C in the terminal. Its core 
 * responsibility is to gracefully release all resources occupied by the engine 
 * before the process exits, avoiding resource leaks or inconsistent states.
 * 
 * @param signum Integer representing the signal number (SIGINT = 2 for Ctrl+C)
 * 
 * @details 
 * The handling process includes the following key steps:
 * 1. Log the reception of SIGINT signal for debugging and audit purposes
 * 2. Release engine-related resources (memory, file descriptors, network connections)
 * 3. Clean up associated sessions of devices through session nodes
 * 4. Ensure all resources are properly freed before process termination
 * 
 * @note 
 * 1. This function must use async-signal-safe functions only (e.g., write(), _exit())
 *    Avoid non-safe functions like printf(), malloc(), free() in production code
 * 2. The engine resource release logic must be idempotent to prevent double-free issues
 * 3. If the process should not exit after handling, remove the exit() call and return normally
 * 
 * @warning 
 * Improper resource release may lead to memory leaks or device session corruption
 */
void backend_proxy_sigint_handler(int signum){

// Exit the process gracefully after resource cleanup
    _exit(EXIT_SUCCESS);
}


/**
 * Configuration structure for the high-speed network transmit queue.
 * 
 * This global instance initializes parameters for managing the transmit queue's shared memory,
 * including memory mapping mode, addresses, capacity, and block size.
 */
SharedMemoryPoolQueueConfig high_speed_net_tx_queue_config =    {
    .pool = NULL,
    .map_mode       = SHARE_MEM_MAP_MODE_CONTIGUOUS_BOTH,  // Assumes virtual address mapping mode
    .phy_addr       = HSNET_TX_PHY_ADDR_BASE,              // 64-bit unsigned integer zero value
    .virt_addr      = 0ULL,                                // 64-bit unsigned integer zero value
    .capacity       = MAX_MAP_TABLE_ENTRY_COUNT,           // Total number of element 
    .block_size     = 4096                                 // 4096 bytes per element block
};




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
//    struct HighSpeedNetDeviceSet *set;
//    uint32_t cnt;
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
//    struct HighSpeedNetDeviceSet *set;
//    uint32_t cnt;

    if(NULL == eng){
        error_print("disable_hs_net_dev() returns an error because the engine pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


int query_hs_net_dev(BackendEngine *eng, uint16_t *mask){
//    struct HighSpeedNetDeviceSet *set;
//    uint32_t cnt;

    if(NULL == eng || NULL == mask){
        error_print("query_hs_net_dev() returns an error because the engine or mask pointer is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


int choose_hs_net_dev(BackendEngine *eng, uint16_t *dev_id){
//    struct HighSpeedNetDeviceSet *set;
    HSDevSelector *sel;
    int selector_id, ret;
    // uint16_t target_id;

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
    ret = sel->choose_dev(eng, dev_id);

    if(BACKEND_PROXY_PROCESS_OK != ret || *dev_id >= eng->dev_num){
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


struct BackendEngOps *get_hs_backend_engine_ops(){
    return &hs_backend_eng_ops;
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
        error_print("engine_init_selector failed: the engine pointer is NULL!\n");
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


int open_named_netns(const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "/var/run/netns/%s", name);
    utils_print("open_named_netns() tries to open the netns %s\n", path);
    int ns_id =  open(path, O_RDONLY);
    utils_print("open_named_netns() returns %d\n", ns_id);
    if (ns_id < 0){
        printf("open_named_netns() returns an error because the netns %s is not found!\n", name);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return ns_id;
}

/**
 * @brief Create a TCP listening socket for high-speed network devices to listen for target-matched TCP handshake requests
 * 
 * @details According to the specified IP version (IPv4/IPv6), combined with the backend engine instance (BackendEngine *),
 *          and the IP address (IPAddress) and TCP listening port (tcp_listening_port) configured in the high-speed network 
 *          device instance (struct HighSpeedNetDevice), this function creates a TCP listening socket. 
 *          The socket is specifically used to listen for TCP handshake requests (SYN packets) where the destination is 
 *          the current device, the target IP is the device's IP, and the target port is the configured listening port. 
 *          The creation process includes core operations such as socket initialization, binding, and listening, and 
 *          relies on the backend engine for necessary context support. Returns a normal status code on success and 
 *          an error status code on failure.
 * 
 * @param eng Pointer to the BackendEngine instance. Provides necessary backend context (e.g., resource management, 
 *            protocol support) for the creation of the listening socket; must be properly initialized in advance.
 * @param hs_dev Pointer to the high-speed network device instance. The IPAddress (device IP address) and 
 *               tcp_listening_port (TCP listening port) fields must be properly configured in advance.
 * @param ip_version IP protocol version, usually SESS_IPV4_PROTO (IPv4) or SESS_IPV6_PROTO (IPv6), specifying 
 *                   the IP type for the created listening socket.
 * 
 * @return int Operation result status code:
 *             - BACKEND_PROXY_PROCESS_OK: The listening socket is created successfully and is in a ready listening state.
 *             - BACKEND_PROXY_PROCESS_ERROR: Failed to create the listening socket (possible reasons include port occupation, 
 *               invalid IP/engine configuration, or failure of socket system calls).
 * 
 * @note 1. Before calling, ensure that the eng and hs_dev pointers are not NULL. The eng instance must be initialized normally,
 *          and the IPAddress and tcp_listening_port fields of hs_dev must be legally configured.
 *       2. This function is only responsible for creating the listening socket and does not handle the subsequent 
 *          connection acceptance (accept) logic, which needs to be processed separately by the caller.
 *       3. The function's execution depends on the backend engine's normal operation; if the engine is in an abnormal state,
 *          the socket creation may fail.
 */
int create_hs_net_dev_tcp_listener(BackendEngine *eng, struct HighSpeedNetDevice *hs_dev, int ip_version){
    int                 listen_fd;
//    int                 orig_netns;
    struct sockaddr_in  dev_ip_addr;
    struct in_addr      in4_addr;
    struct IPv4Address  *ipv4_addr;


/*
 * Check input parameters, and initialize IP address.
 */
    if(NULL == eng || NULL == hs_dev){
        error_print("create_hs_net_dev_tcp_listener failed: The input parameter hs_dev is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(hs_dev->tcp_listening_port <= 0){
        error_print("create_hs_net_dev_tcp_listener failed: Invalid tcp_listening_port!\n");
        hs_dev->tcp_listening_port = -1;
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(SESS_IPV4_PROTO == ip_version){
        ipv4_addr = &hs_dev->address.ipv4_addr;
        COPY_IPV4_TO_IN(&in4_addr, ipv4_addr);
    }else if(SESS_IPV6_PROTO == ip_version){
        error_print("create_hs_net_dev_tcp_listener failed: The backend protocol does not support IPv6 yet. It will be supported in the future!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }else{
        error_print("create_hs_net_dev_tcp_listener failed: Unsupported IP version!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


/*
 * Create a listening socket.
 */
    memset(&dev_ip_addr, 0, sizeof(dev_ip_addr));
    dev_ip_addr.sin_family  = AF_INET;
    dev_ip_addr.sin_port    = htons(hs_dev->tcp_listening_port);
    dev_ip_addr.sin_addr    = in4_addr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        error_print("create_hs_net_dev_tcp_listener failed: Socket create failed!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(bind(listen_fd, (struct sockaddr*)&dev_ip_addr, sizeof(dev_ip_addr)) == -1) {
        error_print("create_hs_net_dev_tcp_listener failed: Bind IP:port failed!\n");
        close(listen_fd); 
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if (listen(listen_fd, MAX_TCP_PENDING_CONN) == -1) {
        perror("listen failed");
        error_print("create_hs_net_dev_tcp_listener failed: listen failed!\n");
        close(listen_fd);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    set_nonblocking(listen_fd);
    hs_dev->tcp_listener = listen_fd;

    return BACKEND_PROXY_PROCESS_OK;
};



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
 * ip_addr    =     192.168.10.10
 * dev_id     =     0
 * dev_type   =     0
 * dev_status =     1
 * ns_id      =     100
 */
int engine_init_hs_net_dev(BackendEngine *eng){
    struct HighSpeedNetDeviceSet *set = NULL;
    struct HighSpeedNetDevice *hs_dev;
    dictionary *ini;
    int dev_num, dev_name_len, cnt = 0, cnt_node;
    const char *dev_name, *ip_addr;
    int ip_type, dev_id, dev_type, dev_status, ns_id, tcp_port;
//    int listenning_socket;
    const char *ns_name;
    struct in_addr in4_addr;
    struct in6_addr in6_addr;
    union IPAddress *ip_data;

    char dev_pro_item[MAX_DEV_NAME + MAX_DEVICE_PROPERTY_NAME_LENGTH];

    set = malloc(sizeof(struct HighSpeedNetDeviceSet));
    if(NULL == set){
        error_print("engine_init_hs_net_dev() failed: allocating memory for device set failed!");
        goto hs_net_error;
    }

    ini = iniparser_load(HS_NET_DEV_CFG);
    if (NULL == ini) {
        error_print("engine_init_hs_net_dev() failed: opening INI file failed!");
        goto hs_net_error;
    }

/*
 * Get the number of [Device Name] sections.
 */
    dev_num = iniparser_getnsec(ini);
    if(0 == dev_num){
        error_print("engine_init_hs_net_dev() failed: no high-speed network device information exists in the INI file!\n");
        goto hs_net_error;
    }

    utils_print("dev_num = %d\n", dev_num);

    if(dev_num > MAX_HS_DEV_NUM){
        error_print("engine_init_hs_net_dev() failed: the number of high-speed network device exceeds MAX_HS_DEV_NUM!\n");
        goto hs_net_error;
    }

/*
 * Initialize high speed network device one by one.
 */
    eng->active_mask = 0;
    for(; cnt < dev_num; cnt++){
        dev_name = iniparser_getsecname(ini, cnt);
        dev_name_len = strlen(dev_name);
        utils_print("dev_name = %s, dev_name_len = %d\n", dev_name, dev_name_len);
/*
 * Make sure the length of the device name in the INI file does not exceed MAX_DEV_NAME.
 */
        if(dev_name_len > MAX_DEV_NAME){
            error_print("engine_init_hs_net_dev() failed: high-speed network device name in INI file exceeds maximum length!");
            goto hs_net_error;
        }

        hs_dev = &set->hs_net_dev[cnt];
        snprintf(hs_dev->name, dev_name_len, "%s", dev_name);

        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("ip_addr") + 2, "%s:ip_addr", dev_name);
        utils_print("dev_pro_item = %s, dev_name_len+ strlen(ip_addr) = %ld\n", dev_pro_item, dev_name_len + strlen("ip_addr"));

        ip_addr = iniparser_getstring(ini, dev_pro_item, NULL);

        utils_print("ip_addr = %s\n", ip_addr);

        if(NULL == ip_addr){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device without an IP address configured in the INI file!\n");
            goto hs_net_error;
        }


/*
 * Check if the string in the ip_addr item is valid.
 * If valid, convert it to an IPv4 or IPv6 address.
 */
        ip_type = DEV_IP_TYPE(ip_addr);
        if(SESS_NON_IP_PROTO == ip_type){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device with an invalid IP address string configured in the INI file!\n");
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
        snprintf(dev_pro_item, dev_name_len + strlen("dev_id") + 2, "%s:dev_id", dev_name);
        dev_id = iniparser_getint(ini, dev_pro_item, -1);

        if(-1 == dev_id){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device for which the dev ID is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        utils_print("dev_id = %d\n", dev_id);

        hs_dev->dev_id = dev_id;

/*
 * Check if the content of the dev_type item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_type") + 2, "%s:dev_type", dev_name);
        dev_type = iniparser_getint(ini, dev_pro_item, -1);

        if(!IS_VALID_HS_NET_DEV_TYPE(dev_type)){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device for which the dev type is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        

        hs_dev->dev_type = dev_type;
/*
 * Check if the content of the dev_status item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_status") + 2, "%s:dev_status", dev_name);
        dev_status = iniparser_getint(ini, dev_pro_item, -1);

        utils_print("dev_status = %d\n", dev_status);

        if(!IS_VALID_HS_NET_DEV_STATUS(dev_status)){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device for which the dev status is either incorrectly configured \
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
        snprintf(dev_pro_item, dev_name_len + strlen("ns_name") + 2, "%s:ns_name", dev_name);
        ns_name = iniparser_getstring((const dictionary*)ini, (const char*)dev_pro_item, NULL);
        if(NULL == ns_name){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device for which the ns_name is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        utils_print("ns_name = %s\n", ns_name);

        hs_dev->ns_name = ns_name;

#if 0        
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("ns_id") + 2, "%s:ns_id", dev_name);
        ns_id = iniparser_getint(ini, dev_pro_item, -1);
        if(-1 == ns_id){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device for which the dev ID is either incorrectly configured \
                         or not configured in the INI file.\n");
            goto hs_net_error;
        }
#endif 
        ns_id = open_named_netns(hs_dev->ns_name);
        
        if(ERROR_NAMESPACE_ID == ns_id){
            error_print("engine_init_hs_net_dev() failed: there is at least one high-speed network device for which the ns_id is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->ns_id = ns_id;
        utils_print("ns_id = %d\n", hs_dev->ns_id);
        TAILQ_INIT(&hs_dev->tcp_node_queue);
        TAILQ_INIT(&hs_dev->udp_node_queue);
        TAILQ_INIT(&hs_dev->free_node_queue);

/*
 * Optional
 * If tcp_listening_port is configured in config file, a listening socket will be created on Device IP:tcp_listen_port for new connections.
 * The backend protocol can establish connections passively and notify the frontend of a new connection establishment.
 */

        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("tcp_listening_port") + 2, "%s:tcp_listening_port", dev_name);

        tcp_port = iniparser_getint(ini, dev_pro_item, -1);

        hs_dev->tcp_listening_port      = 0;
        hs_dev->tcp_listener            = -1;
        utils_print("tcp_port = %d\n", tcp_port);        

        if(tcp_port > 0){

        }else{
            hs_dev->tcp_listening_port      = 0;
            hs_dev->tcp_listener            = -1;
        }


    //    Loop to allocate MAX_SESS_NODE_NUM SessionNode instances and insert them into free_node queue
        cnt_node = 0;
        for (; cnt_node < MAX_SESS_NODE_NUM; cnt_node++){
    //  Allocate memory for a single SessionNode
            struct SessionNode *node = (struct SessionNode *)malloc(sizeof(struct SessionNode));
//            utils_print("In %s, the address of the sess_node is %p\n",  __func__, node);
            if (node == NULL) {
            // Memory allocation failed: release all allocated nodes to avoid memory leak
                struct SessionNode *tmp_node = NULL;
                struct SessionNode *next_node = NULL;

            //    Safely traverse the free_node queue and release all inserted nodes
                TAILQ_FOREACH_SAFE(tmp_node, &hs_dev->free_node_queue, entry, next_node) {
                    TAILQ_REMOVE(&hs_dev->free_node_queue, tmp_node, entry);
                    free(tmp_node);
                }
                
                goto hs_net_error;
            } // if (node == NULL)
            node->sess = NULL;
            TAILQ_INSERT_TAIL(&hs_dev->free_node_queue, node, entry);
        }// for (; cnt_node < MAX_SESS_NODE_NUM; cnt_node++)

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
 * @brief Convert string device type to IotDevType enumeration
 * @param type_str String representation of device type (bluetooth/can/zigbee/lora)
 * @return Corresponding IotDevType enum value, IOT_DEV_TYPE_UNKNOWN if not matched
 */
IotProtoType dev_type_str_to_enum(const char *type_str) {
    if (strcmp(type_str, "bluetooth") == 0) return IOT_PROTO_TYPE_BLUETOOTH;
    if (strcmp(type_str, "can") == 0) return IOT_PROTO_TYPE_CAN;
    if (strcmp(type_str, "zigbee") == 0) return IOT_PROTO_TYPE_ZIGBEE;
    if (strcmp(type_str, "lora") == 0) return IOT_PROTO_TYPE_LORA;
    return IOT_DEV_TYPE_UNKNOWN;
}


/**
 * @brief Convert string device status to IotDevStatus enumeration
 * @param status_str String representation of device status (online/offline/error/configuring)
 * @return Corresponding IotDevStatus enum value, IOT_DEV_STATUS_OFFLINE if not matched
 */
IotDevStatus dev_status_str_to_enum(const char *status_str) {
    if (strcmp(status_str, "online") == 0) return IOT_DEV_STATUS_ONLINE;
    if (strcmp(status_str, "offline") == 0) return IOT_DEV_STATUS_OFFLINE;
    if (strcmp(status_str, "error") == 0) return IOT_DEV_STATUS_ERROR;
    if (strcmp(status_str, "configuring") == 0) return IOT_DEV_STATUS_CONFIGURING;
    return IOT_DEV_STATUS_OFFLINE;
}


/**
 * @brief Parse Bluetooth specific attributes from INI section
 * @param ini Pointer to iniparser dictionary object
 * @param section Name of the target INI section (e.g., "device_101")
 * @param bt_attr Pointer to BluetoothDevAttr structure to store parsed data
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: Bluetooth attributes parsed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Parsing failed (e.g., invalid MAC address format, out-of-range values)
 * 
 * @note The Bluetooth specific attributes in INI file must comply with the following format:
 * [device_<dev_id>]
 * bt_port = Bluetooth port/channel number (integer, e.g., 18)
 * bt_version = Bluetooth version (integer, e.g., 5 for BLE 5.0)
 * conn_interval = BLE connection interval in milliseconds (integer, e.g., 50)
 * bt_mac = Bluetooth MAC address (string, format: XX:XX:XX:XX:XX:XX, e.g., 00:12:34:56:78:9A)
 * 
 * Example:
 * [device_101]
 * bt_port = 18
 * bt_version = 5
 * conn_interval = 50
 * bt_mac = 00:12:34:56:78:9A
 */
int parse_bluetooth_attr(dictionary *ini, const char *section, BluetoothDevAttr *bt_attr) {
    char key[128];
    snprintf(key, sizeof(key), "%s:bt_port", section);
    bt_attr->bt_port = iniparser_getint(ini, key, 0);

    snprintf(key, sizeof(key), "%s:bt_version", section);
    bt_attr->bt_version = (uint8_t)iniparser_getint(ini, key, 5);

    snprintf(key, sizeof(key), "%s:conn_interval", section);
    bt_attr->conn_interval = iniparser_getint(ini, key, 50);
    
    // Parse MAC address (format: 00:12:34:56:78:9A -> byte array)
    snprintf(key, sizeof(key), "%s:bt_mac", section);
    const char *mac_str = iniparser_getstring(ini, key, "00:00:00:00:00:00");
    sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &bt_attr->bt_mac[0], &bt_attr->bt_mac[1], &bt_attr->bt_mac[2],
           &bt_attr->bt_mac[3], &bt_attr->bt_mac[4], &bt_attr->bt_mac[5]);
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Parse CAN specific attributes from INI section
 * @param ini Pointer to iniparser dictionary object
 * @param section Name of the target INI section (e.g., "device_102")
 * @param can_attr Pointer to CANDevAttr structure to store parsed data
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: CAN attributes parsed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Parsing failed (e.g., invalid bitrate, unsupported mode)
 * 
 * @note The CAN specific attributes in INI file must comply with the following format:
 * [device_<dev_id>]
 * can_port = CAN port number (integer, e.g., 0)
 * can_bitrate = CAN bus bitrate in bps (integer, e.g., 500000 for 500K)
 * can_mode = CAN working mode (string, normal/loopback/silent, e.g., normal)
 * can_filter_id = CAN filter ID (hex/integer, e.g., 0x12345678 or 305419896)
 * 
 * Example:
 * [device_102]
 * can_port = 0
 * can_bitrate = 500000
 * can_mode = normal
 * can_filter_id = 0x12345678
 */
int parse_can_attr(dictionary *ini, const char *section, CANDevAttr *can_attr) {
    char key[128];
    snprintf(key, sizeof(key), "%s:can_port", section);
    can_attr->can_port = iniparser_getint(ini, key, 0);

    snprintf(key, sizeof(key), "%s:can_bitrate", section);
    can_attr->can_bitrate = iniparser_getint(ini, key, 500000);
    
    // Validate CAN bitrate (common values: 125000, 250000, 500000, 1000000)
    if (can_attr->can_bitrate != 125000 && can_attr->can_bitrate != 250000 &&
        can_attr->can_bitrate != 500000 && can_attr->can_bitrate != 1000000) {
        fprintf(stderr, "Unsupported CAN bitrate %u for section: %s\n", can_attr->can_bitrate, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    // Parse CAN mode (normal/loopback/silent)
    snprintf(key, sizeof(key), "%s:can_mode", section);
    const char *mode_str = iniparser_getstring(ini, key, "normal");
    if (strcmp(mode_str, "loopback") == 0) {
        can_attr->can_mode = 2;
    } else if (strcmp(mode_str, "silent") == 0) {
        can_attr->can_mode = 3;
    } else if (strcmp(mode_str, "normal") == 0) {
        can_attr->can_mode = 1;
    } else {
        fprintf(stderr, "Unsupported CAN mode '%s' for section: %s\n", mode_str, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    snprintf(key, sizeof(key), "%s:can_filter_id", section);
    can_attr->can_filter_id = iniparser_getint(ini, key, 0);
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Parse Zigbee specific attributes from INI section
 * @param ini Pointer to iniparser dictionary object
 * @param section Name of the target INI section (e.g., "device_103")
 * @param zigbee_attr Pointer to ZigbeeDevAttr structure to store parsed data
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: Zigbee attributes parsed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Parsing failed (e.g., invalid channel, invalid MAC address)
 * 
 * @note The Zigbee specific attributes in INI file must comply with the following format:
 * [device_<dev_id>]
 * zigbee_pan_id = Zigbee PAN ID (hex/integer, e.g., 0x1234 or 4660)
 * zigbee_channel = Zigbee channel (integer, 11-26, e.g., 18)
 * zigbee_role = Zigbee device role (string, coordinator/router/enddevice, e.g., coordinator)
 * zigbee_mac = Zigbee MAC address (string, format: XX:XX:XX:XX:XX:XX:XX:XX, e.g., 00:12:34:56:78:9A:BC:DE)
 * 
 * Example:
 * [device_103]
 * zigbee_pan_id = 0x1234
 * zigbee_channel = 18
 * zigbee_role = coordinator
 * zigbee_mac = 00:12:34:56:78:9A:BC:DE
 */
int parse_zigbee_attr(dictionary *ini, const char *section, ZigbeeDevAttr *zigbee_attr) {
    char key[128];
    snprintf(key, sizeof(key), "%s:zigbee_pan_id", section);
    zigbee_attr->zigbee_pan_id = iniparser_getint(ini, key, 0x1234);

    snprintf(key, sizeof(key), "%s:zigbee_channel", section);
    zigbee_attr->zigbee_channel = (uint8_t)iniparser_getint(ini, key, 18);
    
    // Validate Zigbee channel (11-26)
    if (zigbee_attr->zigbee_channel < 11 || zigbee_attr->zigbee_channel > 26) {
        fprintf(stderr, "Invalid Zigbee channel %u (must be 11-26) for section: %s\n", zigbee_attr->zigbee_channel, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    // Parse Zigbee role (coordinator/router/enddevice)
    snprintf(key, sizeof(key), "%s:zigbee_role", section);
    const char *role_str = iniparser_getstring(ini, key, "coordinator");
    if (strcmp(role_str, "router") == 0) {
        zigbee_attr->zigbee_role = 2;
    } else if (strcmp(role_str, "enddevice") == 0) {
        zigbee_attr->zigbee_role = 3;
    } else if (strcmp(role_str, "coordinator") == 0) {
        zigbee_attr->zigbee_role = 1;
    } else {
        fprintf(stderr, "Unsupported Zigbee role '%s' for section: %s\n", role_str, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    // Parse Zigbee MAC address (8 bytes)
    snprintf(key, sizeof(key), "%s:zigbee_mac", section);
    const char *mac_str = iniparser_getstring(ini, key, "00:00:00:00:00:00:00:00");
    if (sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &zigbee_attr->zigbee_mac[0], &zigbee_attr->zigbee_mac[1], &zigbee_attr->zigbee_mac[2],
           &zigbee_attr->zigbee_mac[3], &zigbee_attr->zigbee_mac[4], &zigbee_attr->zigbee_mac[5],
           &zigbee_attr->zigbee_mac[6], &zigbee_attr->zigbee_mac[7]) != 8) {
        fprintf(stderr, "Invalid Zigbee MAC address format for section: %s\n", section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Parse LoRa specific attributes from INI section
 * @param ini Pointer to iniparser dictionary object
 * @param section Name of the target INI section (e.g., "device_104")
 * @param lora_attr Pointer to LoRaDevAttr structure to store parsed data
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: LoRa attributes parsed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Parsing failed (e.g., invalid spreading factor, unsupported frequency band)
 * 
 * @note The LoRa specific attributes in INI file must comply with the following format:
 * [device_<dev_id>]
 * lora_port = LoRa port number (integer, e.g., 2)
 * lora_freq_band = LoRa frequency band (string, EU868/US915/CN470, e.g., EU868)
 * lora_sf = LoRa spreading factor (integer, 7-12, e.g., 9)
 * lora_cr = LoRa coding rate (integer, 1-4, corresponds to 4/5 ~ 4/8, e.g., 1)
 * lora_dev_eui = LoRa device EUI (hex/integer, e.g., 0011223344556677)
 * 
 * Example:
 * [device_104]
 * lora_port = 2
 * lora_freq_band = EU868
 * lora_sf = 9
 * lora_cr = 1
 * lora_dev_eui = 0011223344556677
 */
int parse_lora_attr(dictionary *ini, const char *section, LoRaDevAttr *lora_attr) {
    char key[128];
    snprintf(key, sizeof(key), "%s:lora_port", section);
    lora_attr->lora_port = iniparser_getint(ini, key, 2);
    
    // Parse LoRa frequency band (EU868/US915/CN470)
    snprintf(key, sizeof(key), "%s:lora_freq_band", section);
    const char *freq_str = iniparser_getstring(ini, key, "EU868");
    if (strcmp(freq_str, "US915") == 0) {
        lora_attr->lora_freq_band = 2;
    } else if (strcmp(freq_str, "CN470") == 0) {
        lora_attr->lora_freq_band = 3;
    } else if (strcmp(freq_str, "EU868") == 0) {
        lora_attr->lora_freq_band = 1;
    } else {
        fprintf(stderr, "Unsupported LoRa frequency band '%s' for section: %s\n", freq_str, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    snprintf(key, sizeof(key), "%s:lora_sf", section);
    lora_attr->lora_sf = (uint8_t)iniparser_getint(ini, key, 9);
    
    // Validate LoRa spreading factor (7-12)
    if (lora_attr->lora_sf < 7 || lora_attr->lora_sf > 12) {
        fprintf(stderr, "Invalid LoRa spreading factor %u (must be 7-12) for section: %s\n", lora_attr->lora_sf, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    snprintf(key, sizeof(key), "%s:lora_cr", section);
    lora_attr->lora_cr = (uint8_t)iniparser_getint(ini, key, 1);
    
    // Validate LoRa coding rate (1-4)
    if (lora_attr->lora_cr < 1 || lora_attr->lora_cr > 4) {
        fprintf(stderr, "Invalid LoRa coding rate %u (must be 1-4) for section: %s\n", lora_attr->lora_cr, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    snprintf(key, sizeof(key), "%s:lora_dev_eui", section);
    lora_attr->lora_dev_eui = iniparser_getint(ini, key, 0);
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Parse OpenPowerLink specific attributes from INI section
 * @param ini Pointer to iniparser dictionary object
 * @param section Name of the target INI section (e.g., "device_105")
 * @param plk_attr Pointer to PowerLinkDevAttr structure to store parsed data
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: OpenPowerLink attributes parsed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Parsing failed (e.g., invalid NodeID, unsupported role, invalid MAC address)
 * 
 * @note The OpenPowerLink specific attributes in INI file must comply with the following format:
 * [device_<dev_id>]
 * plk_port = POWERLINK port number (integer, e.g., 0)
 * plk_node_id = POWERLINK NodeID (integer, 1-240, e.g., 1)
 * plk_role = POWERLINK device role (string, mn/cn, e.g., mn)
 * plk_cycle_ms = POWERLINK real-time cycle in milliseconds (integer, 1-10, e.g., 1)
 * plk_rx_pdo_len = Length of received PDO (integer, e.g., 64)
 * plk_tx_pdo_len = Length of transmitted PDO (integer, e.g., 64)
 * plk_mac = POWERLINK device MAC address (string, format: XX:XX:XX:XX:XX:XX, e.g., 00:12:34:56:78:9B)
 * 
 * Example:
 * [device_105]
 * plk_port = 0
 * plk_node_id = 1
 * plk_role = mn
 * plk_cycle_ms = 1
 * plk_rx_pdo_len = 64
 * plk_tx_pdo_len = 64
 * plk_mac = 00:12:34:56:78:9B
 */
int parse_powerlink_attr(dictionary *ini, const char *section, PowerLinkDevAttr *plk_attr) {
    char key[128];
    
    // Parse plk_port (POWERLINK port number)
    snprintf(key, sizeof(key), "%s:plk_port", section);
    plk_attr->plk_port = (uint16_t)iniparser_getint(ini, key, 0);

    // Parse plk_node_id with validation (1-240)
    snprintf(key, sizeof(key), "%s:plk_node_id", section);
    plk_attr->plk_node_id = (uint16_t)iniparser_getint(ini, key, 1);
    if (plk_attr->plk_node_id < 1 || plk_attr->plk_node_id > 240) {
        fprintf(stderr, "Invalid POWERLINK NodeID %u (must be 1-240) for section: %s\n", plk_attr->plk_node_id, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // Parse plk_role (mn/cn) with validation
    snprintf(key, sizeof(key), "%s:plk_role", section);
    const char *role_str = iniparser_getstring(ini, key, "mn");
    if (strcmp(role_str, "mn") == 0) {
        plk_attr->plk_role = 0; // MN (Managing Node)
    } else if (strcmp(role_str, "cn") == 0) {
        plk_attr->plk_role = 1; // CN (Controlled Node)
    } else {
        fprintf(stderr, "Unsupported POWERLINK role '%s' (must be mn/cn) for section: %s\n", role_str, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // Parse plk_cycle_ms with validation (1-10ms)
    snprintf(key, sizeof(key), "%s:plk_cycle_ms", section);
    plk_attr->plk_cycle_ms = (uint32_t)iniparser_getint(ini, key, 1);
    if (plk_attr->plk_cycle_ms < 1 || plk_attr->plk_cycle_ms > 10) {
        fprintf(stderr, "Invalid POWERLINK cycle %u ms (must be 1-10) for section: %s\n", plk_attr->plk_cycle_ms, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // Parse plk_rx_pdo_len (no strict validation, use common default 64)
    snprintf(key, sizeof(key), "%s:plk_rx_pdo_len", section);
    plk_attr->plk_rx_pdo_len = (uint16_t)iniparser_getint(ini, key, 64);

    // Parse plk_tx_pdo_len (no strict validation, use common default 64)
    snprintf(key, sizeof(key), "%s:plk_tx_pdo_len", section);
    plk_attr->plk_tx_pdo_len = (uint16_t)iniparser_getint(ini, key, 64);

    // Parse plk_mac with format validation (XX:XX:XX:XX:XX:XX)
    snprintf(key, sizeof(key), "%s:plk_mac", section);
    const char *mac_str = iniparser_getstring(ini, key, "00:00:00:00:00:00");
    if (sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &plk_attr->plk_mac[0], &plk_attr->plk_mac[1], &plk_attr->plk_mac[2],
           &plk_attr->plk_mac[3], &plk_attr->plk_mac[4], &plk_attr->plk_mac[5]) != 6) {
        fprintf(stderr, "Invalid POWERLINK MAC address format for section: %s (expected XX:XX:XX:XX:XX:XX)\n", section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Parse ModbusTCP specific attributes from INI section
 * @param ini Pointer to iniparser dictionary object
 * @param section Name of the target INI section (e.g., "device_106")
 * @param mb_attr Pointer to ModbusTCPDevAttr structure to store parsed data
 * @return int Result of the function execution
 *         - BACKEND_PROXY_PROCESS_OK: ModbusTCP attributes parsed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Parsing failed (e.g., invalid Unit ID, invalid MAC address)
 * 
 * @note The ModbusTCP specific attributes in INI file must comply with the following format:
 * [device_<dev_id>]
 * mb_port = ModbusTCP port number (integer, default: 502)
 * mb_unit_id = Modbus Unit ID / Slave ID (integer, 0-255)
 * mb_proto_ver = Protocol version (string: "tcp" or "rtu_over_tcp", default: "tcp")
 * mb_timeout_ms = Communication timeout in milliseconds (integer, e.g., 1000)
 * mb_retry_cnt = Request retry count (integer, 0 = no retry)
 * mb_mac = Device MAC address (string, format: XX:XX:XX:XX:XX:XX)
 * 
 * Example:
 * [device_106]
 * mb_port = 502
 * mb_unit_id = 1
 * mb_proto_ver = tcp
 * mb_timeout_ms = 1000
 * mb_retry_cnt = 3
 * mb_mac = 00:12:34:56:78:9C
 */
int parse_modbustcp_attr(dictionary *ini, const char *section, ModbusTCPDevAttr *mb_attr) {
    char key[128];

    // 1. Parse mb_port (Default: 502)
    snprintf(key, sizeof(key), "%s:mb_port", section);
    mb_attr->mb_port = (uint16_t)iniparser_getint(ini, key, 502);

    // 2. Parse mb_unit_id with validation (0-255)
    snprintf(key, sizeof(key), "%s:mb_unit_id", section);
    int unit_id_val = iniparser_getint(ini, key, 1);
    if (unit_id_val < 0 || unit_id_val > 255) {
        fprintf(stderr, "Invalid Modbus Unit ID %d (must be 0-255) for section: %s\n", unit_id_val, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    mb_attr->mb_unit_id = (uint8_t)unit_id_val;

    // 3. Parse mb_proto_ver (tcp vs rtu_over_tcp)
    snprintf(key, sizeof(key), "%s:mb_proto_ver", section);
    const char *proto_str = iniparser_getstring(ini, key, "tcp");
    if (strcmp(proto_str, "rtu_over_tcp") == 0 || strcmp(proto_str, "rtu") == 0) {
        mb_attr->mb_proto_ver = 0x01; // ModbusRTU over TCP
    } else if (strcmp(proto_str, "tcp") == 0) {
        mb_attr->mb_proto_ver = 0x00; // Standard ModbusTCP
    } else {
        fprintf(stderr, "Unsupported Modbus protocol version '%s' (use 'tcp' or 'rtu_over_tcp') for section: %s\n", proto_str, section);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 4. Parse mb_timeout_ms (Default: 1000ms)
    snprintf(key, sizeof(key), "%s:mb_timeout_ms", section);
    mb_attr->mb_timeout_ms = (uint32_t)iniparser_getint(ini, key, 1000);
    
    // Optional: Validate timeout is positive
    if (mb_attr->mb_timeout_ms == 0) {
        fprintf(stderr, "[WARN] Modbus timeout is 0 for section: %s, setting to default 1000ms\n", section);
        mb_attr->mb_timeout_ms = 1000;
    }

    // 5. Parse mb_retry_cnt (Default: 0)
    snprintf(key, sizeof(key), "%s:mb_retry_cnt", section);
    int retry_val = iniparser_getint(ini, key, 0);
    if (retry_val < 0) {
        fprintf(stderr, "Invalid retry count %d for section: %s, using 0\n", retry_val, section);
        retry_val = 0;
    }
    mb_attr->mb_retry_cnt = (uint8_t)retry_val;

    // 6. Parse mb_mac with format validation (XX:XX:XX:XX:XX:XX)
    snprintf(key, sizeof(key), "%s:mb_mac", section);
    const char *mac_str = iniparser_getstring(ini, key, "00:00:00:00:00:00");
    
    // Expect exactly 6 bytes parsed
    int parsed_bytes = sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &mb_attr->mb_mac[0], &mb_attr->mb_mac[1], &mb_attr->mb_mac[2],
           &mb_attr->mb_mac[3], &mb_attr->mb_mac[4], &mb_attr->mb_mac[5]);
           
    if (parsed_bytes != 6) {
        fprintf(stderr, "Invalid ModbusTCP MAC address format for section: %s (expected XX:XX:XX:XX:XX:XX, got: %s)\n", section, mac_str);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Initialize IoT devices from INI configuration file (iot_dev.ini)
 * @param engine Pointer to BackendEngine instance (stores parsed IoT devices in iot_dev_set)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): All devices initialized successfully OR no devices to parse
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Critical error (invalid input/INI load failed/no devices parsed)
 * 
 * @note This function:
 *       1. Allocates IoTDeviceSet memory if not initialized in the engine
 *       2. Loads and parses all [device_xxx] sections from iot_dev.ini
 *       3. Parses common device attributes (dev_id, dev_type, ns_name, etc.)
 *       4. Invokes protocol-specific parse functions (parse_bluetooth_attr, etc.)
 *       5. Stores parsed data in engine->iot_dev_set
 *       6. Cleans up iniparser resources after completion
 * 
 * @warning Ensure MAX_IOT_DEV_NUM, MAX_DEV_NAME are defined before using this function
 * @warning Call backend_engine_cleanup_iot_devices() to free allocated memory
 */
int engine_init_iot_devices(BackendEngine *engine) {
    // 1. Validate input parameters
    if (engine == NULL) {
        fprintf(stderr, "[ERROR] Invalid BackendEngine pointer (NULL)\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 2. Initialize IoT device set (allocate memory if not exists)
    if (engine->iot_dev_set == NULL) {
        engine->iot_dev_set = (IoTDeviceSet *)calloc(1, sizeof(IoTDeviceSet));
        if (engine->iot_dev_set == NULL) {
            fprintf(stderr, "[ERROR] Failed to allocate memory for IoTDeviceSet\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        memset(engine->iot_dev_set, 0, sizeof(IoTDeviceSet));
    }

    // 3. Load and parse INI configuration file
    dictionary *ini = iniparser_load(IOT_DEV_CFG);
    if (ini == NULL) {
        fprintf(stderr, "[ERROR] Failed to load configuration file: iot_dev.ini\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 4. Traverse all INI sections (filter [device_xxx] sections)
    int parsed_dev_count = 0;    // Number of successfully parsed devices
    int failed_dev_count = 0;    // Number of failed device parses
    int total_sections = iniparser_getnsec(ini);
    
    for (int i = 0; i < total_sections && parsed_dev_count < MAX_IOT_DEV_NUM; i++) {
        const char *section = iniparser_getsecname(ini, i);
        if (section == NULL || strncmp(section, "device_", 7) != 0) {
            continue;  // Skip non-device sections
        }

        // Initialize current device structure (clear memory)
        IotDevice *dev = &engine->iot_dev_set->iot_dev[parsed_dev_count];
        memset(dev, 0, sizeof(IotDevice));
        dev->dev_type = IOT_DEV_TYPE_UNKNOWN;
        dev->dev_status = IOT_DEV_STATUS_OFFLINE;
        dev->sess_id = -1;
        dev->fd = -1;
        dev->physical_port = -1;
        dev->ns_name = NULL;

        // -------------------------- Parse Common Attributes --------------------------
        char key[128];
        int parse_successful = 1;

        // 4.1 Parse dev_id (mandatory field)
        snprintf(key, sizeof(key), "%s:dev_id", section);
        dev->dev_id = iniparser_getint(ini, key, -1);
        if (dev->dev_id == -1) {
            fprintf(stderr, "[WARN] %s: Missing or invalid dev_id (skipping device)\n", section);
            parse_successful = 0;
            failed_dev_count++;
            continue;
        }

        // 4.2 Parse dev_type (mandatory field)
        snprintf(key, sizeof(key), "%s:dev_type", section);
        const char *dev_type_str = iniparser_getstring(ini, key, "unknown");
        if (strcasecmp(dev_type_str, "bluetooth") == 0) {
            dev->dev_type = IOT_PROTO_TYPE_BLUETOOTH;
        } else if (strcasecmp(dev_type_str, "zigbee") == 0) {
            dev->dev_type = IOT_PROTO_TYPE_ZIGBEE;
        } else if (strcasecmp(dev_type_str, "can") == 0) {
            dev->dev_type = IOT_PROTO_TYPE_CAN;
        } else if (strcasecmp(dev_type_str, "lora") == 0) {
            dev->dev_type = IOT_PROTO_TYPE_LORA;
        } else if (strcasecmp(dev_type_str, "powerlink") == 0) {
            dev->dev_type = IOT_PROTO_TYPE_POWERLINK;
        } else if (strcasecmp(dev_type_str, "modbustcp") == 0) {
            dev->dev_type = IOT_PROTO_TYPE_MODBUSTCP;
        }
        else {
            fprintf(stderr, "[WARN] %s: Unsupported dev_type '%s' (skipping device)\n", section, dev_type_str);
            parse_successful = 0;
            failed_dev_count++;
            continue;
        }

        // 4.3 Parse ns_name (optional, default empty string)
        snprintf(key, sizeof(key), "%s:ns_name", section);
        const char *ns_name = iniparser_getstring(ini, key, "");
        if (strlen(ns_name) > 0) {
            dev->ns_name = strdup(ns_name);  // Dynamic allocation (free in cleanup)
            if (dev->ns_name == NULL) {
                fprintf(stderr, "[WARN] %s: Failed to allocate memory for ns_name (using empty string)\n", section);
                dev->ns_name = strdup("");
            }
        } else {
            dev->ns_name = strdup("");
        }

        // 4.4 Parse dev_status (optional, default offline)
        snprintf(key, sizeof(key), "%s:dev_status", section);
        const char *status_str = iniparser_getstring(ini, key, "offline");
        if (strcasecmp(status_str, "online") == 0) {
            dev->dev_status = IOT_DEV_STATUS_ONLINE;
        } else if (strcasecmp(status_str, "error") == 0) {
            dev->dev_status = IOT_DEV_STATUS_ERROR;
        } else if (strcasecmp(status_str, "configuring") == 0) {
            dev->dev_status = IOT_DEV_STATUS_CONFIGURING;
        } else {
            dev->dev_status = IOT_DEV_STATUS_OFFLINE;
        }

        // 4.5 Parse device name (optional, default "device_<dev_id>")
        snprintf(key, sizeof(key), "%s:name", section);
        const char *dev_name = iniparser_getstring(ini, key, "");
        if (strlen(dev_name) > 0) {
            strncpy(dev->name, dev_name, MAX_DEV_NAME - 1);
        } else {
            snprintf(dev->name, MAX_DEV_NAME, "device_%d", dev->dev_id);
        }
        dev->name[MAX_DEV_NAME - 1] = '\0';

        // 4.6 Parse working_mode (optional, default client/0)
        snprintf(key, sizeof(key), "%s:working_mode", section);
        const char *work_mode = iniparser_getstring(ini, key, "client");
        dev->config.working_mode = (strcasecmp(work_mode, "gateway") == 0 || 
                                    strcasecmp(work_mode, "server") == 0) ? 1 : 0;

        // 4.7 Parse config_path (optional, default "/etc/iot/dev_<dev_id>.conf")
        snprintf(key, sizeof(key), "%s:config_path", section);
        const char *cfg_path = iniparser_getstring(ini, key, "");
        if (strlen(cfg_path) > 0) {
            strncpy(dev->config.config_path, cfg_path, sizeof(dev->config.config_path) - 1);
        } else {
            snprintf(dev->config.config_path, sizeof(dev->config.config_path), 
                     "/etc/iot/dev_%d.conf", dev->dev_id);
        }
        dev->config.config_path[sizeof(dev->config.config_path) - 1] = '\0';

        // 4.8 Parse auto_connect (optional, default 0)
        snprintf(key, sizeof(key), "%s:auto_connect", section);
        dev->config.auto_connect = iniparser_getint(ini, key, 0);

        // 4.9 Parse physical_port (optional, default -1)
        snprintf(key, sizeof(key), "%s:physical_port", section);
        dev->physical_port = iniparser_getint(ini, key, -1);

        // -------------------------- Parse Protocol-Specific Attributes --------------------------
        int proto_parse_result = BACKEND_PROXY_PROCESS_ERROR;
        switch (dev->dev_type) {
            case IOT_PROTO_TYPE_BLUETOOTH:
                proto_parse_result = parse_bluetooth_attr(ini, section, &dev->specific_attr.bt_attr);
                break;
            case IOT_PROTO_TYPE_CAN:
                proto_parse_result = parse_can_attr(ini, section, &dev->specific_attr.can_attr);
                break;
            case IOT_PROTO_TYPE_ZIGBEE:
                proto_parse_result = parse_zigbee_attr(ini, section, &dev->specific_attr.zigbee_attr);
                break;
            case IOT_PROTO_TYPE_LORA:
                proto_parse_result = parse_lora_attr(ini, section, &dev->specific_attr.lora_attr);
                break;
            case IOT_PROTO_TYPE_POWERLINK:
                proto_parse_result = parse_powerlink_attr(ini, section, &dev->specific_attr.plk_attr);
                break;
            case IOT_PROTO_TYPE_MODBUSTCP:
                proto_parse_result = parse_modbustcp_attr(ini, section, &dev->specific_attr.mb_attr);
                break;
            default:
                proto_parse_result = BACKEND_PROXY_PROCESS_ERROR;
                break;
        }

        // 4.10 Validate protocol attribute parsing result
        if (proto_parse_result != BACKEND_PROXY_PROCESS_OK) {
            fprintf(stderr, "[WARN] %s: Failed to parse %s-specific attributes (skipping device)\n",
                    section, dev_type_str);
            // Free allocated memory for current device
            if (dev->ns_name != NULL) {
                free(dev->ns_name);
                dev->ns_name = NULL;
            }
            memset(dev, 0, sizeof(IotDevice));
            failed_dev_count++;
            parse_successful = 0;
        }

        // 4.11 Count successfully parsed devices
        if (parse_successful) {
            fprintf(stdout, "[INFO] %s: Successfully parsed %s device (dev_id=%d, name=%s)\n",
                    section, dev_type_str, dev->dev_id, dev->name);
            parsed_dev_count++;
        }
    }// for (int i = 0; i < total_sections && parsed_dev_count < MAX_IOT_DEV_NUM; i++)

    // 5. Clean up iniparser resources
    iniparser_freedict(ini);

    // 6. Return standard result codes
    if (parsed_dev_count == 0) {
        fprintf(stderr, "[ERROR] No IoT devices were successfully parsed (total failed: %d)\n", failed_dev_count);
        return BACKEND_PROXY_PROCESS_ERROR;
    } else {
        if (failed_dev_count > 0) {
            fprintf(stdout, "[WARN] Partially successful initialization: %d devices parsed, %d failed\n",
                    parsed_dev_count, failed_dev_count);
            return BACKEND_PROXY_PROCESS_ERROR;
        } else {
            engine->iot_dev_num = parsed_dev_count;
            fprintf(stdout, "[INFO] All %d IoT devices initialized successfully\n", parsed_dev_count);
        }
        return BACKEND_PROXY_PROCESS_OK;
    }
}


/**
 * @brief Initialize IoT device sessions based on parsed IoT device set
 * @param engine Pointer to BackendEngine instance (uses parsed IoT devices from iot_dev_set)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): All sessions initialized successfully OR no sessions to start
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Critical error (invalid input/session creation failed)
 * 
 * @note This function:
 *       1. Depends on the successful execution of engine_init_iot_devices()
 *       2. Iterates through all valid IoT devices in engine->iot_dev_set
 *       3. Starts the corresponding session instance for each device sequentially
 *       4. Initializes protocol-specific communication sessions for connected devices
 *       5. Maintains session context for subsequent data interaction and control
 *       6. **ONLY ONE DEVICE PER PROTOCOL TYPE IS SUPPORTED** (Bluetooth/CAN/ZigBee/LoRa/PowerLink)
 * 
 * @warning Ensure engine_init_iot_devices() has been executed and returned OK before calling
 * @warning Call backend_iot_sess_destroy() to release session resources
 * @warning Do NOT call this function repeatedly without proper cleanup
 * @warning **Only one instance per IoT protocol type is supported in system**
 */
int engine_init_iot_sessions(BackendEngine *engine){
    IotDevice   *iot_dev;
    int         iot_dev_cnt, iot_dev_num, ret;

    iot_dev_cnt = 0;
    iot_dev_num = engine->iot_dev_num;

    /* Initialize global IoT session handles to NULL */
    backend_bluetooth_sess  = NULL;
    backend_can_sess        = NULL;
    backend_zigbee_sess     = NULL;
    backend_lora_sess       = NULL;
    backend_powerlink_sess  = NULL;
    backend_modbustcp_sess  = NULL;

    /* Check for invalid input pointers */
    if (NULL == engine || NULL == engine->iot_dev_set) {
        error_print("engine_init_iot_sessions failed: invalid engine pointer!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    while(iot_dev_cnt < iot_dev_num){
        iot_dev = &engine->iot_dev_set->iot_dev[iot_dev_cnt];

        /* Create session only for ONLINE devices */
        if(IOT_DEV_STATUS_ONLINE == iot_dev->dev_status){
            switch (iot_dev->dev_type) {
                case IOT_PROTO_TYPE_BLUETOOTH:
                    /* Ensure only ONE Bluetooth device instance is supported */
                    if(NULL != backend_bluetooth_sess){
                        error_print("engine_init_iot_sessions failed: only one bluetooth device is supported in backendengine!\n");
                        goto init_iot_sess_error;
                    }

                    backend_bluetooth_sess = malloc(sizeof(IoTBackendSession));

                    if(NULL == backend_bluetooth_sess){
                        error_print("engine_init_iot_sessions failed: insufficient memory for allocating backend_bluetooth_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    ret = engine_init_bluetooth_session(iot_dev, backend_bluetooth_sess);

                    if(BACKEND_PROXY_PROCESS_OK != ret){
                        error_print("engine_init_iot_sessions failed: failed to initialize backend_bluetooth_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    backend_bluetooth_sess->eng = engine;

                    break;
                case IOT_PROTO_TYPE_CAN:
                    /* Ensure only ONE CAN device instance is supported */
                    if(NULL != backend_can_sess){
                        error_print("engine_init_iot_sessions failed: only one CAN device is supported in backendengine!\n");
                        goto init_iot_sess_error;
                    }

                    backend_can_sess = malloc(sizeof(IoTBackendSession));

                    if(NULL == backend_can_sess){
                        error_print("engine_init_iot_sessions failed: insufficient memory for allocating backend_can_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    ret = engine_init_can_session(iot_dev, backend_can_sess);

                    if(BACKEND_PROXY_PROCESS_OK != ret){
                        error_print("engine_init_iot_sessions failed: failed to initialize backend_can_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    backend_can_sess->eng = engine;

                    break;
                case IOT_PROTO_TYPE_ZIGBEE:
                    /* Ensure only ONE ZigBee device instance is supported */
                    if(NULL != backend_zigbee_sess){
                        error_print("engine_init_iot_sessions failed: only one ZigBee device is supported in backendengine!\n");
                        goto init_iot_sess_error;
                    }

                    backend_zigbee_sess = malloc(sizeof(IoTBackendSession));

                    if(NULL == backend_zigbee_sess){
                        error_print("engine_init_iot_sessions failed: insufficient memory for allocating backend_zigbee_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    ret = engine_init_zigbee_session(iot_dev, backend_zigbee_sess);

                    if(BACKEND_PROXY_PROCESS_OK != ret){
                        error_print("engine_init_iot_sessions failed: failed to initialize backend_zigbee_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    backend_zigbee_sess->eng = engine;

                    break;
                case IOT_PROTO_TYPE_LORA:
                    /* Ensure only ONE LoRa device instance is supported */
                    if(NULL != backend_lora_sess){
                        error_print("engine_init_iot_sessions failed: only one LoRa device is supported in backendengine!\n");
                        goto init_iot_sess_error;
                    }

                    backend_lora_sess = malloc(sizeof(IoTBackendSession));

                    if(NULL == backend_lora_sess){
                        error_print("engine_init_iot_sessions failed: insufficient memory for allocating backend_lora_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    ret = engine_init_lora_session(iot_dev, backend_lora_sess);

                    if(BACKEND_PROXY_PROCESS_OK != ret){
                        error_print("engine_init_iot_sessions failed: failed to initialize backend_lora_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    backend_lora_sess->eng = engine;

                    break;
                case IOT_PROTO_TYPE_POWERLINK:
                    /* Ensure only ONE PowerLink device instance is supported */
                    if(NULL != backend_powerlink_sess){
                        error_print("engine_init_iot_sessions failed: only one PowerLink device is supported in backendengine!\n");
                        goto init_iot_sess_error;
                    }

                    backend_powerlink_sess = malloc(sizeof(IoTBackendSession));

                    if(NULL == backend_powerlink_sess){
                        error_print("engine_init_iot_sessions failed: insufficient memory for allocating backend_powerlink_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    ret = engine_init_powerlink_session(iot_dev, backend_powerlink_sess);

                    if(BACKEND_PROXY_PROCESS_OK != ret){
                        error_print("engine_init_iot_sessions failed: failed to initialize backend_powerlink_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    backend_powerlink_sess->eng = engine;

                    break;
                case IOT_PROTO_TYPE_MODBUSTCP:
                    /* Ensure only ONE PowerLink device instance is supported */
                    if(NULL != backend_modbustcp_sess){
                        error_print("engine_init_iot_sessions failed: only one modbusTCP device is supported in backendengine!\n");
                        goto init_iot_sess_error;
                    }

                    backend_modbustcp_sess =  malloc(sizeof(IoTBackendSession));

                    if(NULL == backend_modbustcp_sess){
                        error_print("engine_init_iot_sessions failed: insufficient memory for allocating backend_modbustcp_sess instance!\n");
                        goto init_iot_sess_error;
                    }

                    ret = engine_init_modbustcp_session(iot_dev, backend_modbustcp_sess);

                    if(BACKEND_PROXY_PROCESS_OK != ret){
                        error_print("engine_init_iot_sessions failed: failed to initialize engine_init_modbustcp_session instance!\n");
                        goto init_iot_sess_error;
                    }

                    break;
                default:
                    error_print("engine_init_iot_sessions failed: unsupported device type!\n");
                    goto init_iot_sess_error;

            } // switch (iot_dev_type)
        } // if(IOT_DEV_STATUS_ONLINE == iot_dev->dev_status)

        iot_dev_cnt++;
    }

    return BACKEND_PROXY_PROCESS_OK;

init_iot_sess_error:
    /* Free all allocated session resources on error */
    if(NULL != backend_bluetooth_sess){
        free(backend_bluetooth_sess);
        backend_bluetooth_sess = NULL;
    }
    
    if(NULL != backend_can_sess){
        free(backend_can_sess);
        backend_can_sess = NULL;
    }

    if(NULL != backend_zigbee_sess){
        free(backend_zigbee_sess);
        backend_zigbee_sess = NULL;
    }

    if(NULL != backend_lora_sess){
        free(backend_lora_sess);
        backend_lora_sess = NULL;
    }

    if(NULL != backend_powerlink_sess){
        free(backend_powerlink_sess);
        backend_powerlink_sess = NULL;
    }

    if(NULL != backend_modbustcp_sess){
        free(backend_modbustcp_sess);
        backend_modbustcp_sess = NULL;
    }

    return BACKEND_PROXY_PROCESS_ERROR;
}

/**
 * @brief Clean up IoT device resources (memory + file descriptors)
 * @param engine Pointer to BackendEngine instance
 * 
 * @note This function complements engine_init_iot_devices()
 * @note Must be called before destroying the BackendEngine to prevent memory leaks
 */
void engine_cleanup_iot_devices(BackendEngine *engine) {
    if (engine == NULL || engine->iot_dev_set == NULL) {
        return;
    }

    // Free dynamically allocated memory for each device
    for (int i = 0; i < MAX_IOT_DEV_NUM; i++) {
        IotDevice *dev = &engine->iot_dev_set->iot_dev[i];
        if (dev->ns_name != NULL) {
            free(dev->ns_name);
            dev->ns_name = NULL;
        }
        // Close device file descriptor if open
        if (dev->fd >= 0) {
            close(dev->fd);
            dev->fd = -1;
        }
        // Clear device structure
        memset(dev, 0, sizeof(IotDevice));
    }

    // Free IoT device set memory
    free(engine->iot_dev_set);
    engine->iot_dev_set = NULL;
}

/**
 * @brief Destroy all active IoT backend communication sessions
 *
 * @details This function iterates all created and running IoT device sessions
 *          including Bluetooth, CAN, ZigBee, LoRa and PowerLink.
 *          It calls backend_cleanup_iot_session() for each valid IoT session instance
 *          to clean up context, reset state and unbind from devices.
 *          This is the top-level destructor for ALL IoT sessions.
 *          NOT used for IP network device session management.
 *
 * @note This function takes no parameters and returns no value.
 *       IoTBackendSession memory is managed externally, not freed here.
 *       Only performs session context cleanup and state reset.
 *
 * @warning This function releases ALL running IoT communication sessions
 * @warning Do NOT call during ongoing IoT data transmission
 * @warning This is the reverse & top-level counterpart of engine_init_iot_session()
 */
void backend_iot_sess_destroy(void){

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
        error_print("engine_init_sess_pool() failed: out of memory for session pool allocation!");
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
        error_print("engine_init_sess_pool() failed: out of memory for the shared memory pool allocation!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    ret = init_shared_mem_pool(mem_pool);
    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_sess_pool() failed: initialize the shared memory pool failed!");
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
        error_print("engine_init_sess_pool() failed: out of memory for the shared memory pool lock allocation!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    ret = init_shared_mem_pool_lock(mem_pool_lock);
    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init_sess_pool() failed: initialize the shared memory pool lock failed!");
        free(mem_pool);
        free(mem_pool_lock);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}

int engine_init_shared_mem_queue(BackendEngine *eng){
    struct SharedMemoryPoolQueue    *rx_queue, *tx_queue;
    SharedMemoryPoolQueueConfig     *rx_queue_conf, *tx_queue_conf;

    if(NULL == eng){
        error_print("engine_init_shared_mem_queue() failed: the engine instance is NULL (uninitialized or invalid)!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    rx_queue_conf   = &high_speed_net_rx_queue_config;
    rx_queue        = shared_mem_pool_queue_create_backend(rx_queue_conf);

    if(NULL == rx_queue){
        error_print("engine_init_shared_mem_queue() failed: out of memory for the shared memory RX queue allocation!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    tx_queue_conf   = &high_speed_net_tx_queue_config;
    tx_queue        = shared_mem_pool_queue_create_backend(tx_queue_conf);

    if(NULL == tx_queue){
        error_print("engine_init_shared_mem_queue() failed: out of memory for the shared memory TX queue allocation!");
        free(rx_queue);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    rx_queue_conf->pool = eng->mem_pool;
    tx_queue_conf->pool = eng->mem_pool;
    
    eng->rx_queue       = rx_queue;
    eng->tx_queue       = tx_queue;


    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Initialize the epoll-based poller of the backend engine
 * 
 * This function initializes the epoll instance that belongs to the given BackendEngine. 
 * The poller is responsible for efficiently monitoring I/O events on file descriptors 
 * (e.g., network sockets, eventfd, or other kernel-managed resources) used by the backend engine. 
 * Initialization includes creating an epoll file descriptor via epoll_create1(), setting up 
 * internal event tracking structures, and preparing the poller for subsequent registration 
 * of I/O sources (e.g., listening sockets or HyperAMP notification channels).
 * This component is essential for event-driven I/O handling in the Linux-based backend.
 * 
 * @param eng [in/out] Pointer to a BackendEngine structure. The function initializes the 
 *                     poller-related fields within this structure (e.g., epoll_fd, max_events 
 *                     buffer, and associated metadata).
 * 
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Poller initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Initialization failed (e.g., epoll_create1() failed, 
 *                                        insufficient memory for event buffer, or invalid state)
 * 
 * @note 1. Ensure the eng pointer points to a valid BackendEngine instance before invocation.
 *       2. The poller is intended for use within the Linux environment and relies on standard 
 *          Linux I/O multiplexing facilities; it is not involved in seL4-side operations.
 *       3. After initialization, file descriptors must be explicitly added to the poller 
 *          before their events can be detected during polling.
 */
int engine_init_poller(BackendEngine *eng){
    return poller_init(&eng->poller);
};



/**
 2  * @brief Initialize the HyperAMP queue of the backend engine
 3  * 
 4  * This function initializes the HyperAMP queue within the BackendEngine structure. 
 5  * The HyperAMP queue is a shared communication channel used for message exchange between 
 6  * the frontend engine (running in a seL4 guest OS) and the backend engine (running on Linux). 
 7  * It enables low-latency, reliable inter-environment messaging by providing a pre-allocated, 
 8  * fixed-size buffer with synchronized read/write access semantics. Initialization includes 
 9  * allocating or mapping the shared memory region, setting up queue metadata (e.g., head/tail pointers, 
 10  * capacity, and state flags), and ensuring the queue is ready to receive messages from the frontend.
 11  * 
 12  * @param eng [in/out] Pointer to a BackendEngine structure. The function initializes members 
 13  *                     related to the HyperAMP queue (e.g., shared buffer pointer, queue size, 
 14  *                     producer/consumer indices, and operational status) within this structure.
 15  * 
 16  * @return int Execution result
 17  *         - BACKEND_PROXY_PROCESS_OK: HyperAMP queue initialized successfully
 18  *         - BACKEND_PROXY_PROCESS_ERROR: Initialization failed (e.g., shared memory allocation/mapping 
 19  *                                        failure, invalid configuration, or alignment issues)
 20  * 
 21  * @note 1. Ensure the eng pointer points to a valid BackendEngine instance before calling this function.
 22  *       2. Proper setup of the underlying inter-environment communication infrastructure 
 23  *          (e.g., virtio, shared memory regions, or IPC channels between seL4 and Linux) must be 
 24  *          completed prior to invoking this function.
 25  *       3. The queue layout and memory must be compatible with both the seL4 guest and Linux host 
 26  *          to ensure correct cross-environment visibility and cache coherency.
 27  */
int engine_init_hyperamp_queue(BackendEngine *eng){
    int ret;
/*
 * Initialize the HyperAMP Linux client using the default shared memory address.
 * Since is_creator=1, Linux acts as the creator and initializes both the RX and TX HyperAMP queues.
 * This call also initializes the global HyperampLinuxContext variable g_ctx,
 * which holds the shared memory layout, including pointers to the hyper_rx_queue,
 * hyper_tx_queue, and the underlying data region. The backend engine later retrieves
 * these pointers from g_ctx to establish cross-environment communication with the seL4 frontend.
 */
    ret = hyperamp_linux_init(0, 1);

    if(HYPERAMP_ERROR == ret){
        error_print("engine_init_hyperamp_queue failed: hyperamp_linux_init returned error\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    eng->hyper_rx_queue         = g_ctx.rx_queue;
    eng->hyper_tx_queue         = g_ctx.tx_queue;
    eng->hyper_amp_data_region  = g_ctx.data_region;

    utils_print("In %s, the address of hyper_rx_queue = %p, hyper_tx_queue = %p, hyper_amp_data_region = %p\n", __func__, eng->hyper_rx_queue, eng->hyper_tx_queue, eng->hyper_amp_data_region);

    return BACKEND_PROXY_PROCESS_OK;
}


void engine_init()
{
    int ret;

    p_g_bk_eng = &g_bk_eng;
    memset(p_g_bk_eng, 0, sizeof(BackendEngine));

    ret = engine_init_hs_net_dev(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_hs_net_dev returned error!\n");
        return;
    }

    utils_print("ret of engine_init_hs_net_dev is %d\n", ret);

    ret = engine_init_eng_ops(p_g_bk_eng);
    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_eng_ops returned error!\n");
        return;
    }


    ret = engine_init_selector(p_g_bk_eng);

    ret = engine_init_sess_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_sess_pool returned error!\n");
        return;
    }

    utils_print("engine_init_sess_pool() succeeded!\n");

    ret = engine_init_shared_mem_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_shared_mem_pool returned error!\n");
        return;
    }

    utils_print("engine_init_shared_mem_pool() succeeded!\n");

    ret = engine_init_shared_mem_pool_lock(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_shared_mem_pool_lock returned error!\n");
        return;
    }

    utils_print("engine_init_shared_mem_pool_lock() succeeded!\n");

#if 0
    ret = engine_init_shared_mem_queue(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_shared_mem_queue returned error!\n");
        return;
    }

    utils_print("engine_init_shared_mem_queue() succeeded!\n");
#endif

    ret = engine_init_poller(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_poller returned error!\n");
        return;
    }

    utils_print("engine_init_poller() succeeded!\n");

    ret = engine_init_hyperamp_queue(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_hyperamp_queue returned error!\n");
        return;
    }

    ret = engine_init_iot_devices(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_iot_devices returned error!\n");
        return;
    }

    ret = engine_init_iot_sessions(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_iot_sessions returned error!\n");
        return;
    }
/*
 * 
 */
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
    int             ret;
    size_t          msg_size, buf_size;
    ProxyMsgHeader  *msg_hdr;

    if(NULL == queue || NULL == buf_ptr || NULL == out_len){
            error_print("backend_engine_rx_queue_get failed: invalid input parameters (NULL pointers)");
        return BACKEND_PROXY_PROCESS_ERROR;   
    }

    ret = shared_mem_pool_queue_recv_zc(queue, buf_ptr, &buf_size);

    if(BACKEND_PROXY_PROCESS_ERROR == ret){
        error_print("backend_engine_rx_queue_get failed: failed to retrieve data from RX queue");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(BACKEND_PROXY_PROCESS_AGAIN == ret){
        error_print("backend_engine_rx_queue_get returns: the RX queue is empty");
        return BACKEND_PROXY_PROCESS_AGAIN;
    }

    msg_hdr     = (ProxyMsgHeader *)(*buf_ptr);
    msg_size    = msg_hdr->payload_len;

    if(msg_size + sizeof(ProxyMsgHeader) > buf_max_len){
        error_print("backend_engine_rx_queue_get failed: message total size (header + payload) exceeds buffer maximum length");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    *out_len = msg_size;

    return ret;
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
 * @brief Retrieves a message from the HyperAMP receive queue managed by the BackendEngine.
 *
 * @details This function fetches message data from the HyperAMP RX queue associated with the given
 *          BackendEngine instance, and copies the message data to the provided buffer pointed to by @p data.
 *          It returns an integer status code to indicate the operation result, and outputs the actual 
 *          length of the retrieved message through the @p out_len parameter.
 *          The caller must check the returned status code before using the data in @p data and the value in @p out_len.
 *
 * @param[in]  eng         Pointer to the BackendEngine instance, cannot be NULL
 * @param[in]  max_msg_len Maximum allowed length of the message to read (i.e., the size of the @p data buffer),
 *                         used to prevent buffer overflow
 * @param[out] data        Pointer to a uint8_t buffer that stores the retrieved message data;
 *                         valid only when the return value is BACKEND_PROXY_PROCESS_OK
 * @param[out] out_len     Pointer to a size_t variable that stores the actual length of the retrieved message;
 *                         valid only when the return value is BACKEND_PROXY_PROCESS_OK
 *
 * @return Integer status code indicating the result of the operation:
 *         - BACKEND_PROXY_PROCESS_OK: Operation succeeded
 *         - BACKEND_PROXY_PROCESS_ERROR: System-level error occurred
 *         - BACKEND_PROXY_PROCESS_AGAIN: Message temporarily unavailable
 *
 * @retval BACKEND_PROXY_PROCESS_OK
 *         Message data is retrieved successfully, the @p data buffer contains valid message content
 *         and @p out_len holds the actual message length
 * @retval BACKEND_PROXY_PROCESS_ERROR
 *         A system-level error occurred (e.g., engine is NULL, internal queue not initialized, @p data/@p out_len is NULL),
 *         the @p data buffer and @p out_len are undefined
 * @retval BACKEND_PROXY_PROCESS_AGAIN
 *         Message data is temporarily unavailable (e.g., HyperAMP RX queue is empty),
 *         the @p data buffer and @p out_len are undefined
 *
 * @note The message data copied to @p data is sourced from shared memory (e.g., @c g_ctx.data_region).
 *       The ownership of the @p data buffer is held by the caller (who is responsible for allocating/freeing it),
 *       while the underlying shared memory lifecycle follows the HyperAMP queue protocol.
 *       Refer to HyperAMP documentation for detailed memory and lifecycle management rules.
 */
int backend_engine_hyperamp_rx_queue_get(BackendEngine *eng, size_t max_msg_len, 
                                        uint8_t *data, size_t *out_len){
    int ret;

    if(NULL == eng){
        error_print("backend_engine_hyperamp_rx_queue_get failed: eng is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == eng->hyper_rx_queue){
        error_print("backend_engine_hyperamp_rx_queue_get failed: eng->hyper_rx_queue is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == eng->hyper_amp_data_region){
        error_print("backend_engine_hyperamp_rx_queue_get failed: eng->hyper_amp_data_region is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    if(NULL == data){
        error_print("backend_engine_hyperamp_rx_queue_get failed: data is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == out_len){
        error_print("backend_engine_hyperamp_rx_queue_get failed: out_len is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    

    utils_print("In %s, the address of hyper_rx_queue is %p\n", __func__, eng->hyper_rx_queue);
    ret = hyperamp_queue_dequeue(eng->hyper_rx_queue, HYPERAMP_ZONE_ID_Linux, data, max_msg_len, out_len, eng->hyper_amp_data_region);

    if(HYPERAMP_ERROR == ret){
        error_print("backend_engine_hyperamp_rx_queue_get failed: hyperamp_queue_dequeue execution failed!\n");
        return BACKEND_PROXY_PROCESS_ERROR;  
    }else if(HYPERAMP_AGAIN == ret){
        error_print("backend_engine_hyperamp_rx_queue_get failed: queue is empty!\n");
        return BACKEND_PROXY_PROCESS_AGAIN;
    }else{
/* 
 * hyperamp_queue_dequeue execution succeeded, no action required. 
 */
    }


    return BACKEND_PROXY_PROCESS_OK;
}



/**
 * @brief Sends a message to the HyperAMP transmit queue managed by the BackendEngine.
 *
 * @details This function pushes message data from the provided buffer pointed to by @p data
 *          into the HyperAMP TX queue associated with the given BackendEngine instance.
 *          It returns an integer status code to indicate the operation result.
 *          The caller must check the returned status code to determine if the message was 
 *          successfully queued for transmission.
 *
 * @param[in] eng        Pointer to the BackendEngine instance, cannot be NULL
 * @param[in] data       Pointer to a uint8_t buffer containing the message data to be sent;
 *                       cannot be NULL
 * @param[in] msg_len    Length of the message data in bytes to be sent;
 *                       must be greater than 0 and within the protocol's maximum limit
 *
 * @return Integer status code indicating the result of the operation:
 *         - BACKEND_PROXY_PROCESS_OK: Operation succeeded (message queued)
 *         - BACKEND_PROXY_PROCESS_ERROR: System-level error occurred
 *         - BACKEND_PROXY_PROCESS_AGAIN: Queue temporarily full or resource unavailable
 *
 * @retval BACKEND_PROXY_PROCESS_OK
 *         Message data is successfully copied to the TX queue and scheduled for transmission.
 * @retval BACKEND_PROXY_PROCESS_ERROR
 *         A system-level error occurred (e.g., engine is NULL, internal queue not initialized, 
 *         @p data is NULL, @p msg_len is invalid), the message is NOT sent.
 * @retval BACKEND_PROXY_PROCESS_AGAIN
 *         The TX queue is currently full or back-pressure is applied (e.g., remote peer not ready),
 *         the caller should retry later. The message is NOT sent.
 *
 * @note The message data from @p data is copied into shared memory (e.g., @c g_ctx.data_region) 
 *       managed by the HyperAMP queue protocol. The caller retains ownership of the @p data buffer 
 *       and is responsible for its lifecycle, but the buffer content must remain valid until this 
 *       function returns.
 *       Refer to HyperAMP documentation for detailed memory and lifecycle management rules.
 */
int backend_engine_hyperamp_tx_queue_put(BackendEngine *eng, const uint8_t *data, size_t msg_len){
    int ret;

    if(NULL == eng){
        error_print("backend_engine_hyperamp_tx_queue_put failed: eng is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == eng->hyper_tx_queue){
        error_print("backend_engine_hyperamp_tx_queue_put failed:  eng->hyper_tx_queue is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(msg_len > eng->hyper_tx_queue->block_size){
        error_print("backend_engine_hyperamp_tx_queue_put failed: message length exceeds queue block size limit!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == eng->hyper_amp_data_region){
        error_print("backend_engine_hyperamp_tx_queue_put failed: eng->hyper_amp_data_region is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    if(NULL == data){
        error_print("backend_engine_hyperamp_tx_queue_put failed: data is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    

    utils_print("In %s, the address of hyper_tx_queue is %p\n", __func__, eng->hyper_tx_queue);


    ret = hyperamp_queue_enqueue(eng->hyper_tx_queue, HYPERAMP_ZONE_ID_Linux, data, msg_len, eng->hyper_amp_data_region);

    if(HYPERAMP_ERROR == ret){
        error_print("frontend_engine_hyperamp_tx_queue_put failed: hyperamp_queue_enqueue execution failed!\n");
        return BACKEND_PROXY_PROCESS_ERROR;  
    }else if(HYPERAMP_AGAIN == ret){
        error_print("frontend_engine_hyperamp_tx_queue_put failed: queue is empty!\n");
        return BACKEND_PROXY_PROCESS_AGAIN;
    }else{
/* 
 * hyperamp_queue_enqueue execution succeeded, no action required. 
 */
    }
    
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Runs the listener loop for backend engine to handle TCP passive connection establishment
 * 
 * @details This function is the core entry point for processing TCP passive connections in the backend engine.
 *          It executes the following key steps in sequence:
 *          1. Iterates through all struct HighSpeedNetDevice instances managed by the input BackendEngine_ pointer (eng)
 *          2. For each device, checks if the TCP listener socket (tcp_listener) has been successfully initialized
 *          3. If the listener socket is ready, repeatedly calls the accept() system call to handle passive connection establishment:
 *              - Creates a new socket file descriptor to correspond with the newly established connection
 *              - Allocates and initializes a new struct BackendSession instance, and associates it with the new socket
 *              - Constructs a "new session" message and sends it to the proxy frontend for further processing
 * 
 * @param eng Pointer to the struct BackendEngine_ instance that manages all high-speed network devices
 *            - If eng is NULL, the function will exit early without any connection processing (defensive check recommended in implementation)
 *            - eng must contain valid struct HighSpeedNetDevice instances with initialized tcp_listener to process connections
 * 
 * @note The function only processes TCP passive connections (via accept())—UDP connections are not handled here
 * @note The new struct BackendSession instance is tightly associated with the new connection socket to track session state
 * @warning The function assumes tcp_listener has been properly bound and listened (e.g., via bind()/listen()) before invocation;
 *          uninitialized tcp_listener will be skipped without error reporting
 * @warning Ensure the proxy frontend is ready to receive the "new session" message to avoid message loss or processing failures
 */
void engine_listener_run(struct BackendEngine_ *eng){
    struct BackendSessionPool       *sess_pool;
    struct BackendSessionPoolOps    *ops;
    struct BackendSession           *sess;
    struct HighSpeedNetDeviceSet    *dev_set;
    struct HighSpeedNetDevice       *hs_dev;
    struct sockaddr_in              client_addr;
    PassiveSessParaIP               passive_sess_para;
    socklen_t                       client_addr_len = sizeof(client_addr);
    int                             dev_num, dev_cnt, sock_fd, ret;

    if(NULL == eng || NULL == eng->dev_set || 0 == eng->dev_num || NULL == eng->sess_pool || NULL == eng->sess_pool->ops){

        if (NULL == eng) {
            error_print("engine_listener_run failed: BackendEngine pointer (eng) is NULL\n");
        } else if (NULL == eng->dev_set) {
            error_print("engine_listener_run failed: eng->dev_set (HighSpeedNetDeviceSet) is NULL\n");
        } else if (0 == eng->dev_num) {
            error_print("engine_listener_run: eng->dev_num is 0 - no high-speed network devices available for listener processing\n");
        }else if (NULL == eng->sess_pool) {
            error_print("engine_listener_run failed: eng->sess_pool (BackendSessionPool) is NULL\n");
        } else if (NULL == eng->sess_pool->ops) {
            error_print("engine_listener_run failed: eng->sess_pool->ops (BackendSessionPoolOps) is NULL\n");
        }
        return;
    }

    dev_set     = eng->dev_set;
    dev_num     = eng->dev_num;
    sess_pool   = eng->sess_pool;
    ops         = sess_pool->ops;
    dev_cnt     = 0;

/*
 * STEP 1. This loop iterates through all instances of struct HighSpeedNetDevice that are managed by the input BackendEngine_ pointer.
 */
    for(dev_cnt = 0; dev_cnt < dev_num; dev_cnt++){
        hs_dev = &dev_set->hs_net_dev[dev_cnt];

        if(-1 == hs_dev->tcp_listener)
            continue;

        client_addr_len = sizeof(client_addr);
/*
 * STEP 2. Call accept() to retrieves the first pending connection from the listen queue of the specified TCP listener, and creates a new connected socket (sock_fd) for communicating 
 * with the client.
 * Even if multiple connections are waiting in the accept queue, engine_listener_run accepts only one connection from the TCP listener at a time to ensure fairness among all devices.
 */
        sock_fd = accept(hs_dev->tcp_listener, (struct sockaddr *)&client_addr, &client_addr_len);

        if (-1 == sock_fd){
    /*
     * Handle fatal errors that are not related to non-blocking mode.
     * EAGAIN/EWOULDBLOCK: No pending connections in the accept queue (normal for non-blocking sockets), no need to terminate the program.
     * Other errno values: Real fatal errors (e.g., invalid socket, insufficient permissions, resource exhaustion), print error message 
     *                     and terminate the program abnormally.
     */
            if (errno != EAGAIN && errno != EWOULDBLOCK){
                error_print("engine_listener_run failed: accept() call failed!\n");
                exit(EXIT_FAILURE);
            }
    /*
     * If errno is EAGAIN or EWOULDBLOCK: No pending connections available for the current device's TCP listener.
     * This is a normal scenario in non-blocking socket mode, so we simply skip to the next device in the loop
     * without any error handling or program termination.
     */
            continue;
        }
/*
 * STEP 3. Construct a backend session instance that is associated with the passive TCP connection. 
 *         Then construct a session-creation message to notify the frontend proxy to complete the 
 *         session creation procedure.
 *
 * Currently, the backend proxy protocol only supports the IPv4 passive connection establishment procedure.
 * Support for IPv6 is planned for future implementation.
 */
        memset(&passive_sess_para, 0, sizeof(PassiveSessParaIP));
        passive_sess_para.fd                                    = sock_fd;
        passive_sess_para.dev_id                                = hs_dev->dev_id;
        passive_sess_para.ip_version                            = SESS_IPV4_PROTO;
        passive_sess_para.trans_proto                           = SESS_TCP_PROTO;
        passive_sess_para.ip_port_tuple.ipv4_port_tuple.port    = ntohs(client_addr.sin_port);
        COPY_IN_TO_IPV4(&passive_sess_para.ip_port_tuple.ipv4_port_tuple.ipv4_addr, &client_addr.sin_addr);

        ret = ops->create_sess_passive(sess_pool, &sess, &passive_sess_para);


/*
 * Process abnormal scenarios. Close the connected socket if a session-creation request for the frontend proxy cannot be created successfully.
 */
        if(BACKEND_PROXY_PROCESS_OK != ret){
            error_print("engine_listener_run failed: failed to create a new session instance associated with the connection!\n");
            close(sock_fd);
            continue;
        }

        set_nonblocking(sock_fd);
    }// for(dev_cnt = 0; dev_cnt < dev_num; dev_cnt++)


}


/**
 * @brief Core execution loop for Bluetooth L2CAP session data ingestion and command forwarding.
 * 
 * This function drives the southbound data pipeline for the **Bluetooth** protocol within the BackendEngine.
 * It accesses the specific `IoTBackendSession` instance configured for Bluetooth (SOCK_SEQPACKET/L2CAP),
 * extracts raw L2CAP payloads, converts them into standardized backend proxy messages, and enqueues
 * them into the HyperAMP shared memory TX queue.
 * 
 * @param sess Pointer to the active IoTBackendSession instance for Bluetooth, containing the listening 
 *             or connected socket descriptors and client list.
 * 
 * @details The function executes a cyclic process focused on "Bluetooth-Device-to-Proxy" data flow:
 * 
 * 1. **Connection Management & Data Extraction**:
 *    - **Server Mode**: Iterates the session's client list (`sock_list`) using `accept()` (non-blocking) 
 *      to handle new incoming L2CAP connections on the configured PSM. Manages the linked list of 
 *      connected clients (`IotSockNode`).
 *    - **Client Mode**: Monitors the single active L2CAP connection for incoming packets.
 *    - Extracts raw payloads from L2CAP channels (SOCK_SEQPACKET), preserving message boundaries.
 *      - Handles BLE GATT notifications/indications if mapped to L2CAP.
 *      - Reads classic Bluetooth L2CAP data packets.
 * 
 * 2. **Proxy Protocol Message Conversion**:
 *    - Transforms raw L2CAP data into the unified **Backend Proxy Protocol** format.
 *    - Encapsulates payload with:
 *      - Session ID / Bluetooth MAC Address (BD_ADDR).
 *      - Protocol Type Indicator: `PROTOCOL_BLUETOOTH_L2CAP`.
 *      - PSM (Protocol/Service Multiplexer) value and Channel ID (CID) if available.
 *      - Timestamp and QoS flags.
 * 
 * 3. **Direct Enqueueing to HyperAMP TX Queue**:
 *    - Constructs the proxy message buffer and **directly inserts** it into the global 
 *      **HyperAMP Shared Memory TX Queue**.
 *    - Bypasses intermediate per-session B2F queues for low-to-moderate latency requirements.
 * 
 * 4. **Downlink Command Execution (Frontend-to-Bluetooth)**:
 *    - Checks the HyperAMP RX Queue for control commands targeted at this Bluetooth session.
 *    - Decapsulates commands and writes raw data directly to the corresponding socket:
 *      - Sends L2CAP packets to specific connected clients (Server mode) or the remote device (Client mode).
 *      - Handles disconnection logic if `send()` fails (e.g., remote device out of range).
 * 
 * 5. **Session State Maintenance**:
 *    - Monitors L2CAP link health (socket readability/writeability).
 *    - Cleans up disconnected clients from the `sock_list` using `TAILQ_FOREACH_SAFE`.
 *    - Generates session-close proxy messages upon disconnection events.
 * 
 * @note Uses `SOCK_SEQPACKET` to maintain L2CAP packet boundaries inherently.
 * @note Supports both Server (listening on PSM) and Client (connecting to BD_ADDR) modes.
 * @warning Ensure the session socket is set to non-blocking mode to prevent stalling the engine loop.
 * 
 * @see engine_iot_dev_run()
 * @see TAILQ_FOREACH_SAFE
 */
void engine_iot_bluetooth_run(IoTBackendSession *sess){
    utils_print("In %s\n", __func__);
    int                     ret;
    uint8_t                 data[1024];
    IotMsgBuffer            msg_buf;
    GeneralProxyMsgHeader   proxy_msg_hdr;
    uint8_t                 res_buf[100] = {0};
    uint8_t                 *res_ptr;




    do{
        memset(&msg_buf, 0, sizeof(IotMsgBuffer));
        memset(data, 0, sizeof(data));
        msg_buf.data = data;
        msg_buf.len  = sizeof(data);

        ret = sess->recv_from_remote(sess, &msg_buf, 0);

        if(BACKEND_PROXY_PROCESS_AGAIN == ret){
            error_print("engine_iot_bluetooth_run returned: no bluetooth data available yet!\n");
            return;
        }else if(BACKEND_PROXY_PROCESS_ERROR){
            error_print("engine_iot_bluetooth_run failed: receive bluetooth data error!\n");
        }else{
            memset(&proxy_msg_hdr, 0, sizeof(GeneralProxyMsgHeader));
            proxy_msg_hdr.outer_header.frontend_sess_id = 0;
            proxy_msg_hdr.outer_header.backend_sess_id = 0;
            proxy_msg_hdr.outer_header.proxy_msg_type = PROXY_MSG_TYPE_IOT;

            proxy_msg_hdr.inner_header.iot_hdr.dev_port_id  = 0;
            proxy_msg_hdr.inner_header.iot_hdr.opcode       = 0;
            proxy_msg_hdr.inner_header.iot_hdr.proto_type   = IOT_PROTO_TYPE_BLUETOOTH;
            proxy_msg_hdr.inner_header.iot_hdr.proto_ver    = 1;
            proxy_msg_hdr.inner_header.iot_hdr.payload_len  = msg_buf.len + get_iot_addr_length(IOT_PROTO_TYPE_BLUETOOTH);

            proxy_msg_hdr.iot_addr_len                      = get_iot_addr_length(IOT_PROTO_TYPE_BLUETOOTH);
            proxy_msg_hdr.iot_addr.addr_type                = IOT_PROTO_TYPE_BLUETOOTH;
            proxy_msg_hdr.iot_addr.addr_info.bt_addr.port   = msg_buf.addr.addr_info.bt_addr.port;
            memcpy(proxy_msg_hdr.iot_addr.addr_info.bt_addr.mac, msg_buf.addr.addr_info.bt_addr.mac, sizeof(msg_buf.addr.addr_info.bt_addr.mac));

            res_ptr = res_buf;
            build_proxy_general_message(sess->eng, &proxy_msg_hdr, msg_buf.data, msg_buf.len, &res_ptr, MEMORY_ALLOC_AMPQUEUE, NULL);
            //msg_buf.addr;
        }

    }while(BACKEND_PROXY_PROCESS_OK == ret);

}



/**
 * @brief Core execution loop for CAN bus session data ingestion and frame forwarding.
 * 
 * This function drives the southbound data pipeline for the **CAN/CAN-FD** protocol. It accesses
 * the `IoTBackendSession` bound to a SocketCAN interface (e.g., `can0`), reads raw CAN frames,
 * converts them into standardized proxy messages, and enqueues them into the HyperAMP TX queue.
 * 
 * @param sess Pointer to the active IoTBackendSession instance for CAN, holding the PF_CAN socket.
 * 
 * @details The function executes a cyclic process for "CAN-Bus-to-Proxy" data flow:
 * 
 * 1. **Frame Extraction**:
 *    - Reads raw `struct can_frame` or `struct canfd_frame` from the SocketCAN socket.
 *    - Extracts critical fields: Arbitration ID (Standard/Extended), DLC (Data Length Code), 
 *      Data payload, and Flags (RTR, EDF for FD frames).
 * 
 * 2. **Proxy Protocol Message Conversion**:
 *    - Encapsulates CAN data into the **Backend Proxy Protocol**.
 *    - Headers include:
 *      - Interface Name (e.g., "can0").
 *      - Protocol Type: `PROTOCOL_CAN` or `PROTOCOL_CANFD`.
 *      - CAN ID (29-bit or 11-bit), DLC, and Error State indicators.
 * 
 * 3. **Direct Enqueueing**:
 *    - Inserts the formatted message directly into the **HyperAMP Shared Memory TX Queue**.
 * 
 * 4. **Downlink Command Execution**:
 *    - Retrieves commands from the HyperAMP RX Queue.
 *    - Constructs `can_frame` structures from the payload and writes them to the SocketCAN interface.
 *    - Supports sending Standard and Extended ID frames, and Remote Transmission Requests (RTR).
 * 
 * 5. **Interface Monitoring**:
 *    - Detects BUS-OFF states or interface down events.
 *    - Handles reconnection if the CAN interface is restarted.
 * 
 * @note Requires Linux Kernel support for SocketCAN (PF_CAN).
 * @note Supports both Classic CAN (8 bytes) and CAN-FD (up to 64 bytes) depending on socket configuration.
 * 
 * @see engine_iot_dev_run()
 */
void engine_iot_can_run(IoTBackendSession *sess){
    
}



/**
 * @brief Core execution loop for Modbus TCP session data ingestion and PDU forwarding.
 * 
 * This function manages the southbound data pipeline for **Modbus TCP**. It accesses the
 * `IoTBackendSession` handling established TCP streams, extracts Modbus PDUs (encapsulated in MBAP headers),
 * converts them to proxy messages, and forwards them via HyperAMP.
 * 
 * @param sess Pointer to the active IoTBackendSession instance for Modbus TCP.
 * 
 * @details The function executes a cyclic process for "Modbus-TCP-to-Proxy" data flow:
 * 
 * 1. **PDU Extraction**:
 *    - Reads TCP streams from connected clients (Server) or the PLC server (Client).
 *    - Parses the **MBAP Header** (Transaction ID, Protocol ID, Length, Unit ID).
 *    - Extracts the raw Modbus PDU (Function Code + Data).
 *    - Validates Transaction IDs to ensure request/response matching if in stateful mode.
 * 
 * 2. **Proxy Protocol Message Conversion**:
 *    - Encapsulates data into the **Backend Proxy Protocol**.
 *    - Headers include:
 *      - Protocol Type: `PROTOCOL_MODBUS_TCP`.
 *      - MBAP Metadata (Transaction ID, Unit ID).
 *      - Function Code (e.g., 03 Read Holding Registers, 06 Write Single Register).
 *      - Register addresses and values (parsed optionally for inspection, or passed as raw binary).
 * 
 * 3. **Direct Enqueueing**:
 *    - Enqueues the message into the **HyperAMP Shared Memory TX Queue**.
 * 
 * 4. **Downlink Command Execution**:
 *    - Receives write requests from the Frontend.
 *    - Constructs valid Modbus TCP packets (MBAP + PDU) and sends them over the TCP socket.
 *    - Handles timeout and retransmission logic if required by the session policy.
 * 
 * 5. **Connection Health**:
 *    - Monitors TCP connection status (keep-alive, EOF).
 *    - Cleans up closed connections and notifies the frontend via proxy messages.
 * 
 * @note Operates at the TCP level; does not implement a full Modbus state machine unless configured.
 * @note Ensures atomic reading of MBAP headers to prevent frame tearing.
 * 
 * @see engine_iot_dev_run()
 */
void engine_iot_modbustcp_run(IoTBackendSession *sess){}



/**
 * @brief Core execution loop for ZigBee session data ingestion and cluster command forwarding.
 * 
 * This function drives the data pipeline for **ZigBee** (typically via a serial gateway or USB dongle).
 * It accesses the `IoTBackendSession`, parses incoming ZigBee frames (APS/ZCL layers), converts them
 * to proxy messages, and enqueues them into HyperAMP.
 * 
 * @param sess Pointer to the active IoTBackendSession instance for ZigBee.
 * 
 * @details The function executes a cyclic process for "ZigBee-Device-to-Proxy" data flow:
 * 
 * 1. **Frame Extraction**:
 *    - Reads raw bytes from the ZigBee coordinator interface (UART/USB).
 *    - Decapsulates the transport layer (e.g., TI Z-Stack, Ember ASH/EZSP, or XBee API frames).
 *    - Extracts **APSDE-DATA** payloads containing Cluster Commands and Attribute Reports.
 * 
 * 2. **Proxy Protocol Message Conversion**:
 *    - Transforms ZigBee data into the **Backend Proxy Protocol**.
 *    - Headers include:
 *      - Protocol Type: `PROTOCOL_ZIGBEE`.
 *      - Source/Destination IEEE Addresses (64-bit) and Network Addresses (16-bit).
 *      - Endpoint, Cluster ID, and Profile ID.
 *      - ZCL Command ID and Payload.
 * 
 * 3. **Direct Enqueueing**:
 *    - Inserts the message into the **HyperAMP Shared Memory TX Queue**.
 * 
 * 4. **Downlink Command Execution**:
 *    - Receives control commands (e.g., "Turn On Light", "Set Temperature") from the Frontend.
 *    - Encapsulates them into appropriate ZCL commands and transmits via the coordinator.
 * 
 * 5. **Network State Maintenance**:
 *    - Monitors coordinator status (network formed, device joined/left events).
 *    - Handles serial port reconnection errors.
 * 
 * @note Requires specific parsing logic based on the underlying ZigBee stack (e.g., Z-Stack, EZSP).
 * @note Focuses on application-level cluster data rather than raw MAC layer frames.
 * 
 * @see engine_iot_dev_run()
 */
void engine_iot_zigbee_run(IoTBackendSession *sess){}



/**
 * @brief Core execution loop for LoRaWAN session data ingestion and uplink payload forwarding.
 * 
 * This function manages the southbound pipeline for **LoRaWAN**. It accesses the `IoTBackendSession`
 * connected to a LoRa Gateway bridge (e.g., UDP forwarder or MQTT backend), extracts decrypted 
 * application payloads, converts them to proxy messages, and enqueues them into HyperAMP.
 * 
 * @param sess Pointer to the active IoTBackendSession instance for LoRaWAN.
 * 
 * @details The function executes a cyclic process for "LoRaWAN-Uplink-to-Proxy" data flow:
 * 
 * 1. **Payload Extraction**:
 *    - Receives uplink notifications from the Network Server (NS) or Gateway Bridge.
 *    - Extracts the decrypted **Application Payload** (FPort data).
 *    - Captures metadata: DevEUI, AppEUI, FCnt (Frame Counter), RSSI, SNR, and DR (Data Rate).
 * 
 * 2. **Proxy Protocol Message Conversion**:
 *    - Encapsulates data into the **Backend Proxy Protocol**.
 *    - Headers include:
 *      - Protocol Type: `PROTOCOL_LORAWAN`.
 *      - Device Identifiers (DevEUI).
 *      - Port Number (FPort).
 *      - RF Metadata (RSSI, SNR, Gateway ID).
 * 
 * 3. **Direct Enqueueing**:
 *    - Inserts the message into the **HyperAMP Shared Memory TX Queue**.
 * 
 * 4. **Downlink Command Execution**:
 *    - Receives downlink commands from the Frontend.
 *    - Schedules downlink messages (confirmed/unconfirmed) to be sent via the Network Server.
 *    - Handles timing constraints (RX1/RX2 windows) if managed locally.
 * 
 * 5. **Session Monitoring**:
 *    - Monitors connectivity to the LoRa Gateway Bridge.
 *    - Tracks device activation status (OTAA/ABP).
 * 
 * @note Assumes decryption is handled upstream (by NS) or keys are available in the session context.
 * @note Optimized for low-frequency, small-payload uplinks typical of LoRaWAN.
 * 
 * @see engine_iot_dev_run()
 */
void engine_iot_lora_run(IoTBackendSession *sess){}



/**
 * @brief Core execution loop for Ethernet POWERLINK session data ingestion and PDO/SDO forwarding.
 * 
 * This function drives the high-performance data pipeline for **Ethernet POWERLINK**. It accesses
 * the `IoTBackendSession` managing the POWERLINK stack (MN or CN mode), extracts real-time 
 * Process Data Objects (PDOs) and Service Data Objects (SDOs), and forwards them via HyperAMP.
 * 
 * @param sess Pointer to the active IoTBackendSession instance for POWERLINK.
 * 
 * @details The function executes a cyclic process for "POWERLINK-Cycle-to-Proxy" data flow:
 * 
 * 1. **Real-Time Data Extraction**:
 *    - Intercepts or reads data from the POWERLINK cycle buffer.
 *    - Extracts **PDOs** (Process Data Objects) mapped to specific Object Dictionary entries.
 *    - Captures **SDO** (Service Data Objects) responses for configuration and diagnostics.
 *    - Synchronizes with the SoC (Start of Cycle) event for deterministic timing.
 * 
 * 2. **Proxy Protocol Message Conversion**:
 *    - Encapsulates industrial data into the **Backend Proxy Protocol**.
 *    - Headers include:
 *      - Protocol Type: `PROTOCOL_POWERLINK`.
 *      - Node ID (MN or CN identifier).
 *      - Object Index/Sub-index references.
 *      - Cycle Counter and Phase timestamp.
 * 
 * 3. **Direct Enqueueing**:
 *    - Inserts time-critical messages into the **HyperAMP Shared Memory TX Queue**.
 *    - Prioritizes PDO updates to minimize jitter for frontend visualization/control.
 * 
 * 4. **Downlink Command Execution**:
 *    - Receives setpoint changes or configuration commands from the Frontend.
 *    - Writes to PDO mapping areas for immediate effect in the next cycle.
 *    - Initiates SDO transfers for parameter configuration.
 * 
 * 5. **State & Error Handling**:
 *    - Monitors POWERLINK network state (Pre-Operational, Operational).
 *    - Detects node guard failures or communication errors.
 * 
 * @note Requires precise timing and potentially real-time kernel patches (PREEMPT_RT) for MN operation.
 * @note Handles strict cycle synchronization requirements inherent to POWERLINK.
 * 
 * @see engine_iot_dev_run()
 */
void engine_iot_powerlink_run(IoTBackendSession *sess){}

/**
 * @brief Core execution loop for IoT device data ingestion and direct proxy message forwarding.
 * 
 * This function drives the southbound data pipeline of the BackendEngine. It directly accesses
 * specific IoTBackendSession instances for heterogeneous protocols, extracts raw payloads,
 * converts them into standardized backend proxy protocol messages, and immediately enqueues
 * them into the HyperAMP shared memory TX queue for frontend transmission.
 * 
 * @param eng Pointer to the BackendEngine instance, which holds pointers to the active session
 *            objects for each supported protocol.
 * 
 * @details The function executes a cyclic process focused on direct "Device-to-Proxy-to-Frontend" data flow:
 * 
 * 1. **Direct Session Access & Data Extraction**:
 *    - Iterates through and accesses specific IoTBackendSession instances managed by the engine:
 *      - **backend_bluetooth_sess**: Extracts BLE GATT notifications/indications or L2CAP packet payloads.
 *      - **backend_can_sess**: Reads raw CAN frames (ID + DLC + Data) from socketcan interfaces.
 *      - **backend_zigbee_sess**: Captures ZigBee cluster commands and attribute reports from endpoints.
 *      - **backend_lora_sess**: Retrieves decrypted LoRaWAN application payloads (FPort data) from uplinks.
 *      - **backend_powerlink_sess**: Fetches real-time Process Data Objects (PDO) and Service Data Objects (SDO) 
 *        from Ethernet POWERLINK nodes, handling cycle synchronization if required.
 *      - **backend_modbustcp_sess**: Reads Modbus TCP Protocol Data Units (PDUs) from established TCP streams, 
 *        extracting Function Codes (e.g., 03 Read Holding Registers, 04 Read Input Registers) and associated 
 *        register data values while validating transaction IDs.
 *    - Note: These sessions are designed for low-to-moderate data rate scenarios; high-frequency 
 *      streaming is handled via different mechanisms.
 * 
 * 2. **Proxy Protocol Message Conversion**:
 *    - Transforms the extracted raw protocol-specific data into a unified **Backend Proxy Protocol** format.
 *    - Encapsulates the payload with standard headers including:
 *      - Session ID / Device Identifier (derived from the specific session object).
 *      - Protocol Type Indicator (CAN, BLE, ZigBee, LoRa, POWERLINK, ModbusTCP).
 *      - Timestamp and QoS flags.
 *      - Payload length and binary data.
 *    - Performs minimal processing: The goal is transparent transmission of the device data 
 *      in a standardized envelope without complex business logic (e.g., no register mapping).
 * 
 * 3. **Direct Enqueueing to HyperAMP TX Queue**:
 *    - Constructs the final proxy message buffer.
 *    - **Directly inserts** the message into the global **HyperAMP Shared Memory TX Queue**.
 *    - Bypasses intermediate per-session Backend-to-Frontend (B2F) queues, as these sessions 
 *      operate at lower speeds where direct queue insertion is efficient and sufficient.
 *    - Ensures the Frontend Proxy can immediately retrieve these messages from shared memory.
 * 
 * 4. **Downlink Command Execution (Frontend-to-Device)**:
 *    - Checks the HyperAMP Shared Memory RX Queue (or associated command channel) for incoming 
 *      control commands from the frontend targeted at these specific sessions.
 *    - Decapsulates the proxy message to retrieve the raw command payload.
 *    - Writes the raw data directly to the corresponding session's socket/channel:
 *      - e.g., sending a CAN frame, writing a BLE characteristic, transmitting a POWERLINK SDO write,
 *        or sending a Modbus TCP Write Request (Function Code 06/16) via **backend_modbustcp_sess**.
 * 
 * 5. **Session State Maintenance**:
 *    - Monitors connection health for each specific session object (link status, TCP connectivity, timeouts).
 *    - Handles disconnection events by cleaning up resources and generating a session-close 
 *      proxy message directly to the HyperAMP TX queue.
 *    - Manages reconnection logic based on configured policies for each protocol type.
 * 
 * @note This function acts as a **direct protocol adapter**. It bypasses intermediate buffering 
 *       (B2F queues) for efficiency in low-speed IoT scenarios, writing directly to the shared memory TX queue.
 * @note The supported sessions include specialized industrial protocols like **Ethernet POWERLINK** and 
 *       **Modbus TCP**, requiring precise handling of their respective frame structures.
 * @warning Ensure that all specific session pointers (e.g., backend_modbustcp_sess, backend_powerlink_sess) 
 *          are initialized and valid before calling this function to avoid segmentation faults.
 * 
 * @see engine_run_hyperamp()
 * @see IoTBackendSession
 */
void engine_iot_dev_run(struct BackendEngine_ *eng){
/*
 * IoTBackendSession *backend_bluetooth_sess;
 * IoTBackendSession *backend_can_sess;
 * IoTBackendSession *backend_zigbee_sess;
 * IoTBackendSession *backend_lora_sess;
 * IoTBackendSession *backend_powerlink_sess;
 * IoTBackendSession *backend_modbustcp_sess;
 */
    utils_print("In %s\n", __func__);
    if(NULL != backend_bluetooth_sess)
        engine_iot_bluetooth_run(backend_bluetooth_sess);
    
    if(NULL != backend_can_sess){
        engine_iot_can_run(backend_can_sess);
    }

    if(NULL != backend_modbustcp_sess){
        engine_iot_modbustcp_run(backend_modbustcp_sess);
    }

}


/**
 * @brief Main loop function of the engine, handling message processing and data transmission cyclically
 * 
 * This function executes a continuous loop consisting of four main steps. After completing all steps,
 * it returns to the first step to implement the core operation of the backend engine.
 * 
 * @details The loop process is as follows:
 * 
 * 1. Read data from the HyperAMP RX queue owned by the BackendEngine instance, and process them sequentially 
 * through the backend proxy protocol stack.
 *    
 *    If any data is read, there are two cases:
 *    (a) For device messages, strategy messages, and session messages:
 *        The backend proxy protocol stack performs corresponding processing for each type, constructs
 *        response packets, and returns them to the frontend proxy through the HyperAMP TX queue.
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
 * 5. Retrieve and process data received from remote IoT devices across heterogeneous protocol sessions:
 *    Iterate through active sessions representing diverse connectivity technologies, including:
 *    - **CAN Bus**: Raw frame extraction and ID-based filtering.
 *    - **Bluetooth/BLE**: GATT characteristic notifications or L2CAP payload parsing.
 *    - **ZigBee**: Cluster-specific attribute decoding and endpoint routing.
 *    - **LoRa**: LoRaWAN payload decryption and port-based demultiplexing.
 *    - **Modbus TCP**: Function code validation, register mapping, and PDU parsing.
 *    
 *    For each session, fetch the raw payload from the underlying transport layer, perform protocol-specific 
 *    parsing and business logic processing (e.g., unit conversion, threshold checking, state machine updates). 
 *    Once processed, construct standardized response packets or update internal device states. Finally, enqueue 
 *    the results into the HyperAMP TX queue for transmission to the frontend, or trigger downstream actions 
 *    in subsequent steps. This step ensures unified handling of multi-protocol inbound data from the physical world.
 * 
 * After completing the above four steps, the function returns to step (1) to continue the loop.
 */
void engine_run_hyperamp(){
    BackendEngine                   *eng;
    volatile HyperampShmQueue       *hyper_rx_queue, *hyper_tx_queue;
    struct BackendSessionQueue      *active_queue_f2b, *active_queue_b2f;
    struct BackendSession           *cur_sess, *next_sess;
    struct BackendSessionPool       *sess_pool;
    struct BackendSessionPoolOps    *sess_pool_ops;
    NetPoller                       *net_poller;
//    uint8_t                         *proxy_msg;
//    uint32_t                        msg_size;
    size_t                          block_size;
    int                             ret;
    uint8_t                         msg_buf[HYPERAMP_MSG_HDR_PLUS_MAX_SIZE];

    eng = get_global_backend_engine();

    if(NULL == eng){
        error_print("engine_run failed: the global backend engine is not initialized!");
        return ;
    }



/* 
 * hyper_rx_queue: HyperAMP receive queue instance for cross-OS shared memory communication.
 * This local receive queue maps to the front-end's HyperAMP transmit queue (hyper_tx_queue).
 * Data sent by the front-end through its hyper_tx_queue is received locally via this hyper_rx_queue.
 * 
 * hyper_tx_queue: HyperAMP transmit queue instance for cross-OS shared memory communication.
 * This local transmit queue serves as the front-end's HyperAMP receive queue (hyper_rx_queue).
 * Data sent locally through this hyper_tx_queue is received by the front-end via its hyper_rx_queue.
 * 
 * hyper_amp_data_region: The memory region where cross-OS shared memory data is stored,
 * which is the underlying storage for data transmitted via HyperAMP queues.
 */
    if(NULL == eng->hyper_rx_queue || NULL == eng->hyper_tx_queue || NULL == eng->hyper_amp_data_region){
        error_print("engine_run_hyperamp failed: The global backend engine's HyperAMP RX queue, HyperAMP TX queue or HyperAMP shared memory data region has not been initialized!");
        return ;
    }

    hyper_rx_queue = eng->hyper_rx_queue;
    hyper_tx_queue = eng->hyper_tx_queue;
    (void)hyper_rx_queue;
    (void)hyper_tx_queue;


    BACKEND_ENGINE_GET_F2B_QUEUE(eng, active_queue_f2b);
    BACKEND_ENGINE_GET_B2F_QUEUE(eng, active_queue_b2f);

    if(NULL == active_queue_f2b || NULL == active_queue_b2f){
        error_print("engine_run_hyperamp failed: The global backend engine's f2b session queue or b2f session queue has not been initialized!");
        return ;
    }

    if(NULL == eng->sess_pool || NULL == eng->sess_pool->ops){
        error_print("engine_run_hyperamp failed: Global backend engine's session pool (sess_pool) or its operation set (ops) is not initialized!");
        return ;
    }

    sess_pool       = eng->sess_pool;
    sess_pool_ops   = sess_pool->ops;
    net_poller      = &eng->poller;

    do{
/*
 * STEP (1)
 */
eng_run_step1:

        do{
    /*
     * Retrieve data from the Hyper AMP RX queue.
     */

            if(0)
                goto eng_run_step1;

            sleep(1);
            printf("In %s, engine run step1\n", __func__);
            ret = backend_engine_hyperamp_rx_queue_get(eng, HYPERAMP_MSG_HDR_PLUS_MAX_SIZE, msg_buf, &block_size);

    /*
     * If returning BACKEND_PROXY_PROCESS_ERROR, it indicates a system-level error (e.g., invalid queue handle, shared memory access exception, etc.)
     * Processing cannot continue; print error message and return directly.
     */
            if(BACKEND_PROXY_PROCESS_ERROR == ret){
                error_print("engine_run_hyperamp failed: failed to get data from the HyperAMP RX queue!\n");
                return;
            }


    /*
     * If returning BACKEND_PROXY_PROCESS_AGAIN, it indicates temporary inability to retrieve data (e.g., empty queue, resource temporarily occupied, etc., non-error state)     
     * No error reporting needed; jump to eng_run_step2 to execute the next process.
     */
            if(BACKEND_PROXY_PROCESS_AGAIN == ret){
                error_print("engine_run_hyperamp failed: HyperAMP RX queue is empty!\n");
                goto eng_run_step2;
            }


    /*
     * Process the proxy message.
     */     printf("In %s-0\n", __func__);
            backend_proxy_msg_process(msg_buf);
            printf("In %s-1\n", __func__);

        }while(BACKEND_PROXY_PROCESS_OK == ret);


/*
 * STEP (2)
 */
eng_run_step2:

/*
 * Recall the BACKEND_ENGINE_GET_F2B_QUEUE again to update active_queue_f2b, because the STEP (1) procedure may renew the front-to-back queue (queue_f2b) of the session pool.
 */
        printf("In %s, engine run step2\n", __func__);
        BACKEND_ENGINE_GET_F2B_QUEUE(eng, active_queue_f2b);

        TAILQ_FOREACH_SAFE(cur_sess, active_queue_f2b, entries_f2b, next_sess){
/*
 * Call the data_process_f2b function pointer in the session pool's operation set (sess_pool_ops), which attempts to send the front-to-end to back-end data maintained by the 
 * current session (cur_sess) via the socket maintained by this session.
 *
 * The return value corresponds to three scenarios:
 * Returns BACKEND_PROXY_PROCESS_OK: All data has been sent successfully.
 * Returns BACKEND_PROXY_PROCESS_AGAIN: Not all data has been sent, and no errors occurred.
 * Returns BACKEND_PROXY_PROCESS_ERROR: An error occurred during the sending process.
 */         
            printf("In %s, engine run step2-1\n", __func__);
            ret = sess_pool_ops->data_process_f2b(cur_sess);
            printf("In %s, engine run step2-2\n", __func__);
/*
 * If data_process_f2b returns BACKEND_PROXY_PROCESS_OK, this indicates all message segments in the front-to-back (F2B) message queue have been sent via the session's socket. 
 * Such sessions should be detached from the F2B active queue, and their "linked to queue" state flag should be cleared.
 */
            if(BACKEND_PROXY_PROCESS_OK == ret){
                printf("In %s, engine run step2-3\n", __func__);
                TAILQ_REMOVE(active_queue_f2b, cur_sess, entries_f2b);
                cur_sess->state_f2b &= ~BACKEND_SESS_LINKED_TO_QUEUE;
                printf("In %s, engine run step2-4\n", __func__);
            }
/*
 * If data_process_f2b returns BACKEND_PROXY_PROCESS_ERROR, it means an error occurs when trying to send data via the socket of the session. This type of session should not only be 
 * detached from the front-to-end active queue, but also be removed from the session pool.
 */
            if(BACKEND_PROXY_PROCESS_ERROR == ret){
                printf("In %s, engine run step2-5\n", __func__);
                TAILQ_REMOVE(active_queue_f2b, cur_sess, entries_f2b);
                printf("In %s, engine run step2-6\n", __func__);
                sess_pool_ops->delete_sess(sess_pool, cur_sess);
                printf("In %s, engine run step2-7\n", __func__);
            }
 /*
  * Nothing to do when not all data has been sent and there are no errors.
  */
        } // TAILQ_FOREACH_SAFE(cur_sess, active_queue_f2b, entries_f2b, next_sess)


/*
 * STEP (3)
 */
eng_run_step3:
/*
 * The execution process of poller_run function is as follows:
 * (1) Traverse the sockets in the epoll list, read data from them, and insert the data into the back-to-front message queue.
 * (2) Mark the sessions that have received data as active back-to-front active sessions.
 */
            if(0)
                goto eng_run_step3;

        printf("In %s, engine run step3\n", __func__);
        poller_run(eng, net_poller);



eng_run_step4:
/*
 * Recall the BACKEND_ENGINE_GET_B2F_QUEUE again to update active_queue_b2f, because the STEP (3) procedure may renew the front-to-back queue (queue_b2f) of the session pool.
 */

            if(0)
                goto eng_run_step4;

        printf("In %s, engine run step4\n", __func__);
        BACKEND_ENGINE_GET_B2F_QUEUE(eng, active_queue_b2f);

        printf("The address of active_queue_b2f = %p, the address of eng->sess_pool-> = %p\n", active_queue_b2f, &eng->sess_pool->queue_b2f);

        TAILQ_FOREACH_SAFE(cur_sess, active_queue_b2f, entries_b2f, next_sess){
/*
 * Call the data_process_b2f function pointer from the session pool's operation set (sess_pool_ops). This function attempts to send the back-to-front 
 * (B2F) data maintained by the current session (cur_sess) via the shared-memory TX queue.
 *
 * Return value scenarios:
 * - BACKEND_PROXY_PROCESS_OK: All data has been sent successfully.
 * - BACKEND_PROXY_PROCESS_AGAIN: Not all data was sent, and no errors occurred.
 * - BACKEND_PROXY_PROCESS_ERROR: An error occurred during the sending process.
 */
            printf("Before enter data_process_b2f\n");
            ret = sess_pool_ops->data_process_b2f(cur_sess);
            printf("After enter data_process_b2f\n");
/*
 * If data_process_b2f returns BACKEND_PROXY_PROCESS_OK, this indicates all message segments in the back-to-front (B2F) message queue have been successfully
 * sent via the shared memory TX queue. Such sessions should be detached from the B2F active queue.
 */
            if(BACKEND_PROXY_PROCESS_OK == ret){
                TAILQ_REMOVE(active_queue_b2f, cur_sess, entries_b2f);
                cur_sess->state_b2f &= ~BACKEND_SESS_LINKED_TO_QUEUE;
            }
/*
 * If data_process_b2f returns BACKEND_PROXY_PROCESS_ERROR, an error occurred while attempting to send data via the session's socket. Such sessions need to 
 * be both detached from the B2F active queue and removed from the session pool.
 */
            if(BACKEND_PROXY_PROCESS_ERROR == ret){
                TAILQ_REMOVE(active_queue_b2f, cur_sess, entries_b2f);
                sess_pool_ops->delete_sess(sess_pool, cur_sess);
            }
/*
 * If data_process_b2f returns BACKEND_PROXY_PROCESS_AGAIN, the shared-memory TX queue is full (not all data sent, no errors). Sending to the TX queue should 
 * stop, and ownership of the shared-memory TX queue should be transferred to the front-end. The front-end will then read this data from its RX queue, which
 * maps to the local TX queue (queue mapping: local TX <---> front-end RX).
 */
            if(BACKEND_PROXY_PROCESS_AGAIN == ret){
                break;
            }
        } // TAILQ_FOREACH_SAFE(cur_sess, active_queue_b2f, entries_b2f, next_sess)

/*
 * STEP (5)
 * Retrieve data from remote IoT devices, process it, and prepare responses for the frontend.
 */

eng_run_step5:
        if(0)
            goto eng_run_step5;

        engine_iot_dev_run(eng);
        sleep(1);

/*
 * Go back to STEP 1.
 */
    }while(1);
}


/**
 * @brief Destroys all resources associated with high-speed network devices managed by the backend engine
 * 
 * @details This function cleans up all resources occupied by high-speed network devices in the backend engine,
 *          following a strict resource release sequence to avoid leaks or undefined behavior:
 *          1. Perform defensive null pointer checks on critical input parameters (BackendEngine and its core members)
 *          2. Iterate over all high-speed network devices in the device set:
 *              - Close active TCP listener sockets if the listening port is configured
 *              - Release TCP session nodes: remove from queue, clear BACKEND_SESS_LINKED_TO_DEV_NODE flag,
 *                delete session instance via session pool ops, free node memory, and reset the TCP queue
 *              - Release UDP session nodes (same logic as TCP nodes) and reset the UDP queue
 *              - Free all nodes in the free node queue (no associated valid session instances) and reset the queue
 *          3. Close the epoll instance (epfd) used for I/O polling by the backend engine
 * 
 * @param eng Pointer to the BackendEngine structure that manages high-speed network devices and session pools
 *            - If eng is NULL, or critical members (dev_set, sess_pool, sess_pool->ops, delete_sess) are NULL,
 *              the function returns immediately without any cleanup
 *            - eng->dev_num specifies the total number of high-speed network devices to process
 * 
 * @note Session nodes are not recycled to the free node queue when deleting TCP/UDP sessions (the BACKEND_SESS_LINKED_TO_DEV_NODE
 *       flag is cleared before session deletion, preventing node recycling)
 * @note All node queues (TCP, UDP, free node) are explicitly reset with TAILQ_INIT after resource release to ensure clean state
 * @warning This function performs irreversible resource release: closed sockets/epoll instances and freed memory
 *          cannot be recovered; call only when the backend engine is shutting down or devices are no longer needed
 * @warning The function name contains a typo ("destory" should be "destroy") - ensure consistency with call sites
 */
void engine_destory_hs_net_dev(BackendEngine *eng){
    struct HighSpeedNetDeviceSet    *set = NULL;
    struct HighSpeedNetDevice       *hs_dev;
    struct BackendSessionPool       *sess_pool;
    struct BackendSessionPoolOps    *ops;
    int                             dev_num, cnt = 0;
    struct BackendSession           *tcp_sess, *udp_sess;
    struct SessionNode              *tmp_node, *next_tcp_node, *next_udp_node, *next_free_node;
//    const char *dev_name, *ip_addr;
//    int ip_type, dev_id, dev_type, dev_status;
//    int ns_id;
//    char *ns_name;
//    struct in_addr in4_addr;
//    struct in6_addr in6_addr;
//    union IPAddress *ip_data;

    if(NULL == eng || NULL == eng->dev_set || NULL == eng->sess_pool || NULL == eng->sess_pool->ops || NULL == eng->sess_pool->ops->delete_sess){
        return;
    }

    dev_num     = eng->dev_num;
    set         = eng->dev_set;
    sess_pool   = eng->sess_pool;
    ops         = sess_pool->ops;

/*
 * Release the various resources occupied by the device.
 */
    while(cnt < dev_num){
        hs_dev = &set->hs_net_dev[cnt];

        if(hs_dev->tcp_listening_port > 0){
            if(-1 != hs_dev->tcp_listener){
                close(hs_dev->tcp_listener);
            }
        }
/*
 * Through the session node, release the associated sessions of the device.
 *
 * There is no need to recycle the session node when deleting TCP and UDP sessions. If the BACKEND_SESS_LINKED_TO_DEV_NODE flag is cleared, the corresponding 
 * session node will not be recycled to the free node queue when the session instance  deletion procedure (high_speed_delete_sess) is invoked.
 */
        TAILQ_FOREACH_SAFE(tmp_node, &hs_dev->tcp_node_queue, entry, next_tcp_node) {
            TAILQ_REMOVE(&hs_dev->tcp_node_queue, tmp_node, entry);
            tcp_sess = tmp_node->sess;
            tcp_sess->sess_dev_link_state &= ~BACKEND_SESS_LINKED_TO_DEV_NODE;
            ops->delete_sess(sess_pool, tcp_sess);
            free(tmp_node);
            tmp_node = NULL;
        }

        TAILQ_INIT(&hs_dev->tcp_node_queue);

        TAILQ_FOREACH_SAFE(tmp_node, &hs_dev->udp_node_queue, entry, next_udp_node) {
            TAILQ_REMOVE(&hs_dev->udp_node_queue, tmp_node, entry);
            udp_sess = tmp_node->sess;
            udp_sess->sess_dev_link_state &= ~BACKEND_SESS_LINKED_TO_DEV_NODE;
            ops->delete_sess(sess_pool, udp_sess);
            free(tmp_node);
            tmp_node = NULL;
        }

        TAILQ_INIT(&hs_dev->udp_node_queue);

/*
 * Session nodes in the free node queue are not associated with any valid session instances, 
 * so they can be directly freed without additional checks.
 */
        TAILQ_FOREACH_SAFE(tmp_node, &hs_dev->free_node_queue, entry, next_free_node) {
            TAILQ_REMOVE(&hs_dev->free_node_queue, tmp_node, entry);
            free(tmp_node);
            tmp_node = NULL;
        }

        TAILQ_INIT(&hs_dev->free_node_queue);
/*
 * Release all the free session nodes.
 */
        cnt++;
    } // while(cnt < dev_num)

    close(eng->poller.epfd);

}


void engine_destory_sess_pool(BackendEngine *eng){
//    struct BackendSessionPool       *sess_pool;

    if(NULL == eng || NULL == eng->sess_pool){
        return;
    }
}


void engine_destory_mem_pool(BackendEngine *eng);

void engine_destory_mem_pool_lock(BackendEngine *eng);

void engine_destory(){
//    int ret;

    p_g_bk_eng = &g_bk_eng;

    engine_destory_hs_net_dev(p_g_bk_eng);

    memset(p_g_bk_eng, 0, sizeof(BackendEngine));
}
