#ifndef ENGINE_H
#define ENGINE_H

#include "dev.h"
#include "session_pool.h"


struct SharedMemoryPool;
struct SharedMemoryPoolLock;


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

void engine_init();
void engine_run();
#endif