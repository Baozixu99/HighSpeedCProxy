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



/**
 * @brief FIFO queue (ring buffer implementation) based on SharedMemoryPool
 * 
 * A high-efficiency first-in-first-out (FIFO) queue implemented with a ring buffer structure,
 * which allocates memory from an associated shared memory pool. It is designed for inter-process
 * or inter-thread communication, with core parameters reflecting memory size and element quantity:
 * - `capacity` represents total allocated memory size for the queue
 * - `max_num_items` represents maximum number of elements (calculated as capacity/block_size)
 * - Ring buffer indexes (`header`/`tail`) manage element enqueue/dequeue efficiently
 */
struct SharedMemoryPoolQueue {
    /* Pointer to the associated shared memory pool. All memory for the queue
       (including control block and data blocks) is allocated from this pool. */
    struct SharedMemoryPool *pool;

    /* Physical address of the queue control block in shared memory.
       Used for direct access across processes or between kernel and user spaces. */
    uint64_t                phy_addr;

    /* Virtual address of the queue control block in the current process.
       Used for normal access to the queue's control structure. */
    uint64_t                virt_addr;

    /* Head index of the ring buffer, pointing to the next element to be dequeued.
       Works with `tail` to maintain FIFO order. */
    uint16_t                header;

    /* Tail index of the ring buffer, pointing to the next free slot for enqueuing.
       Works with `header` to maintain FIFO order. */
    uint16_t                tail;

    /* Current number of elements in the queue. Dynamically updated with
       enqueue (increment) and dequeue (decrement) operations. */
    size_t                  length;

    /* Total memory size (in bytes) allocated to this queue from the shared memory pool.
       Determines the maximum storage space available for elements. */
    size_t                  capacity;

    /* Maximum number of elements the queue can hold. Calculated as (capacity / block_size),
       representing the upper limit of elements based on allocated memory and element size. */
    size_t                  max_num_items;

    /* Remaining memory size (in bytes) that can be used for enqueuing elements. Calculated as 
       (capacity - length * block_size), indicating available memory in the queue. */
    size_t                  surplus;

    /* Size (in bytes) of each element's memory block. All elements in the queue
       occupy a fixed size to simplify memory management and access. */
    size_t                  block_size;
};


/**
 * @brief Configuration parameters structure for initializing SharedMemoryPoolQueue
 * 
 * A structure containing all necessary parameters required to initialize a SharedMemoryPoolQueue
 * instance. It aggregates external input parameters that define the queue's memory properties
 * and association with a shared memory pool, serving as a clean interface for queue initialization.
 * 
 * Key parameters include:
 * - `pool`: Reference to the associated shared memory pool (mandatory)
 * - `phy_addr`/`virt_addr`: Addresses of the control block in shared memory and current process
 * - `capacity`: Total memory size allocated to the queue from the pool
 * - `block_size`: Fixed size of each element's memory block (determines max element count)
 */
typedef struct SharedMemoryPoolQueueConfig_{
    struct SharedMemoryPool *pool;        // Associated shared memory pool
    uint64_t                phy_addr;     // Physical address of control block in shared memory
    uint64_t                virt_addr;    // Virtual address of control block in current process
    size_t                  capacity;     // Total memory size (bytes) allocated to the queue
    size_t                  block_size;   // Size (bytes) of each element's memory block
} SharedMemoryPoolQueueConfig;


/**
 * @brief FIFO queue ENQUEUE operation macro (standard C compatible version)
 * 
 * This macro implements the PUSH operation for SharedMemoryPoolQueue. It uses a do...while structure 
 * to ensure syntax compatibility and returns the operation result through the second parameter. 
 * The specific logic is as follows:
 * 1. Increment the header index; if it exceeds max_num_items, wrap around (circular nature)
 * 2. Check if the operation triggers a queue exception (header equals tail or out of bounds)
 * 3. If abnormal, roll back the header and set an error status; otherwise, set a success status
 * 
 * @param queue Pointer to the SharedMemoryPoolQueue structure (input)
 * @param result Variable to receive the operation result (output), which can be:
 *               - BACKEND_PROXY_PROCESS_OK: Operation succeeded
 *               - BACKEND_PROXY_PROCESS_ERROR: Operation failed (queue is full or abnormal)
 */
#define SHMP_QUEUE_ENQUEUE(queue, result) do { \
    /* Initialize result to success status */ \
    (result) = BACKEND_PROXY_PROCESS_OK; \
    /* Save original header for rollback in case of exception */ \
    uint16_t original_header = (queue)->header; \
    \
    /* Update header: increment by 1, wrap around if exceeding max_num_items */ \
    (queue)->header++; \
    if ((queue)->header >= (queue)->max_num_items) { \
        (queue)->header -= (queue)->max_num_items; \
    } \
    \
    /* Check for abnormal status: header conflicts with tail or out of bounds */ \
    if ((queue)->header == (queue)->tail || (queue)->header >= (queue)->max_num_items) { \
        /* Roll back header to pre-operation state */ \
        (queue)->header = original_header; \
        /* Set error result */ \
        (result) = BACKEND_PROXY_PROCESS_ERROR; \
    } \
} while(0)


