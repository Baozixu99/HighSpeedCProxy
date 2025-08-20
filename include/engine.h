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

typedef struct BackendEngine_{
    struct HighSpeedNetDeviceSet    *dev_set; // High speed network device set
    uint32_t                        dev_num;
    uint32_t                        active_mask; // Show the positions of all the active high-speed network devices as a mask.
    struct BackendSessionPool       *sess_pool; // Session pool
    struct SharedMemoryPool         *mem_pool; // Shared memory pool
    struct SharedMemoryPoolLock     *mem_pool_lock; // Shared memory pool lock
    uint32_t                        selector_id;
    struct BackendEngOps            *ops;
} BackendEngine;


struct BackendEngOps {
    int (*enable_dev)(BackendEngine *eng, uint32_t mask);                            // Enable devices in the high-speed network device set
    int (*disable_dev)(BackendEngine *eng, uint32_t mask);                           // Disable devices in the high-speed network device set
    int (*query_dev)(BackendEngine *eng, uint32_t *mask);                            // Query information/status of devices in the high-speed network device set
    int (*choose_dev)(BackendEngine *eng, uint32_t *dev_id);                         // Choose the most appropriate high-speed network device over which to establish a session, and record its device ID
    int (*conf_dev_selector)(BackendEngine *eng, uint32_t sel_id);                        // Configure the device selection function/strategy for the high-speed network device set
    int (*query_dev_sel_id)(BackendEngine *eng, uint32_t *sel_id);                        // Query the ID of the current device selector in the backend engine
};

#define HS_DEV_SELECTOR_NAME_LEN                                       30
#define HS_DEV_SELECTOR_NUM                                            3


typedef struct{
    char sel_name[HS_DEV_SELECTOR_NAME_LEN];
    int (*choose_dev)(BackendEngine *eng, uint32_t *dev_id);
} HSDevSelector;


extern HSDevSelector *p_hs_dev_sel;

extern BackendEngine *p_g_bk_eng;


void engine_init();
void engine_run();
void engine_destory();

BackendEngine *get_global_backend_engine();

int engine_init_eng_ops(BackendEngine *eng);
int engine_init_selector(BackendEngine *eng);


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