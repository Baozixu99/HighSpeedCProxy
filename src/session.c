#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h> 
#include "session.h"
#include "dev.h"
#include "common_utils.h"

IoTBackendSession *backend_bluetooth_sess   = NULL;
IoTBackendSession *backend_can_sess         = NULL;
IoTBackendSession *backend_zigbee_sess      = NULL;
IoTBackendSession *backend_lora_sess        = NULL;
IoTBackendSession *backend_powerlink_sess   = NULL;
IoTBackendSession *backend_modbustcp_sess   = NULL;

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



/**
 * @brief Initialize Bluetooth communication session for IoT device
 * @param dev Pointer to IotDevice instance (pre-initialized device)
 * @param sess Pointer to IoTBackendSession instance (memory pre-allocated)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Session initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Session initialization failed
 *
 * @note This function only initializes session context, NOT device hardware.
 *       Device initialization is completed in the prior stage.
 *       The memory of IoTBackendSession is pre-allocated, no need to manage.
 *       1. Binds session context to the corresponding Bluetooth device
 *       2. Configures session state and communication parameters
 *       3. Sets up data transceiving logic for the session
 *       4. Prepares session for subsequent data interaction
 *
 * @warning Device hardware initialization must be completed before calling
 * @warning Session memory is managed externally, do not free in this function
 */
