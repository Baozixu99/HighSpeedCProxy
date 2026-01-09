#ifndef ENGINE_H
#define ENGINE_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>

#include "dev.h"
#include "session_pool.h"
#include "backend_proto.h"
#include "shared_mem_io.h"
#include "poller.h"
#include "common_utils.h"


#define HS_NET_DEV_CFG          "hs_net_dev.ini"

#define MAX_TCP_PENDING_CONN    10

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
    struct HighSpeedNetDeviceSet    *dev_set;       // High speed network device set
    uint16_t                        dev_num;
    uint16_t                        active_mask;    // Show the positions of all the active high-speed network devices as a mask.
    struct BackendSessionPool       *sess_pool;     // Session pool
    struct SharedMemoryPool         *mem_pool;      // Shared memory pool
    struct SharedMemoryPoolLock     *mem_pool_lock; // Shared memory pool lock
    uint16_t                        selector_id;
    NetPoller                       poller;
    int                             bind_udp_socket; // Socket descriptor for the listening UDP port (bound UDP port fd)
    int                             bind_tcp_socket; // Socket descriptor for the listening TCP port (bound TCP port fd)
    struct SharedMemoryPoolQueue    *rx_queue;       // RX queue
    struct SharedMemoryPoolQueue    *tx_queue;       // TX queue
    struct BackendEngOps            *ops;
} BackendEngine;


struct BackendEngOps {
    int (*enable_dev)(BackendEngine *eng, uint16_t mask);                            // Enable devices in the high-speed network device set
    int (*disable_dev)(BackendEngine *eng, uint16_t mask);                           // Disable devices in the high-speed network device set
    int (*query_dev)(BackendEngine *eng, uint16_t *mask);                            // Query information/status of devices in the high-speed network device set
    int (*choose_dev)(BackendEngine *eng, uint16_t *dev_id);                         // Choose the most appropriate high-speed network device over which to establish a session, and record its device ID
    int (*conf_dev_selector)(BackendEngine *eng, uint16_t sel_id);                        // Configure the device selection function/strategy for the high-speed network device set
    int (*query_dev_sel_id)(BackendEngine *eng, uint16_t *sel_id);                        // Query the ID of the current device selector in the backend engine
};

#define HS_DEV_SELECTOR_NAME_LEN                                       30
#define HS_DEV_SELECTOR_NUM                                            3


typedef struct{
    char sel_name[HS_DEV_SELECTOR_NAME_LEN];
    int (*choose_dev)(BackendEngine *eng, uint16_t *dev_id);
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
int engine_init_shared_mem_queue(BackendEngine *eng);
int engine_init_poller(BackendEngine *eng);

int create_hs_net_dev_tcp_listener(BackendEngine *eng, struct HighSpeedNetDevice *hs_dev, int ip_version);
void engine_destory_hs_net_dev(BackendEngine *eng);
void engine_destory_sess_pool(BackendEngine *eng);
void engine_destory_mem_pool(BackendEngine *eng);
void engine_destory_mem_pool_lock(BackendEngine *eng);

int engine_choose_hs_net(BackendEngine *eng, int *selected_dev_id);

void engine_listener_run(struct BackendEngine_ *eng);
/**
 * @brief Get the f2b queue from BackendEngine's session pool
 * 
 * This macro retrieves the f2b queue from the BackendSessionPool associated with a BackendEngine.
 * It includes null pointer checks for the engine and its session pool. Error messages are printed
 * via error_print() when checks fail, with the macro name included for debugging.
 * 
 * @param engine Pointer to a BackendEngine structure; must not be NULL for valid queue retrieval
 * @param result_var Variable to store the result (pointer to BackendSessionQueue or NULL)
 * 
 * @note The result is stored in the provided result_var, which should be of type
 *       'struct BackendSessionQueue *'
 */
#define BACKEND_ENGINE_GET_F2B_QUEUE(engine, result_var) \
    do { \
        /* Initialize result to NULL by default */ \
        (result_var) = NULL; \
 \
        /* Check if BackendEngine pointer is NULL */ \
        utils_print("In BACKEND_ENGINE_GET_F2B_QUEUE, address of engine is %p \n", engine); \
        if (!(engine)) { \
            error_print("[BACKEND_ENGINE_GET_F2B_QUEUE] Error: BackendEngine pointer is NULL when getting f2b queue"); \
        } \
        /* Check if session pool within BackendEngine is NULL */ \
        else if (!(engine)->sess_pool) { \
            error_print("[BACKEND_ENGINE_GET_F2B_QUEUE] Error: BackendSessionPool in BackendEngine is NULL when getting f2b queue"); \
        } \
        /* All checks passed - retrieve the f2b queue */ \
        else { \
            (result_var) = &(engine)->sess_pool->queue_f2b; \
        } \
    } while (0)

/**
 * @brief Get the b2f queue from BackendEngine's session pool
 * 
 * This macro retrieves the b2f queue from the BackendSessionPool associated with a BackendEngine.
 * It includes null pointer checks for the engine and its session pool. Error messages are printed
 * via error_print() when checks fail, with the macro name included for debugging.
 * 
 * @param engine Pointer to a BackendEngine structure; must not be NULL for valid queue retrieval
 * @param result_var Variable to store the result (pointer to BackendSessionQueue or NULL)
 * 
 * @note The result is stored in the provided result_var, which should be of type
 *       'struct BackendSessionQueue *'
 */
#define BACKEND_ENGINE_GET_B2F_QUEUE(engine, result_var) \
    do { \
        /* Initialize result to NULL by default */ \
        (result_var) = NULL; \
 \
        /* Check if BackendEngine pointer is NULL */ \
        if (!(engine)) { \
            error_print("[BACKEND_ENGINE_GET_B2F_QUEUE] Error: BackendEngine pointer is NULL when getting b2f queue"); \
        } \
        /* Check if session pool within BackendEngine is NULL */ \
        else if (!(engine)->sess_pool) { \
            error_print("[BACKEND_ENGINE_GET_B2F_QUEUE] Error: BackendSessionPool in BackendEngine is NULL when getting b2f queue"); \
        } \
        /* All checks passed - retrieve the b2f queue */ \
        else { \
            (result_var) = &(engine)->sess_pool->queue_b2f; \
        } \
    } while (0)


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
                               size_t buf_max_len, size_t *out_len);

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
                                size_t data_len, size_t *sent_len);



struct BackendEngOps *get_hs_backend_engine_ops();


#endif