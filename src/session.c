
#include "session.h"
#include "common_utils.h"


/**
 * @brief Allocates and initializes a SessMsgSeg structure
 * 
 * @param len        Length of the data buffer (in bytes)
 * @param type   Memory source type of the data buffer (dynamic allocation or shared memory)
 * @param shared_data Pointer to shared memory data (valid only when data_src is SESS_MSG_SEG_SHARED_MEM)
 * @param mem_pool Pointer to the SharedMemoryPool instance (valid only when type is SESS_MSG_SEG_SHARED_MEM). 
                   Used to associate the SessMsgSeg with the shared memory pool for management (e.g., validation, release tracking).
 * 
 * @return Pointer to the newly allocated SessMsgSeg on success; NULL on failure
 * 
 * @note - If data_src is SESS_MSG_SEG_DYNAMIC_ALLOC: allocates data buffer with malloc()
 *       - If data_src is SESS_MSG_SEG_SHARED_MEM: uses shared_data directly (does not allocate new memory)
 *       - Initializes TAILQ entry to default state
 */
struct SessMsgSeg *sess_msg_seg_alloc(size_t len, SessMsgSegType type, const uint8_t *shared_data, struct SharedMemoryPool *mem_pool){
    struct SessMsgSeg *msg_seg;
    msg_seg = malloc(sizeof(struct SessMsgSeg));

    if(NULL == msg_seg){
        error_print("sess_msg_seg_alloc failed: insurficient memory resource for message segment!");
        return NULL;
    }

    switch(type) {
        case SESS_MSG_SEG_DYNAMIC_ALLOC:
            msg_seg->data = malloc(len);

            if(NULL == msg_seg->data){
                error_print("sess_msg_seg_alloc failed: insurficient memory resource for message data!");
                goto msg_alloc_error;
            }
            break;
        case SESS_MSG_SEG_SHARED_MEM:
/*
 * The memory for storing data is prealloc from the memory pool.
 */
            if(NULL == mem_pool){
                error_print("sess_msg_seg_alloc failed: Shared memory pool (mem_pool) cannot be NULL when using SESS_MSG_SEG_SHARED_MEM type!");
                goto msg_alloc_error;
            }

            msg_seg->data       = (uint8_t *)shared_data;
            msg_seg->mem_pool   = mem_pool;
            break;

        default:
/*
 * Unsupported message segment type.
 */
            error_print("sess_msg_seg_alloc failed: unsupported message segment type!");
            goto msg_alloc_error;
    }

    msg_seg->type = type;
    msg_seg->len  = len;

    return msg_seg;
msg_alloc_error:

    return NULL;
}

/**
 * @brief Releases a SessMsgSeg structure and its associated resources
 * 
 * @param seg_ptr Double pointer to the SessMsgSeg to be released (will be set to NULL after release)
 * 
 * @note - If type is SESS_MSG_SEG_DYNAMIC_ALLOC: frees the data buffer with free()
 *       - If type is SESS_MSG_SEG_SHARED_MEM: does not free data (managed by shared memory system)
 *       - Safely handles NULL input (no operation performed)
 */
void sess_msg_seg_free(struct SessMsgSeg *seg_ptr){
    struct SessMsgSeg *msg_seg;

    if(NULL == seg_ptr){
        error_print("sess_msg_seg_free failed: input pointer is invalid!");
        return;
    }

    msg_seg = seg_ptr;


    switch(msg_seg->type) {
        case SESS_MSG_SEG_DYNAMIC_ALLOC:
            if(NULL == msg_seg->data){
                error_print("sess_msg_seg_free failed: data pointer is NULL!");
            }
            free(msg_seg->data);
            break;
        case SESS_MSG_SEG_SHARED_MEM:
/*
 * The memory belongs to the shared memory pool.
 */
            break;

        default:
/*
 * Unsupported message segment type.
 */
            error_print("sess_msg_seg_free failed: unsupported message segment type!");
    }

    free(msg_seg);
}


/**
 * @brief Release all SessMsgSeg elements in the SessMsgQueue
 * 
 * This function traverses the SessMsgQueue, releases each SessMsgSeg element
 * according to its memory type, and finally clears the queue.
 * 
 * For dynamic allocation type (SESS_MSG_SEG_DYNAMIC_ALLOC):
 * - Free the data buffer allocated by malloc()
 * - Free the SessMsgSeg structure itself
 * 
 * For shared memory type (SESS_MSG_SEG_SHARED_MEM):
 * - Do not free the shared data buffer (managed by SharedMemoryPool)
 * - Only free the SessMsgSeg structure itself
 * 
 * @param queue Pointer to the SessMsgQueue to be cleared
 */
void sess_msg_queue_free_all(struct SessMsgQueue *queue) {
    if (queue == NULL) {
        return; // Avoid null pointer operation
    }

    struct SessMsgSeg *seg, *next_seg;


    TAILQ_FOREACH_SAFE(seg, queue, entry, next_seg) {
        /* 1. Remove the segment from the queue */
        TAILQ_REMOVE(queue, seg, entry);

        /* 2. Deallocate memory based on segment type */
        if (seg->type == SESS_MSG_SEG_DYNAMIC_ALLOC) {
            // Free dynamically allocated data buffer
            free(seg->data);
        } else if (seg->type == SESS_MSG_SEG_SHARED_MEM) {

            // if (current_seg->mem_pool) {
            //     shared_memory_pool_release(current_seg->mem_pool, current_seg->data);
            // }
        }
        /* 3. Free the segment structure itself */
        free(seg);

    }// TAILQ_FOREACH_SAFE


#if 0
    TAILQ_FOREACH(seg, queue, entry){
        // Manually save the next node before releasing current node
        next_seg = TAILQ_NEXT(seg, entry);

        // Remove current segment from the queue
        TAILQ_REMOVE(queue, seg, entry);

        // Free resources based on memory type
        if (SESS_MSG_SEG_DYNAMIC_ALLOC == seg->type) {
            // Free dynamically allocated data buffer
            if (NULL != seg->data) {
                free(seg->data);
                seg->data = NULL;
            }
        }

        if (SESS_MSG_SEG_SHARED_MEM == seg->type) {
            // Free dynamically allocated data buffer
            if (NULL != seg->data || NULL != seg->mem_pool) {
                free_shared_mem(seg->mem_pool, (uint64_t)seg->data);
            }
        }


        // Shared memory data is managed by SharedMemoryPool, no need to free here

        // Free the SessMsgSeg structure itself
        free(seg);

        // Move to next node (since current node is freed)
        seg = next_seg;
    }
#endif

    TAILQ_INIT(queue);
}


int session_send(struct BackendSession* sess, const uint8_t* data, uint32_t size)
{
    return 0;
}


int session_recv(struct BackendSession* sess, uint8_t* data, uint32_t size)
{
    return 0;
}


void delete_session(struct BackendSession* sess)
{

}