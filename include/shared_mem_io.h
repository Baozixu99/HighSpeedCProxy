#ifndef SHARED_MEM_IO_H
#define SHARED_MEM_IO_H

#include <stdint.h>
#include <unistd.h>

#define ERROR_SHARED_MEM_ADDR           UINT64_MAX



struct SharedMemoryPoolLock{
    int value; // Just for placehoder. We will redefine this struct after receiving the partner's document.
};

struct SharedMemoryPool{
    int                             value; // Just for placehoder. We will redefine this struct after receiving the partner's document.
    struct SharedMemoryPoolLock     lock;
};

struct DequeNode{
    int                             value; // // Just for placehoder. We will redefine this struct after receiving the partner's document.
    struct DequeNode                *prev;
    struct DequeNode                *next;
};


/*
 * Double-ended queue implemented based on SharedMemoryPool, 
 */
struct SharedMemoryPoolQueue {
    struct SharedMemoryPool *pool;       // Associated shared memory pool
    // Pointers to core nodes of the deque (head and tail)
    struct DequeNode        *head;
    struct DequeNode        *tail;
    // Other properties
    size_t                  length;      // Queue length
    size_t                  capacity;    // Capacity of the queue
    size_t                  block_size;  // Size of each memory block in the queue
};


int init_shared_mem_pool(struct SharedMemoryPool *mem_pool);
void free_shared_mem_pool(struct SharedMemoryPool *mem_pool);
uint64_t alloc_shared_mem(struct SharedMemoryPool *mem_pool);
void free_shared_mem(struct SharedMemoryPool *mem_pool, uint64_t addr);

int init_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);
void free_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);
int fetch_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);
int release_shared_mem_pool_lock(struct SharedMemoryPoolLock *mem_pool);



struct SharedMemoryPoolQueue *shared_mem_pool_queue_create(struct SharedMemoryPool* pool,
                                                           size_t max_elements,
                                                           size_t element_size);

int shared_mem_pool_queue_initialize(struct SharedMemoryPoolQueue *queue);

int shared_mem_pool_queue_destroy(struct SharedMemoryPoolQueue* queue);



int shared_mem_pool_queue_send(struct SharedMemoryPoolQueue *queue, 
                               const void **data, 
                               size_t data_size);


int shared_mem_pool_queue_recv(struct SharedMemoryPoolQueue *queue, 
                               void **buffer, 
                               size_t buffer_size, 
                               size_t *out_data_size);


/**
 * @brief Acquire access right to the shared memory queue by obtaining the shared memory pool lock
 * @param queue Pointer to the SharedMemoryPoolQueue instance to be accessed
 * @return BACKEND_PROXY_PROCESS_OK if lock is acquired successfully;
 *         BACKEND_PROXY_PROCESS_ERROR if a system-level error occurs (e.g., invalid pool handle);
 *         BACKEND_PROXY_PROCESS_AGAIN if lock acquisition times out (retry may succeed)
 * @note Retrieves the lock associated with the shared memory pool (queue->pool) to control queue access;
 *       Must be paired with SHARED_MEM_QUEUE_UNLOCK using the same queue to prevent deadlocks;
 *       BACKEND_PROXY_PROCESS_AGAIN indicates temporary unavailability - callers should retry later
 */
#define SHARED_MEM_QUEUE_LOCK(queue)  (fetch_shared_mem_pool_lock((queue)->pool))

/**
 * @brief Release access right to the shared memory queue by releasing the shared memory pool lock
 * @param queue Pointer to the SharedMemoryPoolQueue instance that was accessed
 * @return BACKEND_PROXY_PROCESS_OK if lock is released successfully;
 *         BACKEND_PROXY_PROCESS_ERROR if a system-level error occurs (e.g., releasing an unheld lock)
 * @note Releases the lock associated with the shared memory pool (queue->pool) to end controlled access;
 *       Must be paired with SHARED_MEM_QUEUE_LOCK using the same queue to prevent deadlocks;
 *       Does not return BACKEND_PROXY_PROCESS_AGAIN - release operation either succeeds or fails
 */
#define SHARED_MEM_QUEUE_UNLOCK(queue)  (release_shared_mem_pool_lock((queue)->pool))

#endif