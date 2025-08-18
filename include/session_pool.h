#ifndef SESSION_POOL_H
#define SESSION_POOL_H

#include <stdint.h> 
#include <sys/queue.h>

#include "session.h"

struct  ControlMsg;
struct BackendSessionPoolOps;

struct BackendSessionPool {
    char* pool_name;
    int sess_num;                       //pool当前容量
    int capacity;                       //pool总容量
    struct BackendSessionPoolOps* ops;
    struct BackendSessionQueue act_queue;     // 活动会话队列
    struct BackendSessionIDQueue id_queue;    // 会话ID队列
    struct BackendSession* htable;  // Session hashtable
} ;

struct BackendSessionPoolOps {
    int (*create_sess)(struct BackendSessionPool* s_pool, struct BackendSession* sess);
    int (*insert_sess)(struct BackendSessionPool* s_pool, struct BackendSession* sess);

    struct BackendSession* (*search_sess)(struct BackendSessionPool* s_pool, uint16_t id);

    int (*delete_sess)(struct BackendSessionPool* s_pool, struct BackendSession* sess);

    int (*data_process)(struct BackendSession* sess, uint8_t* in, 
        uint32_t in_size, uint8_t* out, uint32_t* out_size);

    void (*destroy_pool)(struct BackendSessionPool* s_pool);
};

struct BackendSessionPool* high_speed_pool;
int high_speed_init_pool(struct BackendSessionPool* pool); 

//helper func
uint16_t allocate_id(struct BackendSessionIDQueue* id_q);
void release_id(struct BackendSessionIDQueue* id_q, uint16_t id);
void print_pool(struct BackendSessionPool* s_pool);
void high_speed_delete_all_sess(struct BackendSessionPool* s_pool);
void fill_id_queue(struct BackendSessionIDQueue* id_q);
void inc_sess_num(struct BackendSessionPool* pool);
void dec_sess_num(struct BackendSessionPool* pool);
#endif