/**
 * @brief FIFO-queue DEQUEUE operation macro (standard C compatible version)
 * 
 * This macro implements the DEQUEUE operation for SharedMemoryPoolQueue with priority on empty queue check.
 * It first verifies if the queue is empty before modifying any indices. The logic is:
 * 1. Check if queue is empty (tail equals header) - return error immediately if true
 * 2. Save original tail for rollback in case of subsequent errors
 * 3. Increment the tail index; if exceeding max_num_items, wrap around (circular nature)
 * 4. Check for out-of-bounds error; rollback and return error if detected
 * 5. Return success status if all operations complete normally
 * 
 * @param queue Pointer to the SharedMemoryPoolQueue structure (input)
 * @param result Variable to receive the operation result (output), which can be:
 *               - BACKEND_PROXY_PROCESS_OK: Operation succeeded
 *               - BACKEND_PROXY_PROCESS_ERROR: Operation failed (queue is empty or abnormal)
 */
#define SHMP_QUEUE_DEQUEUE(queue, result) do { \
    /* First check if queue is empty (tail equals header) */ \
    if ((queue)->tail == (queue)->header) { \
        (result) = BACKEND_PROXY_PROCESS_ERROR; \
    } else { \
        /* Initialize result to success status */ \
        (result) = BACKEND_PROXY_PROCESS_OK; \
        /* Save original tail for rollback in case of exception */ \
        uint16_t original_tail = (queue)->tail; \
        \
        /* Update tail: increment by 1 */ \
        (queue)->tail++; \
        \
        /* Handle wrap-around if exceeding max_num_items */ \
        if ((queue)->tail >= (queue)->max_num_items) { \
            (queue)->tail -= (queue)->max_num_items; \
        } \
        \
        /* Check for out-of-bounds error */ \
        if ((queue)->tail >= (queue)->max_num_items) { \
            /* Roll back tail to pre-operation state */ \
            (queue)->tail = original_tail; \
            (result) = BACKEND_PROXY_PROCESS_ERROR; \
        } \
    } \
} while(0)


/**
 * @brief Macro to get the virtual address of the element pointed by header in the shared memory queue
 * 
 * Calculates the virtual address of the queue element that the header index points to.
 * The address is derived from the queue's base virtual address plus the offset calculated by
 * header index multiplied by block size.
 * 
 * @param queue Pointer to the SharedMemoryPoolQueue structure
 * @return uint64_t Virtual address of the element at header position
 */
#define SHMP_QUEUE_HEADER_VIRT_ADDR(queue) \
    ((uint64_t)(queue)->virt_addr + (uint64_t)(queue)->header * (uint64_t)(queue)->block_size)


/**
 * @brief Macro to get the virtual address of the position pointed by tail in the shared memory queue
 * 
 * Calculates the virtual address of the queue position that the tail index points to (next available slot for enqueuing).
 * The address is derived from the queue's base virtual address plus the offset calculated by
 * tail index multiplied by block size.
 * 
 * @param queue Pointer to the SharedMemoryPoolQueue structure
 * @return uint64_t Virtual address of the position at tail index
 */
#define SHMP_QUEUE_TAIL_VIRT_ADDR(queue) \
    ((uint64_t)(queue)->virt_addr + (uint64_t)(queue)->tail * (uint64_t)(queue)->block_size)


/**
 * @brief Macro to calculate used memory size using header and tail indices
 * 
 * Computes the total memory occupied by elements in the queue using header and tail indices,
 * without relying on the length field. Follows circular queue logic:
 * - When tail >= header: used elements = tail - header
 * - When tail < header: used elements = (max_num_items - header) + tail
 * Total used memory is then elements count multiplied by block size.
 * 
 * @param queue Pointer to the SharedMemoryPoolQueue structure
 * @return size_t Total used memory size in bytes
 */
#define SHMP_QUEUE_USED_MEMORY(queue) \
    ((size_t)( \
        ((queue)->tail >= (queue)->header) ? \
        ((queue)->tail - (queue)->header) : \
        ((queue)->max_num_items - (queue)->header + (queue)->tail) \
    ) * (size_t)(queue)->block_size)


/**
 * @brief Macro to calculate surplus (available) memory in the shared memory queue
 * 
 * Implements a two-step internal calculation:
 * 1. First compute the number of available slots using header, tail and max capacity:
 *    - When tail >= header: surplus slots = max_num_items - (tail - header)
 *    - When tail < header: surplus slots = header - tail
 * 2. Then multiply available slots by block size to get surplus memory in bytes
 * 
 * @param queue Pointer to the SharedMemoryPoolQueue structure
 * @return size_t Surplus memory size in bytes
 */