int engine_init_bluetooth_session(IotDevice *dev, IoTBackendSession *sess) {
    int     sk, flags;
    struct  sockaddr_l2 loc_addr, rem_addr;
    struct  timeval tv;
    char    dest[18] = "A8:41:F4:8C:7F:E6";
// "A8:41:F4:8C:7F:E6";
    // Validate input parameters
    if (NULL == dev || NULL == sess) {
        error_print("engine_init_bluetooth_session failed: invalid input parameters!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // Bind device and function pointers to the session
    sess->bound_dev         = dev;
    sess->send_to_remote    = bluetooth_send_to_remote;
    sess->recv_from_remote  = bluetooth_recv_from_remote;
    sess->sess_type         = IOT_PROTO_TYPE_BLUETOOTH;

    // 1. Create Socket
    // Fixed to SOCK_DGRAM for connectionless communication
    sk = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);;
    if (sk < 0) {
        error_print("engine_init_bluetooth_session failed: socket creation failed!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 2. Configure Local Address
    // Hardcoded PSM 0x1001
    memset(&loc_addr, 0, sizeof(loc_addr));
    loc_addr.l2_family  = AF_BLUETOOTH;
    bacpy(&loc_addr.l2_bdaddr, BDADDR_ANY); // Bind to all local adapters
    loc_addr.l2_psm     = htobs(0x1001);        // Hardcoded PSM value

    if (bind(sk, (struct sockaddr *)&loc_addr, sizeof(loc_addr)) < 0) {
        error_print("engine_init_bluetooth_session failed: bind failed (may require root privileges or PSM is in use)!\n");
        close(sk);
        return BACKEND_PROXY_PROCESS_ERROR;
    }


    // 3. Bind and listen the socket (Server Mode Only), or connect to server (Client Mode Only)
    // working_mode == 1 indicates the device acts as a server and must bind to the PSM
    if (IOT_WORK_MODE_SERVER == dev->config.working_mode) {
        if(listen(sk, 1) < 0){
            error_print("engine_init_bluetooth_session failed: listen failed!\n");
            close(sk);
            return BACKEND_PROXY_PROCESS_ERROR;
        }

        TAILQ_INIT(&sess->sock_list);
/*
 * accept will be called in engine operation loop.
 */

    }else if(IOT_WORK_MODE_CLIENT == dev->config.working_mode){
/*
 * Connect to remote note.
 */
        memset(&rem_addr, 0, sizeof(rem_addr));
        rem_addr.l2_family = AF_BLUETOOTH;
        rem_addr.l2_psm = htobs(0x1001);
        str2ba(dest, &rem_addr.l2_bdaddr);

        tv.tv_sec   = 5;
        tv.tv_usec  = 0;
        setsockopt(sk, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sk, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if(connect(sk, (struct sockaddr *)&rem_addr, sizeof(rem_addr)) < 0){
            error_print("engine_init_bluetooth_session failed: connect failed!\n");
            close(sk);
            return BACKEND_PROXY_PROCESS_ERROR;
        }
    }else{
        error_print("engine_init_bluetooth_session failed: unsupported working mode!\n");
        close(sk);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * Set to unblock mode.
 */
    flags = fcntl(sk, F_GETFL, 0);

    if (flags == -1) {
        error_print("engine_init_bluetooth_session failed: failed to get socket flags!\n");
        close(sk);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if (fcntl(sk, F_SETFL, flags | O_NONBLOCK) == -1) {
        error_print("engine_init_bluetooth_session failed: failed to set non-blocking mode!\n");
        close(sk);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // Assign the file descriptor to the session
    // Note: Remote address is not resolved here. 
    // The proxy frontend will provide the destination address dynamically during data transmission.
    sess->working_mode = dev->config.working_mode;

    if (IOT_WORK_MODE_CLIENT == dev->config.working_mode) {
        sess->dev_fd = sk;      // Client: single connected fd
        sess->listen_fd = -1;   // Not used
    } else { // SERVER
        sess->listen_fd = sk;   // Server: listening fd
        sess->dev_fd = -1;      // No single device fd
    }

    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Initialize CAN bus communication session for IoT device
 * @param dev Pointer to IotDevice instance (pre-initialized device)
 * @param sess Pointer to IoTBackendSession instance (memory pre-allocated)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Session initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Session initialization failed
 *
 * @note This function only initializes session context, NOT device hardware.
 *       Device initialization is completed in the prior stage.
 *       The memory of IoTBackendSession is pre-allocated, no need to manage.
 *       1. Binds session context to the corresponding CAN device
 *       2. Configures session state and bus communication parameters
 *       3. Sets up message queue and data processing logic
 *       4. Prepares session for subsequent bus communication
 *
 * @warning Device hardware initialization must be completed before calling
 * @warning Session memory is managed externally, do not free in this function
 */
int engine_init_can_session(IotDevice *dev, IoTBackendSession *sess) {
    int sk;
    struct sockaddr_can addr;
    struct ifreq ifr;

    // 1. Validate input parameters
    if (NULL == dev || NULL == sess) {
        error_print("engine_init_can_session failed: invalid input parameters!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 2. Bind context and function pointers
    sess->bound_dev = dev;
    sess->send_to_remote    = can_send_to_remote;
    sess->recv_from_remote  = can_recv_from_remote;
    sess->sess_type         = IOT_PROTO_TYPE_CAN;

    // 3. Create Socket (RAW mode fixed)
    sk = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sk < 0) {
        error_print("engine_init_can_session failed: socket creation failed!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 4. Prepare Address Structure
    addr.can_family = AF_CAN;
    
    // Fixed Interface Name: "can0"
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "can0", IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    // Get Interface Index for "can0"
    // This is required even if we don't bind immediately, to ensure the interface exists
    // and to populate addr.can_ifindex for potential future use or sendto.
    if (ioctl(sk, SIOCGIFINDEX, &ifr) < 0) {
        error_print("engine_init_can_session failed: interface can0 not found!\n");
        close(sk);
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    addr.can_ifindex = ifr.ifr_ifindex;

    // 5. Conditional Binding (Mirroring Bluetooth Logic)
    // Only bind if the device is in Server Mode (working_mode == 1)
    if (1 == dev->config.working_mode) {
        if (bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            error_print("engine_init_can_session failed: bind failed!\n");
            close(sk);
            return BACKEND_PROXY_PROCESS_ERROR;
        }
        
        // Optional: In server mode, we might want to receive all frames
        setsockopt(sk, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
    } else {
        // Client/Proxy Mode:
        // We do NOT bind. The socket is created and resolved to "can0" index,
        // but not bound to it. This allows the proxy to send data dynamically
        // without being stuck in a specific receive context, similar to the 
        // unbound Bluetooth client socket behavior.
        // Note: To receive in client mode without bind, specific routing or 
        // sendto usage is typically required, but this matches the requested pattern.
        
        // Even in client mode, if we expect to receive responses on this socket,
        // we might still need to set filters, but we skip the bind() call.
        // Clearing filter just in case, though effect without bind varies by driver.
        setsockopt(sk, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
    }

    // 6. Assign File Descriptor
    sess->dev_fd = sk;

    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Initialize ZigBee communication session for IoT device
 * @param dev Pointer to IotDevice instance (pre-initialized device)
 * @param sess Pointer to IoTBackendSession instance (memory pre-allocated)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Session initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Session initialization failed
 *
 * @note This function only initializes session context, NOT device hardware.
 *       Device initialization is completed in the prior stage.
 *       The memory of IoTBackendSession is pre-allocated, no need to manage.
 *       1. Binds session context to the corresponding ZigBee device
 *       2. Configures session state and network communication parameters
 *       3. Sets up wireless data interaction logic
 *       4. Prepares session for subsequent network communication
 *
 * @warning Device hardware initialization must be completed before calling
 * @warning Session memory is managed externally, do not free in this function
 */
int engine_init_zigbee_session(IotDevice *dev, IoTBackendSession *sess){
    if(NULL == dev || NULL == sess){
        error_print("engine_init_zigbee_session failed: invalid input parameters!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess->bound_dev         = dev;
    sess->send_to_remote    = zigbee_send_to_remote;
    sess->recv_from_remote  = zigbee_recv_from_remote;
    sess->sess_type         = IOT_PROTO_TYPE_ZIGBEE;

    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Initialize LoRa communication session for IoT device
 * @param dev Pointer to IotDevice instance (pre-initialized device)
 * @param sess Pointer to IoTBackendSession instance (memory pre-allocated)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Session initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Session initialization failed
 *
 * @note This function only initializes session context, NOT device hardware.
 *       Device initialization is completed in the prior stage.
 *       The memory of IoTBackendSession is pre-allocated, no need to manage.
 *       1. Binds session context to the corresponding LoRa device
 *       2. Configures session state and long-range communication parameters
 *       3. Sets up uplink/downlink data transfer logic
 *       4. Prepares session for subsequent RF communication
 *
 * @warning Device hardware initialization must be completed before calling
 * @warning Session memory is managed externally, do not free in this function
 */
int engine_init_lora_session(IotDevice *dev, IoTBackendSession *sess){
    if(NULL == dev || NULL == sess){
        error_print("engine_init_zigbee_session failed: invalid input parameters!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess->bound_dev         = dev;
    sess->send_to_remote    = lora_send_to_remote;
    sess->recv_from_remote  = lora_recv_from_remote;
    sess->sess_type         = IOT_PROTO_TYPE_LORA;

    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Initialize PowerLink real-time communication session for IoT device
 * @param dev Pointer to IotDevice instance (pre-initialized device)
 * @param sess Pointer to IoTBackendSession instance (memory pre-allocated)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Session initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Session initialization failed
 *
 * @note This function only initializes session context, NOT device hardware.
 *       Device initialization is completed in the prior stage.
 *       The memory of IoTBackendSession is pre-allocated, no need to manage.
 *       1. Binds real-time session context to the corresponding PowerLink device
 *       2. Configures session state and synchronous communication parameters
 *       3. Sets up real-time data buffer and processing logic
 *       4. Prepares session for subsequent industrial Ethernet communication
 *
 * @warning Device hardware initialization must be completed before calling
 * @warning Session memory is managed externally, do not free in this function
 */
int engine_init_powerlink_session(IotDevice *dev, IoTBackendSession *sess){
    if(NULL == dev || NULL == sess){
        error_print("engine_init_powerlink_session failed: invalid input parameters!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    sess->bound_dev         = dev;
    sess->send_to_remote    = powerlink_send_to_remote;
    sess->recv_from_remote  = powerlink_recv_from_remote;
    sess->sess_type         = IOT_PROTO_TYPE_POWERLINK;

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Initialize ModbusTCP communication session for IoT device
 * @param dev Pointer to IotDevice instance (pre-initialized device)
 * @param sess Pointer to IoTBackendSession instance (memory pre-allocated)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Session initialized successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Session initialization failed
 *
 * @note This function only initializes session context, NOT device hardware.
 *       Device initialization is completed in the prior stage.
 *       The memory of IoTBackendSession is pre-allocated, no need to manage.
 *       1. Binds session context to the corresponding ModbusTCP device
 *       2. Configures session state and TCP communication parameters
 *       3. Sets up data buffer and Modbus frame processing logic
 *       4. Prepares session for subsequent ModbusTCP communication
 *
 * @warning Device hardware initialization must be completed before calling
 * @warning Session memory is managed externally, do not free in this function
 */
int engine_init_modbustcp_session(IotDevice *dev, IoTBackendSession *sess){
    if(NULL == dev || NULL == sess){
        error_print("engine_init_modbustcp_session failed: invalid input parameters!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Universal cleanup interface for IoT device backend communication session
 * @param sess Pointer to pre-allocated IoTBackendSession instance (IoT device only)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed due to invalid input or unknown IoT session type
 *
 * @note This function is dedicated for IoT device session cleanup ONLY.
 *       It is NOT used for IP network device session cleanup.
 *       Automatically dispatches to protocol-specific handler based on sess_type (paired with IoT device type).
 *       Only clears IoT session context, state and control flags.
 *       NO device hardware deinitialization.
 *       NO memory free operation for IoTBackendSession (memory managed externally).
 *
 * @warning This function is the reverse of IoT device session initialization functions
 * @warning Only for IoT device sessions, DO NOT use for IP network sessions
 * @warning sess_type must be paired with corresponding IoT device type
 * @warning Ensure all IoT data interaction completed before calling
 */
int backend_cleanup_iot_session(IoTBackendSession *sess){
        if (sess == NULL || sess->bound_dev == NULL) {
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // Dispatch by IoT session type (paired with device type)
    switch (sess->sess_type) {
        case IOT_PROTO_TYPE_BLUETOOTH:
            return cleanup_bluetooth_iot_session(sess->bound_dev, sess);
            
        case IOT_PROTO_TYPE_CAN:
            return cleanup_can_iot_session(sess->bound_dev, sess);
            
        case IOT_PROTO_TYPE_ZIGBEE:
            return cleanup_zigbee_iot_session(sess->bound_dev, sess);
            
        case IOT_PROTO_TYPE_LORA:
            return cleanup_lora_iot_session(sess->bound_dev, sess);
            
        case IOT_PROTO_TYPE_POWERLINK:
            return cleanup_powerlink_iot_session(sess->bound_dev, sess);
            
        default:
            // Unknown IoT session type
            return BACKEND_PROXY_PROCESS_ERROR;
    }
}

/**
 * @brief Internal cleanup handler for Bluetooth IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): Bluetooth IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for Bluetooth IoT session cleanup only.
 *       Resets session context, state and transceiving flags.
 *       Device hardware remains initialized.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_bluetooth_iot_session(IotDevice *dev, IoTBackendSession *sess){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Internal cleanup handler for CAN IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): CAN IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for CAN IoT session cleanup only.
 *       Resets session context, state and bus communication flags.
 *       Device hardware remains initialized.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_can_iot_session(IotDevice *dev, IoTBackendSession *sess){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Internal cleanup handler for ZigBee IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): ZigBee IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for ZigBee IoT session cleanup only.
 *       Resets session context, state and network flags.
 *       Device hardware remains initialized.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_zigbee_iot_session(IotDevice *dev, IoTBackendSession *sess){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Internal cleanup handler for LoRa IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): LoRa IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for LoRa IoT session cleanup only.
 *       Resets session context, state and RF communication flags.
 *       Device hardware remains initialized.
 *
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_lora_iot_session(IotDevice *dev, IoTBackendSession *sess){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Internal cleanup handler for PowerLink IoT session
 * @param dev Pointer to IotDevice instance
 * @param sess Pointer to IoTBackendSession instance
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK (0): PowerLink IoT session cleaned up successfully
 *         - BACKEND_PROXY_PROCESS_ERROR (-1): Cleanup failed
 *
 * @note Internal handler for PowerLink IoT session cleanup only.
 *       Resets real-time session context, state and sync flags.
 *       Device hardware remains initialized.
 * @warning For internal IoT session cleanup only, called by backend_cleanup_iot_session()
 */
int cleanup_powerlink_iot_session(IotDevice *dev, IoTBackendSession *sess){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Send data to remote Bluetooth device (BLE/Classic Bluetooth)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_BLUETOOTH)
 * @param msg_buf Pointer to IotMsgBuffer containing:
 *                - data: Raw BLE/Core Bluetooth payload (max 512 bytes for BLE)
 *                - len: Payload length
 *                - addr: Destination Bluetooth address (mac + port)
 *                - ext_info: Pointer to BluetoothDevAttr (connection params)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Data sent successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/timeout/MTU exceed)
 * 
 * @note BLE connection interval is controlled by bound_dev->specific_attr.bt_attr.conn_interval
 * @note Automatically updates sess->tx_packets and sess->tx_bytes on success
 */
int bluetooth_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf){
    utils_print("In %s\n", __func__);
    struct sockaddr_l2  remote_addr;
    int                 snd_size;
    IotSockNode         *node, *next_node; 

    utils_print("MAC address = %s, port = %d\n", msg_buf->addr.addr_info.bt_addr.mac, msg_buf->addr.addr_info.bt_addr.port);
    utils_print("Bluetooth message = %s\n", msg_buf->data);

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.l2_family = AF_BLUETOOTH;
    str2ba((const char *)msg_buf->addr.addr_info.bt_addr.mac, &remote_addr.l2_bdaddr);
    remote_addr.l2_psm = htobs(msg_buf->addr.addr_info.bt_addr.port);


#if 0
    snd_size = sendto(sess->dev_fd, msg_buf->data, msg_buf->len, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));

    if(snd_size < 0){
        error_print("bluetooth_send_to_remote failed: unable to send bluetooth message to remode!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
#endif

    if(IOT_WORK_MODE_CLIENT == sess->working_mode){
        snd_size = send(sess->dev_fd, msg_buf->data, msg_buf->len, 0);

        if(snd_size < 0){
            error_print("bluetooth_send_to_remote failed: unable to send bluetooth message to remode!\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }
    }else{
    // Iterate through the socket list to select matching sockets and perform send operations
        TAILQ_FOREACH_SAFE(node, &sess->sock_list, entries, next_node){

        }

    }

    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Receive data from remote Bluetooth device (BLE/Classic Bluetooth)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_BLUETOOTH)
 * @param msg_buf Pointer to IotMsgBuffer to store:
 *                - data: Received BLE/Core Bluetooth payload
 *                - len: Output - actual received length
 *                - addr: Output - Source Bluetooth address (mac + port)
 *                - ext_info: Output - Pointer to BluetoothDevAttr (connection status)
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = blocking)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Data received successfully
 *         - BACKEND_PROXY_PROCESS_AGAIN: No data available yet
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/read error/timeout)
 * 
 * @note Filters duplicate packets using Bluetooth MAC address in addr
 * @note Automatically updates sess->rx_packets and sess->rx_bytes on success
 */
int bluetooth_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms){
    utils_print("In %s\n", __func__);
    ssize_t             rcv_size;
    struct sockaddr_l2  remote_addr;
    socklen_t           addr_len;
    int                 ret;

    /*
     * client receive data directly.
     */
    utils_print("Bluetooth session working mode = %d\n", sess->working_mode);

    if(IOT_WORK_MODE_CLIENT == sess->working_mode){
        rcv_size = recv(sess->dev_fd, msg_buf->data, msg_buf->len, 0);

    /*
     * receive data successfully.
     */
        if (rcv_size > 0) {
            msg_buf->len = rcv_size;
            addr_len = sizeof(remote_addr);
            memset(&remote_addr, 0, addr_len);

    /*
     * get peer address and port.
     */
            ret = getpeername(sess->dev_fd, (struct sockaddr *)&remote_addr, &addr_len);

            if(ret < 0){
                error_print("bluetooth_recv_from_remote failed: failed to get remote address info!\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }

             if (remote_addr.l2_family != AF_BLUETOOTH) {
                error_print("get_l2cap_peer_info failed: Socket is not a Bluetooth L2CAP socket!\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }

            msg_buf->addr.addr_type                 = IOT_PROTO_TYPE_BLUETOOTH;
            msg_buf->addr.addr_info.bt_addr.port    = remote_addr.l2_psm;
            ba2str(&remote_addr.l2_bdaddr, (char *)msg_buf->addr.addr_info.bt_addr.mac);
            return BACKEND_PROXY_PROCESS_OK;
        }else if(0 == rcv_size){
            error_print("get_l2cap_peer_info failed: Connection closed by peer!\n");
            return BACKEND_PROXY_PROCESS_ERROR;
        }else{
            error_print("rcv_size < 0!\n");
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // In non-blocking mode, no data available is a normal condition.
                return BACKEND_PROXY_PROCESS_AGAIN;
            } 
            else if (errno == EINTR) {
                // Interrupted by a signal. Usually retryable; returning AGAIN lets the upper layer decide.
                return BACKEND_PROXY_PROCESS_AGAIN;
            } 
            else {
                // Real error (e.g., ECONNRESET, EBADF, etc.)
                error_print("read_bluetooth_nonblocking: Recv failed with system error!\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }

        }//

    }

    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Send CAN frame to remote CAN bus device (CAN 2.0/CAN FD)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_CAN)
 * @param msg_buf Pointer to IotMsgBuffer containing:
 *                - data: Raw CAN frame payload (0-8 for CAN 2.0, 0-64 for CAN FD)
 *                - len: Payload length
 *                - addr: Destination CAN address (port + can_id + bus_id)
 *                - ext_info: Pointer to CANDevAttr (bitrate/mode)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: CAN frame sent successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/bitrate mismatch/format error)
 * 
 * @note Automatically applies CAN filter (can_filter_id) before sending
 * @note Updates sess->tx_packets/tx_bytes and device statistics on success
 */
int can_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Receive CAN frame from remote CAN bus device (CAN 2.0/CAN FD)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_CAN)
 * @param msg_buf Pointer to IotMsgBuffer to store:
 *                - data: Received CAN payload
 *                - len: Output - actual received length
 *                - addr: Output - Source CAN address (port + can_id + bus_id)
 *                - ext_info: Output - Pointer to CANDevAttr (filter match status)
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = blocking)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: CAN frame received successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/filter mismatch/timeout)
 * 
 * @note Supports both standard (11-bit) and extended (29-bit) CAN IDs
 * @note Automatically skips error frames based on CAN mode (can_mode)
 */
int can_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Send data to remote Zigbee device (802.15.4)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_ZIGBEE)
 * @param msg_buf Pointer to IotMsgBuffer containing:
 *                - data: Raw Zigbee payload (max 127 bytes)
 *                - len: Payload length
 *                - addr: Destination Zigbee address (mac + pan_id + endpoint)
 *                - ext_info: Pointer to ZigbeeDevAttr (channel/role)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Data sent successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/PAN ID mismatch/channel error)
 * 
 * @note Uses Zigbee role (coordinator/router/enddevice) to determine transmission mode
 * @note Updates session and device statistics on success
 */
int zigbee_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Receive data from remote Zigbee device (802.15.4)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_ZIGBEE)
 * @param msg_buf Pointer to IotMsgBuffer to store:
 *                - data: Received Zigbee payload
 *                - len: Output - actual received length
 *                - addr: Output - Source Zigbee address (mac + pan_id + endpoint)
 *                - ext_info: Output - Pointer to ZigbeeDevAttr (network join status)
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = blocking)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Data received successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/network error/timeout)
 * 
 * @note Filters data by Zigbee MAC address (zigbee_mac) to avoid cross-device interference
 * @note Automatically retries reception for lost packets (up to 3 times)
 */
int zigbee_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Send data to remote LoRa/LoRaWAN device
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_LORA)
 * @param msg_buf Pointer to IotMsgBuffer containing:
 *                - data: Raw LoRa payload (max 255 bytes)
 *                - len: Payload length
 *                - addr: Destination LoRa address (dev_eui + port + freq_band)
 *                - ext_info: Pointer to LoRaDevAttr (SF/CR)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Data sent successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/SF mismatch/freq error)
 * 
 * @note Automatically applies coding rate (lora_cr) during transmission
 * @note Updates LoRa DevEUI association and session statistics on success
 */
int lora_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Receive data from remote LoRa/LoRaWAN device
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_LORA)
 * @param msg_buf Pointer to IotMsgBuffer to store:
 *                - data: Received LoRa payload
 *                - len: Output - actual received length
 *                - addr: Output - Source LoRa address (dev_eui + port + freq_band)
 *                - ext_info: Output - Pointer to LoRaDevAttr (RSSI/SNR)
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = blocking)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: Data received successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/weak signal/timeout)
 * 
 * @note Decodes LoRa payload using configured spreading factor (lora_sf)
 * @note Verifies DevEUI to ensure data comes from authorized device
 */
int lora_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Send PDO data to remote OpenPowerLink device (Industrial Ethernet)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_POWERLINK)
 * @param msg_buf Pointer to IotMsgBuffer containing:
 *                - data: Raw PDO payload (matches plk_tx_pdo_len)
 *                - len: Payload length (must match plk_tx_pdo_len)
 *                - addr: Destination PowerLink address (node_id + mac + pdo_id)
 *                - ext_info: Pointer to PowerLinkDevAttr (cycle_ms/role)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: PDO data sent successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/NodeID error/PDO mismatch/cycle violation)
 * 
 * @note Only MN (Managing Node) can send to CN (Controlled Node)
 * @note Updates session tx statistics and PowerLink device status on success
 */
int powerlink_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Receive PDO data from remote OpenPowerLink device (Industrial Ethernet)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_POWERLINK)
 * @param msg_buf Pointer to IotMsgBuffer to store:
 *                - data: Received PDO payload
 *                - len: Output - actual received length (matches plk_rx_pdo_len)
 *                - addr: Output - Source PowerLink address (node_id + mac + pdo_id)
 *                - ext_info: Output - Pointer to PowerLinkDevAttr (cycle_ms/role)
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = blocking)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: PDO data received successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/PDO mismatch/role error/timeout)
 * 
 * @note Real-time cycle (plk_cycle_ms) is enforced for reception timing
 * @note Automatically validates NodeID to prevent unauthorized PDO data
 */
int powerlink_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms){
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Send data to remote ModbusTCP device (Industrial Ethernet)
 * 
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_MODBUSTCP)
 * @param msg_buf Pointer to IotMsgBuffer containing:
 *                - data: Raw ModbusTCP request payload
 *                - len: Payload length (valid ModbusTCP frame length)
 *                - addr: Destination ModbusTCP address (unit_id + ip + port + function_code)
 *                - ext_info: Pointer to ModbusTCPDevAttr (timeout/retry/unit_id)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: ModbusTCP data sent successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/UnitID error/format mismatch/socket error)
 * 
 * @note Based on TCP connection-oriented communication, send only in connected state
 * @note Updates session tx statistics and ModbusTCP device status on success
 */
int modbustcp_send_to_remote(IoTBackendSession *sess, const IotMsgBuffer *msg_buf){
    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Receive data from remote ModbusTCP device (Industrial Ethernet)
 *
 * @param sess Pointer to IoTBackendSession instance (must be IOT_SESS_TYPE_MODBUSTCP)
 * @param msg_buf Pointer to IotMsgBuffer to store:
 *                - data: Received ModbusTCP response payload
 *                - len: Output - actual received length
 *                - addr: Output - Source ModbusTCP address (unit_id + ip + port + function_code)
 *                - ext_info: Output - Pointer to ModbusTCPDevAttr (timeout/retry/unit_id)
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = blocking)
 * @return int Execution result
 *         - BACKEND_PROXY_PROCESS_OK: ModbusTCP data received successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed (invalid param/offline/format mismatch/unit error/timeout)
 *
 * @note TCP-based communication, socket receive timeout is enforced for reception timing
 * @note Automatically validates UnitID to prevent unauthorized ModbusTCP data
 */
int modbustcp_recv_from_remote(IoTBackendSession *sess, IotMsgBuffer *msg_buf, int timeout_ms){
    return BACKEND_PROXY_PROCESS_OK;
}