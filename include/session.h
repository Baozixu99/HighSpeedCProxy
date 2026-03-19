#ifndef SESSION_H
#define SESSION_H

#include <stdint.h> 
#include <sys/queue.h>
#include <sys/epoll.h>
#include "uthash.h"
#include "poller.h"
#include "shared_mem_io.h"

// Backend proxy processing result: Success (data read/written normally, logic executed completely)
#define BACKEND_PROXY_PROCESS_OK               0

// Backend proxy processing result: Failure (system-level error, such as memory allocation failure, invalid handle, protocol parsing error, etc. Requires error investigation)
#define BACKEND_PROXY_PROCESS_ERROR            -1

// Backend proxy processing result: Process temporarily unavailable (non-error state, only data read/write cannot be completed, such as empty queue, data not ready, resource temporarily occupied, etc. Retry is allowed)
#define BACKEND_PROXY_PROCESS_AGAIN            1

struct ControlMsg{
    uint16_t dev_id;
};


typedef enum {
    SESS_ESTABLISH_PASSIVE = 0,
    SESS_ESTABLISH_ACTIVE  = 1
} SessEstablishType;

/**
 * @brief Memory source type of the data field in message segment (SessMsgSeg)
 * 
 * Used to identify whether the memory pointed to by the data pointer in the SessMsgSeg structure
 * is dynamically allocated or comes from shared memory (to determine subsequent memory management 
 * approaches, such as whether manual release is required)
 */
typedef enum {
    SESS_MSG_SEG_DYNAMIC_ALLOC,  ///< data points to dynamically allocated memory (needs to be freed with free())
    SESS_MSG_SEG_SHARED_MEM      ///< data points to shared memory (no manual release needed; managed by the shared memory manager)
} SessMsgSegType;


/**
 * @brief Structure representing a segment of session message data, supporting both dynamic and shared memory
 * This structure encapsulates a segment of message data for session communication, including
 * metadata about the data buffer and the buffer itself. It can manage data in two modes:
 * dynamically allocated memory or shared memory (via a shared memory pool).
 */
struct SessMsgSeg {
/*< Length of the data buffer (in bytes). Indicates the valid data size in the buffer pointed to by @ref data. */
    uint16_t len; 
/*
 * < Memory source type of the data buffer, corresponding to SessMsgSegType.
 * Valid values: SESS_MSG_SEG_DYNAMIC_ALLOC (dynamic memory) or SESS_MSG_SEG_SHARED_MEM (shared memory). 
 */
    uint16_t type; 
/*
 * < Pointer to the associated shared memory pool.
 * Valid and non-NULL only when @ref type is SESS_MSG_SEG_SHARED_MEM, used for managing shared memory ownership and validation.
 * NULL when @ref type is SESS_MSG_SEG_DYNAMIC_ALLOC (no shared memory pool associated). 
 */
    struct SharedMemoryPool *mem_pool;
/*< Pointer to the actual data buffer.
 * For SESS_MSG_SEG_DYNAMIC_ALLOC: Points to memory allocated via malloc().
 * For SESS_MSG_SEG_SHARED_MEM: Points to a valid data segment within the shared memory managed by @ref mem_pool. 
 */ 
    uint8_t *data; 
/*
 * < TAILQ queue entry. Used to link multiple SessMsgSeg structures into a doubly linked list for sequential access. 
 */
    TAILQ_ENTRY(SessMsgSeg) entry; 
};


TAILQ_HEAD(SessMsgQueue, SessMsgSeg);


struct SessMsgSeg *sess_msg_seg_alloc(size_t len, SessMsgSegType type, const uint8_t *shared_data, struct SharedMemoryPool *mem_pool);
void sess_msg_seg_free(struct SessMsgSeg *seg_ptr);

void sess_msg_queue_free_all(struct SessMsgQueue *queue);

struct BackendProtocolProcess; 
struct BackendEngine_;

#define BACKEND_SESS_LINKED_TO_QUEUE                1
#define BACKEND_SESS_LINKED_TO_DEV_NODE             1

