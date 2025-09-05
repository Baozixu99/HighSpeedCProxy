
#include "session.h"
#include "shared_mem_io.h"
#include "common_utils.h"


/**
 * @brief Allocates and initializes a SessMsgSeg structure
 * 
 * @param len        Length of the data buffer (in bytes)
 * @param type   Memory source type of the data buffer (dynamic allocation or shared memory)
 * @param shared_data Pointer to shared memory data (valid only when data_src is SESS_MSG_SEG_SHARED_MEM)
 * 
 * @return Pointer to the newly allocated SessMsgSeg on success; NULL on failure
 * 
 * @note - If data_src is SESS_MSG_SEG_DYNAMIC_ALLOC: allocates data buffer with malloc()
 *       - If data_src is SESS_MSG_SEG_SHARED_MEM: uses shared_data directly (does not allocate new memory)
 *       - Initializes TAILQ entry to default state
 */
struct SessMsgSeg *sess_msg_seg_alloc(size_t len, SessMsgSegType type, const uint8_t *shared_data){
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
            msg_seg->data = shared_data;
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
void sess_msg_seg_free(struct SessMsgSeg **seg_ptr){
    struct SessMsgSeg *msg_seg;

    if(NULL == seg_ptr || NULL == *seg_ptr){
        error_print("sess_msg_seg_free failed: input pointer is invalid!");
        return;
    }

    msg_seg = *seg_ptr;


    switch(msg_seg->type) {
        case SESS_MSG_SEG_DYNAMIC_ALLOC:
            if(NULL == msg_seg->data){
                error_print("sess_msg_seg_free failed: data pointer is NULL!");
            }
            break;
        case SESS_MSG_SEG_SHARED_MEM:

            break;

        default:
/*
 * Unsupported message segment type.
 */
            error_print("sess_msg_seg_free failed: unsupported message segment type!");
    }

    free(msg_seg);
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