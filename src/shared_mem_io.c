#include "shared_mem_io.h"
#include "backend_proto.h"

int init_shared_mem_pool(struct SharedMemoryPool *mem_pool){
    return BACKEND_PROXY_PROCESS_OK;
}


void free_shared_mem_pool(struct SharedMemoryPool *mem_pool){

}


uint64_t alloc_shared_mem(struct SharedMemoryPool *mem_pool){
    return 0;
}


void free_shared_mem(struct SharedMemoryPool *mem_pool, uint64_t addr){
    
}


int init_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool){
    return BACKEND_PROXY_PROCESS_OK;
}



void free_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool){

}



int fetch_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool){
    return BACKEND_PROXY_PROCESS_OK;
}


int release_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool){
    return BACKEND_PROXY_PROCESS_OK;
}