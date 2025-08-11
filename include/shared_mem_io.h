#ifndef SHARED_MEM_IO_H
#define SHARED_MEM_IO_H

#include <stdint.h>


struct SharedMemoryPool{
    int value; // Just for placehoder. We will redefine this struct after receiving the partner's document.
};


struct SharedMemoryPoolLock{
    int value; // Just for placehoder. We will redefine this struct after receiving the partner's document.
};

int init_shared_mem_pool(struct SharedMemoryPool *mem_pool);
void free_shared_mem_pool(struct SharedMemoryPool *mem_pool);
uint64_t alloc_shared_mem(struct SharedMemoryPool *mem_pool);
void free_shared_mem(struct SharedMemoryPool *mem_pool, uint64_t addr);

int init_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);
void free_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);
int fetch_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);
int release_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);

#endif