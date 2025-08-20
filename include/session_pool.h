#ifndef SESSION_POOL_H
#define SESSION_POOL_H

#include <stdint.h> 
#include <sys/queue.h>

#include "message.h"
#include "session.h"

struct BackendEngine_;

struct BackendSessionPoolOps;

struct BackendSessionPool {
    char                            *pool_name;
    int                             sess_num;       // Current capacity of the pool
    int                             capacity;       // Total capacity of the pool
    struct BackendEngine_           *engine;        // Points to the engine that this pool belongs to.
    struct BackendSessionPoolOps    *ops;           // Operation function set.
    struct BackendSessionQueue      act_queue;      // Active session queue
    struct BackendSessionIDQueue    id_queue;       // Session ID queue
    struct BackendSession           *htable;        // Session hash table
};

struct BackendSessionPoolOps {
    int (*create_sess)(struct BackendSessionPool *s_pool, struct BackendSession **sess,  struct SessMsgPara *para);
    int (*insert_sess)(struct BackendSessionPool *s_pool, struct BackendSession *sess);

    struct BackendSession* (*search_sess)(struct BackendSessionPool *s_pool, uint16_t id);

    int (*delete_sess)(struct BackendSessionPool *s_pool, struct BackendSession *sess);

    int (*data_process)(struct BackendSession *sess, uint8_t *in, 
        uint32_t in_size, uint8_t *out, uint32_t *out_size);

    void (*destroy_pool)(struct BackendSessionPool *s_pool);
};

struct BackendSessionPool  *high_speed_pool;
int high_speed_init_pool(struct BackendSessionPool *pool); 

struct BackendSessionPool *get_backend_high_speed_pool();

//helper func
uint16_t allocate_id(struct BackendSessionIDQueue *id_q);
void release_id(struct BackendSessionIDQueue *id_q, uint16_t id);
void print_pool(struct BackendSessionPool *s_pool);
void high_speed_delete_all_sess(struct BackendSessionPool *s_pool);
void fill_id_queue(struct BackendSessionIDQueue *id_q);
void inc_sess_num(struct BackendSessionPool *pool);
void dec_sess_num(struct BackendSessionPool *pool);
#endif