struct SessionNode;

struct BackendSession {
    int         sess_type;
    int         proto_type;
    int         dev_id;
    int         establish_mode;
    int         sock_fd;
    int         ip_version;
    uint16_t    frontend_sess_id;
    uint16_t    backend_sess_id; // hash key
/*
 * State machine states, indicating the linked status of the session in different directions
 * state_f2b: When its value is BACKEND_SESS_LINKED_TO_QUEUE, it means the entries_f2b node of the current session
 * is linked to the queue_f2b (belonging to BackendSessionPool in BackendEngine_)
 * state_b2f: When its value is BACKEND_SESS_LINKED_TO_QUEUE, it means the entries_b2f node of the current session
 * is linked to the queue_bf2 (belonging to BackendSessionPool in BackendEngine_)
 */
    uint8_t state_f2b; 
    uint8_t state_b2f; 

/**
 * Backend session and HighSpeedNetDevice link state flag
 * details  1. Basic state indication: When set to BACKEND_SESS_LINKED_TO_DEV_NODE, the current BackendSession
 *            is bound to either the udp_node or tcp_node queue of the struct HighSpeedNetDevice instance indexed by dev_id,
 *            and the specific queue (udp_node/tcp_node) is determined by the value of proto_type(SESS_TCP_PROTO/SESS_UDP_PROTO).
 *          2. Key function for session closure: Session closure can be either a normal shutdown or an abnormal shutdown
 *            caused by proxy unexpected termination. This state (BACKEND_SESS_LINKED_TO_DEV_NODE) ensures that
 *            the function pointed to by the delete_sess pointer can correctly handle both of these two closure scenarios
 *            during its invocation.
 */
    uint8_t sess_dev_link_state; 
// Message queues
    struct SessMsgQueue msg_f2b; // front-end to back-end message queue
    struct SessMsgQueue msg_b2f; // back-end to front-end message queue
// Queue link nodes
    TAILQ_ENTRY(BackendSession) entries_f2b; // front-end to back-end active queue node
    TAILQ_ENTRY(BackendSession) entries_b2f; // back-end to front-end active queue node
// Protocol processing
    struct BackendProtocolProcess *protocol_process; // protocol processing module pointer
// Pointer to the backend engine associated with this session
    struct BackendEngine_   *eng;
// Private data pointer (used to store session-specific data)
    struct SessionNode      *sess_node;
    void                    *pri_data;
    NetChannel              net_channel;
    UT_hash_handle          hh;
};

/**
 * @brief Link backend session to the specified queue (only if not linked yet) and set state bit
 * 
 * @param[in]  sess  Pointer to struct BackendSession (target session)
 * @param[in]  dir   Direction identifier ("f2b" for front2back, "b2f" for back2front)
 * 
 * @details 1. Validate critical pointers (sess/eng/sess_pool)
 *          2. Check if BACKEND_SESS_LINKED_TO_QUEUE bit is NOT set in state_<dir>
 *          3. If not set: set the bit (bitwise OR) + insert entries_<dir> into queue_<dir>
 * 
 * @note - Avoids duplicate linking (prevents inserting the same node into TAILQ multiple times)
 *       - BACKEND_SESS_LINKED_TO_QUEUE must be a single-bit mask (e.g., 1U<<0)
 */
