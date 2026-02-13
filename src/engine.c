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
    int                 listen_fd, orig_netns;
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
    int ip_type, dev_id, dev_type, dev_status, ns_id, tcp_port, listenning_socket;
    char *ns_name;
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
        utils_print("dev_pro_item = %s, dev_name_len+ strlen(ip_addr) = %d\n", dev_pro_item, dev_name_len + strlen("ip_addr"));

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
        ns_name = iniparser_getstring(ini, dev_pro_item, -1);
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

    ret = engine_init_shared_mem_queue(p_g_bk_eng);

    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("engine_init failed: engine_init_shared_mem_queue returned error!\n");
        return;
    }

    utils_print("engine_init_shared_mem_queue() succeeded!\n");

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
#if 0
    if(NULL == eng || NULL == eng->hyper_rx_queue || eng->hyper_amp_data_region || NULL == data || NULL == out_len){
        error_print("frontend_engine_hyperamp_rx_queue_get failed: invalid input parameters (NULL pointers)\n");
        if(NULL == eng){
            error_print("eng is NULL!\n");
        }else{
            if(NULL == eng->hyper_rx_queue){
                error_print("hyper_rx_queue is NULL!\n");
            }

            if(NULL == eng->hyper_amp_data_region){
                error_print("hyper_amp_data_region is NULL!\n");
            }
        }

        if(NULL == data){
            error_print("data is NULL!\n");
        }

        if(NULL == out_len){
            error_print("out_len is NULL!\n");
        }
        return BACKEND_PROXY_PROCESS_ERROR;   
    }
