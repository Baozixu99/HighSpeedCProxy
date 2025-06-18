#ifndef SESSION_POOL_H
#define SESSION_POOL_H

#include <stdint.h> 
#include <sys/queue.h>

#include "session.h"

struct  ControlMsg;
struct BackendSessPoolOps;

struct BackendSessionPool {
    char* pool_name;
    int sess_num;                       //pool当前容量
    int capacity;                       //pool总容量
    struct BackendSessPoolOps* ops;
    struct BackendSessionQueue* act_queue;     // 活动会话队列
    struct BackendSessionIDQueue* id_queue;    // 会话ID队列
    struct BackendSession* htable;  // Session hashtable
} ;

struct BackendSessPoolOps {

    struct BackendSession* (*create)(int dev_id, struct ControlMsg* cmsg);

    int (*init_pool)(struct BackendSession* sess); 

    int (*insert_sess)(struct BackendSessionPool* s_pool, struct BackendSession* sess);

    struct BackendSession* (*search_sess)(struct BackendSessionPool* s_pool, uint16_t id);

    int (*delete_sess)(struct BackendSessionPool* s_pool, struct BackendSession* sess);

    int (*data_process)(struct BackendSession* sess, uint8_t* in, 
        uint32_t in_size, uint8_t* out, uint32_t* out_size);

    void (*destroy_pool)(struct BackendSession* sess);
};
#endif