#define BACKEND_SESS_LINK_TO_QUEUE(sess, dir) do {                          \
    /* Step 1: Validate pointers to avoid null dereference */                \
    if ((sess) != NULL && (sess)->eng != NULL && (sess)->eng->sess_pool != NULL) { \
        /* Step 2: Check if NOT linked yet (target bit is 0) */              \
        if (((sess)->state_##dir & BACKEND_SESS_LINKED_TO_QUEUE) == 0) {      \
            /* Step 3: Set linked bit + insert into queue */                 \
            (sess)->state_##dir |= BACKEND_SESS_LINKED_TO_QUEUE;              \
            TAILQ_INSERT_TAIL(&(sess)->eng->sess_pool->queue_##dir, (sess), entries_##dir); \
        }                                                                     \
    }                                                                        \
} while (0)



/**
 * @brief Unlink backend session from the specified queue (only if linked) and clear state bit
 * 
 * @param[in]  sess  Pointer to struct BackendSession (target session)
 * @param[in]  dir   Direction identifier ("f2b" for front2back, "b2f" for back2front)
 * 
 * @details 1. Validate critical pointers (sess/eng/session_pool)
 *          2. Check if BACKEND_SESS_LINKED_TO_QUEUE bit is set in state_<dir>
 *          3. If set: remove entries_<dir> from queue_<dir> + clear the bit (bitwise AND NOT)
 * 
 * @note - Avoids invalid unlinking (prevents removing a node not in TAILQ)
 *       - Ensure queue_<dir> is initialized before calling
 */
#define BACKEND_SESS_UNLINK_FROM_QUEUE(sess, dir) do {                      \
    /* Step 1: Validate pointers to avoid null dereference */                \
    if ((sess) != NULL && (sess)->eng != NULL && (sess)->eng->sess_pool != NULL) { \
        /* Step 2: Check if already linked (target bit is 1) */              \
        if (((sess)->state_##dir & BACKEND_SESS_LINKED_TO_QUEUE) != 0) {      \
            /* Step 3: Remove from queue + clear linked bit */                \
            TAILQ_REMOVE(&(sess)->eng->sess_pool->queue_##dir, (sess), entries_##dir); \
            (sess)->state_##dir &= ~BACKEND_SESS_LINKED_TO_QUEUE;             \
        }                                                                     \
    }                                                                        \
} while (0)


/**
 * @brief Get the associated shared memory pool (mem_pool) from a BackendSession pointer
 * This macro retrieves the shared_memory_pool pointer contained in BackendEngine_
 * through the eng member (pointing to BackendEngine_) of the BackendSession structure.
 * @param sess Pointer to a struct BackendSession
 * @return Pointer to struct SharedMemoryPool, i.e., sess->eng->mem_pool
 */
#define BACKEND_SESSION_MEM_POOL(sess) ((sess)->eng->mem_pool)


/**
 * @brief Insert a SessMsgSeg pointer into the specified direction queue of BackendSession
 * 
 * @param[in]  sess  Pointer to struct BackendSession (the session containing the target queue)
 * @param[in]  seg   Pointer to struct SessMsgSeg (the message segment to be inserted)
 * @param[in]  dir   Direction identifier, must be "f2b" (front2back) or "b2f" (back2front)
 * 
 * @details 1. Validate that sess and seg are non-NULL to avoid null dereference
 *          2. Insert seg into sess->msg_<dir> queue using TAILQ_INSERT_TAIL (FIFO order)
 *          3. The queue is identified by concatenating "msg_" with dir (msg_f2b or msg_b2f)
 * 
 * @note - <dir> must be "f2b" or "b2f"; invalid values will cause compilation errors
 *       - Ensure sess->msg_<dir> has been initialized with TAILQ_INIT() before insertion
 *       - seg must point to a valid SessMsgSeg instance (allocated and initialized)
 *       - This macro performs a tail insertion to maintain FIFO order of messages
 */
#define SESS_MSG_SEG_INSERT_QUEUE(sess, seg, dir) do {                     \
    /* Validate critical pointers */                                        \
    if ((sess) != NULL && (seg) != NULL) {                                  \
        /* Insert the segment into the target queue (msg_f2b or msg_b2f) */ \
        TAILQ_INSERT_TAIL(&(sess)->msg_##dir, (seg), entry);                \
    }                                                                        \
} while (0)


/**
 * @brief Remove and return the first SessMsgSeg pointer from the specified direction queue of BackendSession
 * 
 * @param[in]  sess     Pointer to struct BackendSession (the session containing the target queue)
 * @param[out] seg_ptr  Double pointer to struct SessMsgSeg (output: receives the removed segment; set to NULL if queue is empty)
 * @param[in]  dir      Direction identifier, must be "f2b" (front2back) or "b2f" (back2front)
 * 
 * @details 1. Validate that sess and seg_ptr are non-NULL to avoid null dereference
 *          2. Check if the target queue (msg_<dir>) is non-empty using TAILQ_FIRST
 *          3. If non-empty: remove the first element with TAILQ_REMOVE and assign to *seg_ptr
 *          4. If empty: set *seg_ptr to NULL
 * 
 * @note - <dir> must be "f2b" or "b2f"; invalid values will cause compilation errors
 *       - Ensure sess->msg_<dir> has been initialized with TAILQ_INIT() before removal
 *       - seg_ptr must be a valid double pointer (points to a struct SessMsgSeg* variable)
 *       - The removed segment's memory is not freed by this macro (caller must handle via sess_msg_seg_free)
 */
#define SESS_MSG_SEG_REMOVE_HEAD(sess, seg_ptr, dir) do {                  \
    /* Validate critical pointers */                                        \
    if ((sess) != NULL && (seg_ptr) != NULL) {                              \
        /* Initialize output to NULL (handles empty queue case) */           \
        *(seg_ptr) = NULL;                                                  \
        /* Check if queue is non-empty */                                    \
        if (TAILQ_FIRST(&(sess)->msg_##dir) != NULL) {                      \
            /* Get the first element and remove it from the queue */         \
            *(seg_ptr) = TAILQ_FIRST(&(sess)->msg_##dir);                   \
            TAILQ_REMOVE(&(sess)->msg_##dir, *(seg_ptr), entry);             \
        }                                                                    \
    }                                                                        \
} while (0)



/**
 * @brief Enumeration representing epoll registration status of a session
 */
typedef enum {
    EPOLL_REG_STATUS_UNREGISTERED,  // Socket not registered with epoll (initial state)
    EPOLL_REG_STATUS_REGISTERED     // Socket registered with epoll
} EpollRegistrationStatus;


/**
 * @brief Register or modify the socket file descriptor of BackendSession in epoll instance with specified events, with converted return value
 * 
 * @param[in]  sess        Pointer to struct BackendSession (the session containing the socket to process)
 * @param[in]  reg_events  Epoll event to register (e.g., EPOLLIN, EPOLLET, EPOLLOUT, etc.)
 * @param[out] ret_ptr     Pointer to int (output: receives converted result; 
 *                         BACKEND_PROXY_PROCESS_OK on success, BACKEND_PROXY_PROCESS_ERROR on failure)
 * 
 * @details 1. Initialize epoll_event structure with the specified events and update NetChannel's events
 *          2. Determine operation type (EPOLL_CTL_ADD or EPOLL_CTL_MOD) based on NetChannel status
 *          3. Execute epoll_ctl operation and get its raw return value
 *          4. Convert return value: 0 → BACKEND_PROXY_PROCESS_OK; -1 → BACKEND_PROXY_PROCESS_ERROR
 *          5. Store converted result in *ret_ptr
 * 
 * @note - ret_ptr must be a valid pointer to an int (cannot be NULL)
 *       - event can be a combination of epoll event flags (e.g., EPOLLIN | EPOLLET)
 *       - BACKEND_PROXY_PROCESS_OK and BACKEND_PROXY_PROCESS_ERROR are assumed to be predefined status codes
 */
#define BACKEND_SESS_REGISTER_EPOLL(sess, reg_events, ret_ptr) do { \
    struct epoll_event ev; \
    int op; \
    int epoll_ret;  /* Temporary variable to store raw epoll_ctl return value */ \
    \
    ev.events = (reg_events); \
    (sess)->net_channel.events = (reg_events); \
    ev.data.ptr = &(sess)->net_channel; \
    \
    if ((sess)->net_channel.status == EPOLL_REG_STATUS_UNREGISTERED) { \
        op = EPOLL_CTL_ADD; \
        (sess)->net_channel.status = EPOLL_REG_STATUS_REGISTERED; \
    } else if ((sess)->net_channel.status == EPOLL_REG_STATUS_REGISTERED){ \
        op = EPOLL_CTL_MOD; \
    } else{\
        utils_print("BACKEND_SESS_REGISTER_EPOLL failed: invalid net_channel status\n!"); \
        *(ret_ptr) = BACKEND_PROXY_PROCESS_ERROR; \
        break;                                    \
    }\
    \
    /* Get raw return value from epoll_ctl */ \
    epoll_ret = epoll_ctl(((NetPoller*)(sess)->net_channel.arg)->epfd, \
                          op, \
                          (sess)->net_channel.sock_fd, \
                          &ev); \
    \
    /* Convert to predefined status codes */ \
    *(ret_ptr) = (epoll_ret == 0) ? BACKEND_PROXY_PROCESS_OK : BACKEND_PROXY_PROCESS_ERROR; \
} while(0)



/**
 * @brief Unregister the socket file descriptor of BackendSession from epoll instance, with converted return value
 * 
 * @param[in]  sess     Pointer to struct BackendSession (the session containing the socket to unregister)
 * @param[out] ret_ptr  Pointer to int (output: receives converted result;
 *                      BACKEND_PROXY_PROCESS_OK on success, BACKEND_PROXY_PROCESS_ERROR on failure)
 * 
 * @details 1. Retrieve epoll instance descriptor (epfd) from NetPoller via NetChannel's arg member
 *          2. Perform epoll_ctl DEL operation to remove the socket from epoll monitoring
 *          3. Set NetChannel status to EPOLL_REG_STATUS_UNREGISTERED (unregistered state)
 *          4. Convert epoll_ctl return value: 0 → BACKEND_PROXY_PROCESS_OK; -1 → BACKEND_PROXY_PROCESS_ERROR
 *          5. Store converted result in *ret_ptr
 * 
 * @note - ret_ptr must be a valid pointer to an int (cannot be NULL)
 *       - Refer to epoll_ctl(2) man page for detailed error codes (stored in errno on failure)
 *       - The socket itself is not closed by this macro (caller must handle closing if needed)
 *       - BACKEND_PROXY_PROCESS_OK and BACKEND_PROXY_PROCESS_ERROR are assumed to be predefined status codes
 */
#define BACKEND_SESS_UNREGISTER_EPOLL(sess, ret_ptr) do { \
    int epoll_ret;  /* Temporary variable to store raw epoll_ctl return value */ \
    \
    /* Execute epoll_ctl DEL operation and get raw return value */ \
    epoll_ret = epoll_ctl(((NetPoller*)(sess)->net_channel.arg)->epfd, \
                          EPOLL_CTL_DEL, \
                          (sess)->net_channel.sock_fd, \
                          NULL); \
    \
    /* Update status (maintain state consistency regardless of epoll_ctl result) */ \
    (sess)->net_channel.status = EPOLL_REG_STATUS_UNREGISTERED; \
    \
    /* Convert to predefined status codes */ \
    *(ret_ptr) = (epoll_ret == 0) ? BACKEND_PROXY_PROCESS_OK : BACKEND_PROXY_PROCESS_ERROR; \
} while(0)


struct BackendProtocolProcess {

    int (*connect)(struct BackendSession* sess);
    
    int (*accept)(struct BackendSession* sess);
    
    int (*read)(struct BackendSession* sess, uint8_t* data, uint32_t size);
    
    int (*write)(struct BackendSession* sess, const uint8_t* data, uint32_t size);
    
    int (*close)(struct BackendSession* sess);

};

TAILQ_HEAD(BackendSessionQueue, BackendSession);

struct BackendSessionID {
    uint16_t id;
    TAILQ_ENTRY(BackendSessionID) entry;
};
TAILQ_HEAD(BackendSessionIDQueue, BackendSessionID);


#ifndef TAILQ_HEAD
#define TAILQ_HEAD(name, type) \
struct name { \
    struct type *tqh_first; /* First element */ \
    struct type **tqh_last; /* Last element's next pointer */ \
}
#endif


#ifndef TAILQ_END
#define TAILQ_END(head) NULL  // Standard definition: end of queue is NULL
#endif

// Manually define TAILQ_FOREACH_SAFE macro (only use if the system header doesn't support it)
#ifndef TAILQ_FOREACH_SAFE

#if 0
#define TAILQ_FOREACH_SAFE(var, head, field, next_var) \
    for ((var) = TAILQ_FIRST((head)), \
         (next_var) = TAILQ_NEXT((var), field); \
         (var) != TAILQ_END((head)); \
         (var) = (next_var), (next_var) = TAILQ_NEXT((var), field))
#endif

#define TAILQ_FOREACH_SAFE(var, head, field, next_var) \
    for (var = TAILQ_FIRST(head); \
         var != TAILQ_END(head) && (next_var = TAILQ_NEXT(var, field), 1); \
         var = next_var)

#endif


/**
 * @brief IoT session link state enumeration
 */
typedef enum {
    IOT_SESS_STATE_UNINIT = 0,    // Session uninitialized
    IOT_SESS_STATE_LINKED,        // Session linked to IoT device (active)
    IOT_SESS_STATE_UNLINKED,      // Session unlinked from device (inactive)
    IOT_SESS_STATE_ERROR          // Session in error state (need reconnection)
} IotSessLinkState;

/**
 * @brief IoT session working mode (matches IoT device config)
 */
typedef enum {
    IOT_SESS_MODE_CLIENT = 0,     // Client mode (initiates connection)
    IOT_SESS_MODE_GATEWAY = 1     // Gateway/server mode (listens for connections)
} IotSessMode;


/**
 * @brief IoT message queue structure (optimized for IoT small data packets)
 * @note Simplified version of SessMsgQueue for IoT characteristics
 */
typedef struct IotSessMsgQueue_ {
    void                *msg_buf;    // Message buffer (fixed size for IoT)
    uint32_t            buf_size;    // Buffer size (e.g., 4096 bytes)
    uint32_t            msg_count;   // Number of pending messages
    int                 queue_fd;    // Event fd for queue notification
} IotSessMsgQueue;



struct IotMsgBuffer_;
typedef struct IotMsgBuffer_ IotMsgBuffer;


/**
 * @brief Socket node for managing multiple file descriptors within a single session.
 * 
 * Some IoT sessions (e.g., Bluetooth Gateway, Modbus TCP Server) may involve 
 * multiple sockets: one listening socket, one or more connected client sockets, 
 * or separate control/data channels. This struct allows tracking them in a list.
 */
typedef struct IotSockNode_ {
    int                     fd;                  // The file descriptor of the socket
    TAILQ_ENTRY(IotSockNode_) entries;           // Linkage member for the TAILQ
} IotSockNode;

// Define the TAILQ head type for the list of IotSockNode
TAILQ_HEAD(IotSockList_, IotSockNode_);
typedef struct IotSockList_ IotSockList;


/**
 * @brief Working mode of an IoT session/device.
 * 
 * Defines the operational role of the entity in the IoT architecture.
 * This dictates the connection behavior (initiator vs listener) and data flow direction.
 * 
 * - CLIENT_MODE: The device initiates connections and reports data (e.g., Sensors, Actuators).
 * - SERVER_MODE: The device listens for connections and manages clients (e.g., Gateways, Hubs).
 */
typedef enum IotWorkMode_ {
    /** 
     * @brief Client Mode (Initiator / Edge Device).
     * 
     * Characteristics:
     * - Actively initiates connections to a server/gateway.
     * - Primary data flow: Uplink (Device -> Server).
     * - Responsible for auto-reconnection on link failure.
     * - Typical devices: Temperature sensors, Smart plugs, Wearables.
     * - Network behavior: Uses connect(), no listen_fd.
     */
    IOT_WORK_MODE_CLIENT = 0,

    /** 
     * @brief Server Mode (Listener / Gateway / Hub).
     * 
     * Characteristics:
     * - Passively waits for incoming connections from clients.
     * - Primary data flow: Downlink management & Uplink aggregation.
     * - Responsible for accepting new clients and managing multiple sessions.
     * - Typical devices: IoT Gateways, Edge Servers, Modbus TCP Masters (acting as servers).
     * - Network behavior: Uses bind() + listen() + accept(), owns listen_fd.
     */
    IOT_WORK_MODE_SERVER = 1
} IotWorkMode;

/**
 * @brief IoT backend session object (for Bluetooth/CAN/Zigbee/LoRa/OpenPowerLink)
 * @note Each IoT device has exactly one corresponding session (1:1 mapping)
 * @note No session pool required - directly bound to IoT device instance
 */
typedef struct IoTBackendSession_ {
    // 1. Session core identification (1:1 binding with IoT device)
    int                 sess_type;           // IoT session type (matches protocol type)
    int                 sess_role;
    int                 dev_id;              // Associated IoT device ID (global unique)
    int                 sess_id;             // IoT session ID (same as device's sess_id)
    IotWorkMode         working_mode;        // Session working mode (client/gateway)
    
    // 2. Link state management (simplified for IoT 1:1 mapping)
    IotSessLinkState    sess_dev_link_state; // Session-device link state
    uint64_t            last_link_ts;        // Last link state change timestamp (ms)
    uint32_t            reconnect_count;     // Reconnection attempt count (for error recovery)
    
    // 3. Device association (direct pointer to IoT device object)
    struct IotDevice_   *bound_dev;          // Pointer to bound IoT device (core association)
    struct BackendEngine_ *eng;              // Pointer to parent backend engine
    
    // 4. Message queues (optimized for IoT small packets)
    IotSessMsgQueue     msg_dev2eng;         // Device to engine message queue (data report/notify)
    IotSessMsgQueue     msg_eng2dev;         // Engine to device message queue (control command)
    
    // 5. Protocol processing (IoT protocol-specific handler)
    void                *proto_handler;      // Protocol-specific processing module (e.g., BLE handler)
    void                *pri_data;           // Private data (protocol-specific context)
    
    // 6. Hardware/IO related (IoT device access)
    int                 dev_fd;              // Bound device file descriptor (same as device's fd)
    int                 listen_fd;           // Listening socket for connection-oriented protocols (e.g., Bluetooth L2CAP, Modbus TCP). 
    IotSockList         sock_list;           // Head of the linked list for socket nodes
    uint32_t            io_timeout;          // IO operation timeout (ms, e.g., 5000)
    
    // 7. Statistics (session-level performance metrics)
    uint64_t            tx_packets;          // Total packets sent via session
    uint64_t            rx_packets;          // Total packets received via session
    uint64_t            tx_bytes;            // Total bytes sent
    uint64_t            rx_bytes;            // Total bytes received
    uint32_t            error_count;         // Total session errors
        /**
     * @brief Send data to remote IoT device/network (protocol-agnostic)
     * @param sess Pointer to IoTBackendSession instance
     * @param msg_buf Pointer to IotMsgBuffer (contains data + destination address)
     * @return int BACKEND_PROXY_PROCESS_OK on success, BACKEND_PROXY_PROCESS_ERROR on failure
     */
    int (*send_to_remote)(struct IoTBackendSession_ *sess, const IotMsgBuffer *msg_buf);

    /**
     * @brief Receive data from remote IoT device/network (protocol-agnostic)
     * @param sess Pointer to IoTBackendSession instance
     * @param msg_buf Pointer to IotMsgBuffer (stores data + source address)
     * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = blocking)
     * @return int BACKEND_PROXY_PROCESS_OK on success, BACKEND_PROXY_PROCESS_ERROR on failure
     */
    int (*recv_from_remote)(struct IoTBackendSession_ *sess, IotMsgBuffer *msg_buf, int timeout_ms);
    // 8. List linkage (for engine-level session management)
    TAILQ_ENTRY(IoTBackendSession_) entries; // Linked list node for engine session list
} IoTBackendSession;


extern IoTBackendSession *backend_bluetooth_sess;
extern IoTBackendSession *backend_can_sess;
extern IoTBackendSession *backend_zigbee_sess;
extern IoTBackendSession *backend_lora_sess;
extern IoTBackendSession *backend_powerlink_sess;
extern IoTBackendSession *backend_modbustcp_sess;


int bluetooth_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf);
int bluetooth_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms);

