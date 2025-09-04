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

struct SessMsgSeg {
    uint16_t len;
    uint16_t type;
    uint8_t *data;
    TAILQ_ENTRY(SessMsgSeg) entry;
};

TAILQ_HEAD(SessMsgQueue, SessMsgSeg);

struct BackendProtocolProcess; 
struct BackendEngine_;

struct BackendSession {
    int sess_type;
    int sock_fd;
    int ip_version;
    uint16_t frontend_sess_id;
    uint16_t backend_sess_id; // hash key
// State machine states
    int state_f2b; // front-end to back-end state
    int state_b2a; // back-end to front-end state
// Message queues
    struct SessMsgQueue queue_f2b; // front-end to back-end message queue
    struct SessMsgQueue queue_b2a; // back-end to front-end message queue
// Queue link nodes
    TAILQ_ENTRY(BackendSession) entries_f2b; // front-end to back-end active queue node
    TAILQ_ENTRY(BackendSession) entries_active; // global active session queue node
// Protocol processing
    struct BackendProtocolProcess *protocol_process; // protocol processing module pointer
// Pointer to the backend engine associated with this session
    struct BackendEngine_ *eng;
// Private data pointer (used to store session-specific data)
    void *pri_data;
    struct channel *net_channel;
    UT_hash_handle hh;
};


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