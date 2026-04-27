#ifndef SESSION_POOL_H
#define SESSION_POOL_H

#include <stdint.h> 
#include <sys/queue.h>

#include "message.h"
#include "session.h"

struct BackendEngine_;

struct BackendSessionPoolOps;

struct BackendSessionPool {
    char                            *pool_name;     // Name/identifier of the session pool
    int                             sess_num;       // Current number of sessions in the pool
    int                             capacity;       // Maximum number of sessions the pool can hold (total capacity)
    struct BackendEngine_           *engine;        // Pointer to the engine this pool belongs to
    struct BackendSessionPoolOps    *ops;           // Set of operation functions for the pool (e.g., create, delete)
    struct BackendSessionQueue      queue_f2b;      // Queue containing active sessions
    struct BackendSessionQueue      queue_b2f;      // Queue for backend-to-frontend session communication/mapping
    struct BackendSessionIDQueue    id_queue;       // Queue holding available/reusable session IDs
    struct BackendSession           *htable;        // Hash table storing sessions (for efficient lookup by ID)
};




struct BackendSessionPoolOps {
    int (*create_sess)(struct BackendSessionPool *s_pool, struct BackendSession **sess,  struct SessMsgPara *para);
    int (*create_sess_active)(struct BackendSessionPool *s_pool, struct BackendSession **sess,  struct SessMsgPara *para);
    int (*create_sess_passive)(struct BackendSessionPool *s_pool, struct BackendSession **sess, PassiveSessParaIP *para);
    int (*insert_sess)(struct BackendSessionPool *s_pool, struct BackendSession *sess);

    struct BackendSession* (*search_sess)(struct BackendSessionPool *s_pool, uint16_t id);

    int (*delete_sess)(struct BackendSessionPool *s_pool, struct BackendSession *sess);

    int (*data_process)(struct BackendSession *sess);

    int (*data_process_f2b)(struct BackendSession *sess);

    int (*data_process_b2f)(struct BackendSession *sess);

    int (*data_process_nns)(struct BackendSession *sess);

    void (*destroy_pool)(struct BackendSessionPool *s_pool);
};

extern struct BackendSessionPool *high_speed_pool;
int high_speed_init_pool(struct BackendSessionPool *pool); 
void high_speed_deinit_pool(struct BackendSessionPool *pool);

struct BackendSessionPool *get_backend_high_speed_pool();

//helper func
uint16_t allocate_id(struct BackendSessionIDQueue *id_q);
void release_id(struct BackendSessionIDQueue *id_q, uint16_t id);
void print_pool(struct BackendSessionPool *s_pool);
void high_speed_delete_all_sess(struct BackendSessionPool *s_pool);
void fill_id_queue(struct BackendSessionIDQueue *id_q);
void inc_sess_num(struct BackendSessionPool *pool);
void dec_sess_num(struct BackendSessionPool *pool);


int high_speed_data_process_f2b(struct BackendSession *sess);
int high_speed_data_process_b2f(struct BackendSession *sess);
int high_speed_data_process_nns(struct BackendSession *sess);

int __high_speed_create_sess_fastpath(struct BackendSessionPool *s_pool, struct BackendSession **sess, uint16_t new_sess_id, struct SessMsgPara *para);

#endif