int can_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf);
int can_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms);

int zigbee_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf);
int zigbee_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms);

int lora_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf);
int lora_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms);

int powerlink_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf);
int powerlink_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms);


int session_send(struct BackendSession* sess, const uint8_t* data, uint32_t size);
int session_recv(struct BackendSession* sess, uint8_t* data, uint32_t size);
void delete_session(struct BackendSession* sess);



struct IotDevice_;
typedef struct IotDevice_ IotDevice;
struct BackendEngine_;
typedef struct BackendEngine_ BackendEngine;


int engine_init_bluetooth_session(IotDevice *dev, IoTBackendSession *sess);

int engine_init_can_session(IotDevice *dev, IoTBackendSession *sess);

int engine_init_zigbee_session(IotDevice *dev, IoTBackendSession *sess);

int engine_init_lora_session(IotDevice *dev, IoTBackendSession *sess);

int engine_init_powerlink_session(IotDevice *dev, IoTBackendSession *sess);

void backend_iot_sess_destroy(void);



/**
 * @brief Universal cleanup interface for IoT device backend communication session
 * @param sess Pointer to pre-allocated IoTBackendSession instance (IoT device only)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed due to invalid input or unknown IoT session type
 *
 * @note This function is dedicated for IoT device session cleanup ONLY.
 *       It is NOT used for IP network device session cleanup.
 *       Automatically dispatches to protocol-specific handler based on sess_type (paired with IoT device type).
 *       Only clears IoT session context, state and control flags.
 *       NO device hardware deinitialization.
 *       NO memory free operation for IoTBackendSession (memory managed externally).
 *
 * @warning This function is the reverse of IoT device session initialization functions
 * @warning Only for IoT device sessions, DO NOT use for IP network sessions
 * @warning sess_type must be paired with corresponding IoT device type
 * @warning Ensure all IoT data interaction completed before calling
 */
