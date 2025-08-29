#include "shared_mem_io.h"
#include "backend_proto.h"

int init_shared_mem_pool(struct SharedMemoryPool *mem_pool){
    return BACKEND_PROXY_PROCESS_OK;
}


void free_shared_mem_pool(struct SharedMemoryPool *mem_pool){

}


uint64_t alloc_shared_mem(struct SharedMemoryPool *mem_pool){
    if(NULL == mem_pool)
        return ERROR_SHARED_MEM_ADDR;

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


/**
 * Create and initialize a shared memory pool queue.
 * 
 * @param pool Pointer to the underlying SharedMemoryPool to allocate deque nodes from.
 * @param max_elements Maximum number of elements the deque can hold (0 for unlimited, if supported).
 * @param element_size Size of each element in the deque (bytes). Use 0 for variable-size elements.
 * @return Pointer to the newly created SharedMemoryPoolQueue on success; NULL on failure 
 *         (e.g., invalid pool/lock, insufficient memory, invalid parameters).
 */
struct SharedMemoryPoolQueue *shared_mem_pool_queue_create(struct SharedMemoryPool* pool,
                                                           size_t max_elements,
                                                           size_t element_size){
    struct SharedMemoryPoolQueue *queue;

    queue = malloc(sizeof(struct SharedMemoryPoolQueue));

    if(NULL == queue){
        error_print("SharedMemoryPoolDequeCreate failed: failed to allocate memory for SharedMemoryPoolDeque instance");
        return NULL;
    }

    queue->pool         = pool;
    queue->length       = 0;
    queue->capacity     = max_elements;
    queue->block_size   = element_size;

    return queue;
}


/**
 * Initialize a pre-allocated SharedMemoryPoolQueue instance.
 * 
 * This function configures a pre-allocated deque structure, binding it to a shared memory pool
 * and lock, and setting up initial properties (capacity, element size, etc.). It does not 
 * allocate memory for the deque itself (unlike SharedMemoryPoolDeque_Create), but initializes
 * its internal state.
 * 
 * @param queue Pointer to the pre-allocated SharedMemoryPoolDeque instance to initialize. 
 *              Must not be NULL (caller is responsible for memory allocation).

 * 
 * @return BACKEND_PROXY_PROCESS_OK on success;  
 *         BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int shared_mem_pool_queue_initialize(struct SharedMemoryPoolQueue *queue){
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * Destroy a shared memory pool queue and release associated resources.
 * 
 * @param deque Pointer to the SharedMemoryPoolDeque to destroy. Passing NULL is safe (no operation).
 * @return BACKEND_PROXY_PROCESS_OK on success;  
 *         BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int shared_mem_pool_queue_destroy(struct SharedMemoryPoolQueue* queue);


/**
 * Enqueue (send) data to the shared memory pool deque.
 * 
 * @param queue Pointer to the SharedMemoryPoolDeque instance.
 * @param data Double pointer to the data to be enqueued; points to a pointer that references the data 
 *             in shared memory (enables zero-copy: no data copy occurs, as the deque stores the pointer 
 *             to the existing shared memory location directly). The const qualifier ensures the data 
 *             itself is not modified during enqueue.
 * @param data_size Size of the data (in bytes) to be enqueued.
 * @return BACKEND_PROXY_PROCESS_OK on success;  
 *         BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int shared_mem_pool_queue_send(struct SharedMemoryPoolQueue *queue, 
                               const void **data, 
                               size_t data_size){
    return BACKEND_PROXY_PROCESS_OK;
}


/** 
 * Dequeue (receive) data from the shared memory pool deque.
 * 
 * @param queue Pointer to the SharedMemoryPoolDeque instance.
 * @param buffer Double pointer to a buffer; used to return a direct pointer to the dequeued data 
 *               in shared memory (enables zero-copy: no data copy occurs, as the pointer is directly 
 *               assigned to the memory location in the shared pool). The caller does not need to 
 *               pre-allocate a buffer; instead, *buffer will be set to point to the existing data
 * @param buffer_size Size of the buffer (in bytes) provided for receiving data.
 * @param out_data_size Pointer to a size_t variable to store the actual size of dequeued data.
 * @return BACKEND_PROXY_PROCESS_OK on success;  
 *         BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int shared_mem_pool_queue_recv(struct SharedMemoryPoolQueue *queue, 
                              void **buffer, 
                              size_t buffer_size, 
                              size_t *out_data_size){
/*
 * Zero-copy.
 */
    return BACKEND_PROXY_PROCESS_OK;
}