#endif

    if(NULL == eng){
        error_print("frontend_engine_hyperamp_rx_queue_get failed: eng is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == eng->hyper_rx_queue){
        error_print("frontend_engine_hyperamp_rx_queue_get failed: eng->hyper_rx_queue is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == eng->hyper_amp_data_region){
        error_print("frontend_engine_hyperamp_rx_queue_get failed: eng->hyper_amp_data_region is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    
    if(NULL == data){
        error_print("frontend_engine_hyperamp_rx_queue_get failed: data is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(NULL == out_len){
        error_print("frontend_engine_hyperamp_rx_queue_get failed: out_len is NULL!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    

    ret = hyperamp_queue_dequeue(eng->hyper_rx_queue, HYPERAMP_ZONE_ID_Linux, data, max_msg_len, out_len, eng->hyper_amp_data_region);

    if(HYPERAMP_ERROR == ret){
        error_print("frontend_engine_hyperamp_rx_queue_get failed: hyperamp_queue_dequeue execution failed!\n");
        return BACKEND_PROXY_PROCESS_ERROR;  
    }else if(HYPERAMP_AGAIN == ret){
        return BACKEND_PROXY_PROCESS_AGAIN;
    }else{
/* 
 * hyperamp_queue_dequeue execution succeeded, no action required. 
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
    struct SharedMemoryPoolQueue    *rx_queue, *tx_queue;
    struct BackendSessionQueue      *active_queue_f2b, *active_queue_b2f;
    struct BackendSession           *cur_sess, *next_sess;
    struct BackendSessionPool       *sess_pool;
    struct BackendSessionPoolOps    *sess_pool_ops;
    NetPoller                       *net_poller;
    uint8_t                         *proxy_msg;
    uint32_t                        msg_size;
    int                             ret;

    eng = get_global_backend_engine();

    if(NULL == eng){
        error_print("engine_run failed: the global backend engine is not initialized!");
        return ;
    }

/* 
 * rx_queue: Local receive queue, which actually maps to the front-end's transmit queue (tx_queue).
 * Data sent by the front-end through its tx_queue will be received by the local side via this rx_queue.
 * 
 * tx_queue: Local transmit queue, which is used as the front-end's receive queue (rx_queue).
 * Data sent by the local side through this tx_queue will be received by the front-end via its rx_queue.
 */
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

    if(NULL == eng->sess_pool || NULL == eng->sess_pool->ops){
        error_print("engine_run failed: Global backend engine's session pool (sess_pool) or its operation set (ops) is not initialized!");
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

/*
 * Recall the BACKEND_ENGINE_GET_F2B_QUEUE again to update active_queue_f2b, because the STEP (1) procedure may renew the front-to-back queue (queue_f2b) of the session pool.
 */
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
            ret = sess_pool_ops->data_process_f2b(cur_sess);

/*
 * If data_process_f2b returns BACKEND_PROXY_PROCESS_OK, this indicates all message segments in the front-to-back (F2B) message queue have been sent via the session's socket. 
 * Such sessions should be detached from the F2B active queue, and their "linked to queue" state flag should be cleared.
 */
            if(BACKEND_PROXY_PROCESS_OK == ret){
                TAILQ_REMOVE(active_queue_f2b, cur_sess, entries_f2b);
                cur_sess->state_f2b &= ~BACKEND_SESS_LINKED_TO_QUEUE;
            }
/*
 * If data_process_f2b returns BACKEND_PROXY_PROCESS_ERROR, it means an error occurs when trying to send data via the socket of the session. This type of session should not only be 
 * detached from the front-to-end active queue, but also be removed from the session pool.
 */
            if(BACKEND_PROXY_PROCESS_ERROR == ret){
                TAILQ_REMOVE(active_queue_f2b, cur_sess, entries_f2b);
                sess_pool_ops->delete_sess(sess_pool, cur_sess);
            }
 /*
  * Nothing to do when not all data has been sent and there are no errors.
  */
        } // TAILQ_FOREACH_SAFE(cur_sess, active_queue_f2b, entries_f2b, next_sess)

/*
 * Complete data reception from the shared memory region for this operation.
 * Unlock the RX queue to allow the front-end to write data into it.
 */
    SHARED_MEM_QUEUE_UNLOCK(rx_queue);

/*
 * STEP (3)
 */
eng_run_step3:
/*
 * The execution process of poller_run function is as follows:
 * (1) Traverse the sockets in the epoll list, read data from them, and insert the data into the back-to-front message queue.
 * (2) Mark the sessions that have received data as active back-to-front active sessions.
 */
        poller_run(eng, net_poller);

/*
 * STEP (4)
 */
eng_run_step4:
/*
 * Recall the BACKEND_ENGINE_GET_B2F_QUEUE again to update active_queue_b2f, because the STEP (3) procedure may renew the front-to-back queue (queue_b2f) of the session pool.
 */

/* 
 * Acquire access lock for the TX queue.
 */
        ret = SHARED_MEM_QUEUE_LOCK(tx_queue);

/* 
 * If returning BACKEND_PROXY_PROCESS_ERROR, it indicates a system-level error (e.g., invalid lock handle, shared memory pool corruption)
 * Failed to acquire the lock; print error message and exit the current flow.
 */
        if(BACKEND_PROXY_PROCESS_ERROR == ret){
            error_print("engine_run failed: failed to get the lock of the TX queue!");
            return;
        }

        BACKEND_ENGINE_GET_B2F_QUEUE(eng, active_queue_b2f);

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
            ret = sess_pool_ops->data_process_b2f(cur_sess);

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

        SHARED_MEM_QUEUE_UNLOCK(tx_queue);
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
    const char *dev_name, *ip_addr;
    int ip_type, dev_id, dev_type, dev_status, ns_id;
    char *ns_name;
    struct in_addr in4_addr;
    struct in6_addr in6_addr;
    union IPAddress *ip_data;

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
    struct BackendSessionPool       *sess_pool;

    if(NULL == eng || NULL == eng->sess_pool){
        return;
    }
}


void engine_destory_mem_pool(BackendEngine *eng);

void engine_destory_mem_pool_lock(BackendEngine *eng);

void engine_destory(){
    int ret;

    p_g_bk_eng = &g_bk_eng;

    engine_destory_hs_net_dev(p_g_bk_eng);

    memset(p_g_bk_eng, 0, sizeof(BackendEngine));
}