int backend_cleanup_iot_session(IoTBackendSession *sess);

/**
 * @brief Internal cleanup handler for Bluetooth IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Bluetooth IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for Bluetooth IoT session cleanup only.
 *       Resets session context, state and transceiving flags.
 *       Device hardware remains initialized.
 *       No memory free for IoTBackendSession.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_bluetooth_iot_session(IotDevice *dev, IoTBackendSession *sess);

/**
 * @brief Internal cleanup handler for CAN IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): CAN IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for CAN IoT session cleanup only.
 *       Resets session context, state and bus communication flags.
 *       Device hardware remains initialized.
 *       No memory free for IoTBackendSession.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_can_iot_session(IotDevice *dev, IoTBackendSession *sess);

/**
 * @brief Internal cleanup handler for ZigBee IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): ZigBee IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for ZigBee IoT session cleanup only.
 *       Resets session context, state and network flags.
 *       Device hardware remains initialized.
 *       No memory free for IoTBackendSession.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_zigbee_iot_session(IotDevice *dev, IoTBackendSession *sess);

/**
 * @brief Internal cleanup handler for LoRa IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): LoRa IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for LoRa IoT session cleanup only.
 *       Resets session context, state and RF communication flags.
 *       Device hardware remains initialized.
 *       No memory free for IoTBackendSession.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_lora_iot_session(IotDevice *dev, IoTBackendSession *sess);

/**
 * @brief Internal cleanup handler for PowerLink IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): PowerLink IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for PowerLink IoT session cleanup only.
 *       Resets real-time session context, state and sync flags.
 *       Device hardware remains initialized.
 *       No memory free for IoTBackendSession.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_powerlink_iot_session(IotDevice *dev, IoTBackendSession *sess);

#endif