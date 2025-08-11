#ifndef ENGINE_H
#define ENGINE_H

#include <arpa/inet.h>
#include <netinet/in.h>

#include "dev.h"
#include "session_pool.h"
#include "backend_proto.h"
#include "shared_mem_io.h"


#define HS_NET_DEV_CFG "hs_net_dev.ini"
/*
 * Check if the string is a valid IPv4 address
 * Parameter: ip_str - string to check
 * Returns: 1 - valid IPv4, 0 - invalid
 */
#define DEV_IS_IPV4(ip_str) ({ \
    struct in_addr addr; \
    inet_pton(AF_INET, (ip_str), &addr) == 1; \
})

/*
 * Check if the string is a valid IPv6 address
 * Parameter: ip_str - string to check
 * Returns: 1 - valid IPv4, 0 - invalid
 */
#define DEV_IS_IPV6(ip_str) ({ \
    struct in6_addr addr; \
    inet_pton(AF_INET6, (ip_str), &addr) == 1; \
})

/*
 * Check if the string is a valid IP address (IPv4 or IPv6)
 * Parameter: ip_str - string to check
 * Returns: 1 - valid IP, 0 - invalid
 */
#define DEV_IS_VALID_IP(ip_str) (DEV_IS_IPV4(ip_str) || DEV_IS_IPV6(ip_str))

/*
 * Get the IP address type
 * Parameter: ip_str - string to check
 * Returns: SESS_NON_IP_PROTO - invalid, SESS_IPV4_PROTO - IPv4, SESS_IPV6_PROTO - IPv6
 */
#define DEV_IP_TYPE(ip_str) ({ \
    int type = SESS_NON_IP_PROTO; \
    if (DEV_IS_IPV4(ip_str)) { \
        type = SESS_IPV4_PROTO; \
    } else if (DEV_IS_IPV6(ip_str)) { \
        type = SESS_IPV6_PROTO; \
    } \
    type; \
})


#define DEV_IS_VALID_IP(ip_str) (DEV_IS_IPV4(ip_str) || DEV_IS_IPV6(ip_str))

struct BackendEngOps; 

typedef struct {
    struct HighSpeedNetDeviceSet *dev_set; // High speed network device set
    struct BackendSessionPool *sess_pool; // Session pool
    struct SharedMemoryPool *mem_pool; // Shared memory pool
    struct SharedMemoryPoolLock *mem_pool_lock; // Shared memory pool lock
    struct BackendEngOps *ops;
} BackendEngine;


struct BackendEngOps {
    int (*enable_dev)(struct HighSpeedNetDeviceSet *dev_set);                        // Enable devices in the high-speed network device set
    int (*disable_dev)(struct HighSpeedNetDeviceSet *dev_set);                       // Disable devices in the high-speed network device set
    int (*query_dev)(struct HighSpeedNetDeviceSet *dev_set, uint32_t *mask);         // Query information/status of devices in the high-speed network device set
    int (*choose_dev)(BackendEngine *eng, struct HighSpeedNetDeviceSet *dev_set);    // Choose the most appropriate high-speed network device to establish a session over it
    int (*configure_dev_selector)(BackendEngine *eng);                               // Configure the device selection function/strategy for the high-speed network device set
    int (*query_dev_selector_id)(BackendEngine *eng);                                // Query the ID of the current device selector in the backend engine
};

extern BackendEngine *p_g_bk_eng;


void engine_init();
void engine_run();
void engine_destory();

BackendEngine *get_global_backend_engine();

int engine_init_hs_net_dev(BackendEngine *eng);
int engine_init_sess_pool(BackendEngine *eng);
int engine_init_shared_mem_pool(BackendEngine *eng);
int engine_init_shared_mem_pool_lock(BackendEngine *eng);

void engine_destory_hs_net_dev(BackendEngine *eng);
void engine_destory_sess_pool(BackendEngine *eng);
void engine_destory_mem_pool(BackendEngine *eng);
void engine_destory_mem_pool_lock(BackendEngine *eng);

int engine_choose_hs_net(BackendEngine *eng, int *selected_dev_id);
#endif