#define SHMP_QUEUE_SURPLUS_MEMORY(queue) \
    ((size_t)( \
        /* Step 1: Calculate number of surplus slots */ \
        ((queue)->tail >= (queue)->header) ? \
        ((queue)->max_num_items - ((queue)->tail - (queue)->header)) : \
        ((queue)->header - (queue)->tail) \
        /* Step 2: Convert slots to memory size */ \
    ) * (size_t)(queue)->block_size)


/**
 * @brief Allocate a memory slot from the queue head
 * @param queue Pointer to the SharedMemoryPoolQueue structure
 * @param addr_ptr Pointer to store the virtual address of allocated slot (output)
 * @return Returns BACKEND_PROXY_PROCESS_OK on success, BACKEND_PROXY_PROCESS_ERROR on failure
 * @note Allocation logic: 
 *       1. Check if queue is full (next header == tail)
 *       2. If not full, store current header's slot address in addr_ptr
 *       3. Update header with wrap-around handling (mod max_num_items)
 */
#define SHM_POOL_QUEUE_HEAD_ALLOC(queue, addr_ptr) ({ \
    int _status = BACKEND_PROXY_PROCESS_ERROR; \
    if ((queue != NULL) && (addr_ptr != NULL)) { \
        uint16_t _next_header = (queue->header + 1) % (uint16_t)queue->max_num_items; \
        if (_next_header != queue->tail) { \
            *addr_ptr = (uintptr_t)(queue->virt_addr + queue->header * queue->block_size); \
            queue->header = _next_header; \
            _status = BACKEND_PROXY_PROCESS_OK; \
        } else { \
            *addr_ptr = 0; \
        } \
    } \
    _status; \
})



/**
 * @brief Roll back the head position (for reverting when allocation fails)
 * @param queue Pointer to the SharedMemoryPoolQueue structure
 * @note Rollback logic: Restore header to previous position with wrap-around handling
 *       (undoes the header increment from a failed allocation attempt)
 */
#define SHM_POOL_QUEUE_HEAD_ROLLBACK(queue) do { \
    if (queue != NULL) { \
        queue->header = (queue->header == 0) ? \
            (uint16_t)(queue->max_num_items - 1) : \
            (queue->header - 1); \
    } \
} while (0)


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

int shared_mem_pool_queue_initialize(struct SharedMemoryPoolQueue *queue, const SharedMemoryPoolQueueConfig *config);

int shared_mem_pool_queue_destroy(struct SharedMemoryPoolQueue* queue);



int shared_mem_pool_queue_send(struct SharedMemoryPoolQueue *queue, 
                               const void *data, 
                               size_t data_size);

/**
 * @brief Sends a variable-size block (up to block_size) to the SharedMemoryPoolQueue: One Copy
 * 
 * Stores a variable-length data block (with size ≤ block_size) into the queue using one-copy semantics.
 * The data is transferred to a shared memory slot through a single copy operation. Regardless of the 
 * actual data size, the entire block_size space in shared memory is occupied to maintain fixed-size 
 * slot management. Explicitly handles full queue cases.
 * 
 * @param queue        Pointer to the SharedMemoryPoolQueue instance
 * @param data         Input pointer to the variable-length data block to be sent. Must point to valid memory 
 *                     containing the data (not a pointer to a pointer).
 * @param data_size    Size of the input data block (must be > 0 and ≤ queue->block_size for valid operation)
 * 
 * @return int Returns:
 *             - BACKEND_PROXY_PROCESS_OK: Success, data was copied to shared memory and queue state updated
 *             - BACKEND_PROXY_PROCESS_AGAIN: Queue is full (next tail position equals header)
 *             - BACKEND_PROXY_PROCESS_ERROR: Invalid input parameters (NULL pointers for queue or data), 
 *                                           invalid block_size (0 bytes), data_size = 0, or data_size > queue->block_size
 * 
 * @note The input data block (pointed to by data) must remain valid until the copy operation completes. 
 *       One-copy semantics involve a single data transfer (e.g., via memcpy) from the input buffer to the 
 *       target shared memory slot. 
 *       Critical behavior: Even if data_size < block_size, the entire block_size space in shared memory is 
 *       reserved and counted as occupied (surplus decreases by block_size, not data_size).
 *       Queue state (tail, length, surplus) is updated via the SHMP_QUEUE_ENQUEUE macro after the copy completes.
 *       Address calculation: Target slot address is derived from queue->virt_addr + (tail * block_size).
 */
int shared_mem_pool_queue_send_oc(struct SharedMemoryPoolQueue *queue, 
                                  const void *data, 
                                  size_t data_size);


int shared_mem_pool_queue_recv_zc(struct SharedMemoryPoolQueue *queue,
                              void **buffer,
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