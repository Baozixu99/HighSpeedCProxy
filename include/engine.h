#ifndef ENGINE_H
#define ENGINE_H

#include "dev.h"
#include "session_pool.h"
#include "backend_proto.h"
#include "shared_mem_io.h"


typedef struct {
    // 网络设备集合
    struct HighSpeedNetDeviceSet* dev_set;
    
    // 会话池
    struct BackendSessionPool* sess_pool;
    
    // 内存池
    struct SharedMemoryPool* mem_pool;
    
    // 内存池锁
    struct SharedMemoryPoolLock* mem_pool_lock;
} BackendEngine;


extern BackendEngine *p_g_bk_eng;


void engine_init();
void engine_run();

BackendEngine *get_global_backend_engine();

int engine_init_hs_net_dev(BackendEngine *eng);
int engine_init_sess_pool(BackendEngine *eng);
int engine_init_mem_pool(BackendEngine *eng);
int engine_init_mem_pool_lock(BackendEngine *eng);



#endif