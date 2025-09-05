#ifndef SESSION_H
#define SESSION_H

#include <stdint.h> 
#include <sys/queue.h>
#include "uthash.h"
#include "channel.h"

#define BACKEND_PROXY_PROCESS_OK               0
#define BACKEND_PROXY_PROCESS_ERROR            -1

struct ControlMsg{
    uint16_t dev_id;
};

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

struct SessMsgSeg {
    uint16_t len;
    uint16_t type;
    uint8_t *data;
    TAILQ_ENTRY(SessMsgSeg) entry;
};

TAILQ_HEAD(SessMsgQueue, SessMsgSeg);


/**
 * @brief Allocates and initializes a SessMsgSeg structure
 * 
 * @param len        Length of the data buffer (in bytes)
 * @param type   Memory source type of the data buffer (dynamic allocation or shared memory)
 * @param shared_data Pointer to shared memory data (valid only when data_src is SESS_MSG_SEG_SHARED_MEM)
 * 
 * @return Pointer to the newly allocated SessMsgSeg on success; NULL on failure
 * 
 * @note - If data_src is SESS_MSG_SEG_DYNAMIC_ALLOC: allocates data buffer with malloc()
 *       - If data_src is SESS_MSG_SEG_SHARED_MEM: uses shared_data directly (does not allocate new memory)
 *       - Initializes TAILQ entry to default state
 */
struct SessMsgSeg *sess_msg_seg_alloc(size_t len, SessMsgSegType type, const uint8_t *shared_data);

/**
 * @brief Releases a SessMsgSeg structure and its associated resources
 * 
 * @param seg_ptr Double pointer to the SessMsgSeg to be released (will be set to NULL after release)
 * 
 * @note - If data_src is SESS_MSG_SEG_DYNAMIC_ALLOC: frees the data buffer with free()
 *       - If data_src is SESS_MSG_SEG_SHARED_MEM: does not free data (managed by shared memory system)
 *       - Safely handles NULL input (no operation performed)
 */
void sess_msg_seg_free(struct SessMsgSeg **seg_ptr);


struct BackendProtocolProcess; 
struct BackendEngine_;

#define BACKEND_SESS_LINKED_TO_QUEUE            1
struct BackendSession {
    int sess_type;
    int sock_fd;
    int ip_version;
    uint16_t frontend_sess_id;
    uint16_t backend_sess_id; // hash key
/*
 * State machine states, indicating the linked status of the session in different directions
 * state_f2b: When its value is BACKEND_SESS_LINKED_TO_QUEUE, it means the entries_f2b node of the current session
 * is linked to the queue_f2b (belonging to BackendSessionPool in BackendEngine_)
 * state_b2f: When its value is BACKEND_SESS_LINKED_TO_QUEUE, it means the entries_b2f node of the current session
 * is linked to the queue_bf2 (belonging to BackendSessionPool in BackendEngine_)
 */
    uint8_t state_f2b; 
    uint8_t state_b2f; 
// Message queues
    struct SessMsgQueue msg_f2b; // front-end to back-end message queue
    struct SessMsgQueue msg_b2f; // back-end to front-end message queue
// Queue link nodes
    TAILQ_ENTRY(BackendSession) entries_f2b; // front-end to back-end active queue node
    TAILQ_ENTRY(BackendSession) entries_b2f; // back-end to front-end active queue node
// Protocol processing
    struct BackendProtocolProcess *protocol_process; // protocol processing module pointer
// Pointer to the backend engine associated with this session
    struct BackendEngine_ *eng;
// Private data pointer (used to store session-specific data)
    void *pri_data;
    NetChannel net_channel;
    UT_hash_handle hh;
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

int session_send(struct BackendSession* sess, const uint8_t* data, uint32_t size);
int session_recv(struct BackendSession* sess, uint8_t* data, uint32_t size);
void delete_session(struct BackendSession* sess);

#endif