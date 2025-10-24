#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "common_utils.h"

#define PROXY_PROTO_VERSION_1                            1
// #define PROXY_MSG_TYPE_DEV                               0
// #define PROXY_MSG_TYPE_STRGY                             1
// #define PROXY_MSG_TYPE_SESS                              2
// #define PROXY_MSG_TYPE_DATA                              3


typedef enum {
    PROXY_MSG_TYPE_DEV = 0,    // Device message
    PROXY_MSG_TYPE_STRGY,      // Strategy message
    PROXY_MSG_TYPE_SESS,       // Session message
    PROXY_MSG_TYPE_DATA        // Data message
} ProxyMsgType;


#define PROXY_PROTO_DEV_VERSION_1                        1
#define PROXY_PROTO_STRGY_VERSION_1                      1
#define PROXY_PROTO_SESS_VERSION_1                       1



#define PROXY_MSG_HDR_SIZE                               8
#define PROXY_MSG_MIN_SIZE                               1
#define PROXY_MSG_MAX_SIZE                               4088
// Sum of header size and maximum message size (total maximum size including header, in bytes)
#define PROXY_MSG_HDR_PLUS_MAX_SIZE                      (PROXY_MSG_HDR_SIZE + PROXY_MSG_MAX_SIZE) 
#define PROXY_MSG_INVALID_LEN                            -1

struct IPv4Address {
    uint8_t data[4];  
}__attribute__((packed));

struct IPv6Address {
    uint8_t data[16]; 
};

union IPAddress {
    struct IPv4Address ipv4_addr;
    struct IPv6Address ipv6_addr; 
}__attribute__((packed));


/*
 * Macro: Copy IPv4 address from struct in_addr to custom struct IPv4Address
 * Parameters:
 *   dest - Destination structure pointer (struct IPv4Address*)
 *   src  - Source structure pointer (const struct in_addr*)
 * Notes:
 *   1. Converts 32-bit network byte order address to 4-byte array in host order
 *   2. Includes null pointer check to prevent invalid memory access
 */
#define COPY_IN_TO_IPV4(dest, src) do { \
    if ((dest) != NULL && (src) != NULL) { \
        uint32_t addr = ntohl((src)->s_addr);  \
        (dest)->data[0] = (addr >> 24) & 0xFF; \
        (dest)->data[1] = (addr >> 16) & 0xFF; \
        (dest)->data[2] = (addr >> 8) & 0xFF; \
        (dest)->data[3] = addr & 0xFF; \
    } \
} while (0)

/*
 * Macro: Copy data from custom struct IPv4Address to struct in_addr
 * Parameters:
 *   dest - Destination structure pointer (struct in_addr*)
 *   src  - Source structure pointer (const struct IPv4Address*)
 * Notes:
 *   1. Combines 4-byte array into 32-bit value in network byte order
 *   2. Reverse operation of COPY_IN_TO_IPV4 macro
 */
#define COPY_IPV4_TO_IN(dest, src) do { \
    if ((dest) != NULL && (src) != NULL) { \
        uint32_t addr = ((uint32_t)(src)->data[0] << 24) | \
                       ((uint32_t)(src)->data[1] << 16) | \
                       ((uint32_t)(src)->data[2] << 8) | \
                       (uint32_t)(src)->data[3]; \
        (dest)->s_addr = htonl(addr);  \
    } \
} while (0)


/*
 * Macro: Copy IPv6 address from struct in6_addr to custom struct IPv6Address
 * Parameters:
 *   dest - Destination structure pointer (struct IPv6Address*)
 *   src  - Source structure pointer (const struct in6_addr*)
 * Notes:
 *   1. Internally uses memcpy to copy 16 bytes of data (their memory layouts are compatible)
 *   2. Includes null pointer check to avoid accessing null pointers
 */
#define COPY_IN6_TO_IPV6(dest, src) do { \
    if ((dest) != NULL && (src) != NULL) { \
        memcpy((dest)->data, (src)->s6_addr, 16); \
    } \
} while (0)

/*
 * Macro: Copy data from custom struct IPv6Address to struct in6_addr
 * Parameters:
 *   dest - Destination structure pointer (struct in6_addr*)
 *   src  - Source structure pointer (const struct IPv6Address*)
 * Notes:
 *   Reverse copy, functionally symmetric to the above macro
 */
#define COPY_IPV6_TO_IN6(dest, src) do { \
    if ((dest) != NULL && (src) != NULL) { \
        memcpy((dest)->s6_addr, (src)->data, 16); \
    } \
} while (0)


/**
 * @brief Proxy message header structure
 * 
 * This structure defines the header format for proxy messages, containing metadata 
 * such as protocol version, message type, session identifiers, and payload length.
 */
typedef struct {
    uint8_t     version;             // Protocol version. Currently unused, set to 1.
    uint8_t     proxy_msg_type;      // Proxy message type. Possible values: device message (0), strategy message (1), session message (2), data message (3).
    uint16_t    frontend_sess_id;    // Frontend session ID. Used for matching frontend and backend sessions in the frontend proxy.
    uint16_t    backend_sess_id;     // Backend session ID. Used for matching frontend and backend sessions in the backend proxy.
    uint16_t    payload_len;         // Payload length in bytes. Valid range: 1 to 4088. Must not exceed one physical page.
} __attribute__((packed)) ProxyMsgHeader;




/**
 * Calculate the total memory space occupied by the complete message described by ProxyMsgHeader
 * Including: size of the header structure itself + length of the payload data
 */
#define PROXY_MSG_TOTAL_SIZE(p_msg_header) \
    (sizeof(ProxyMsgHeader) + (p_msg_header)->payload_len)


/**
 * @brief Macro to calculate total shared queue memory size with fixed fragment size
 * 
 * For cross-system shared memory usage with strict size regulations, each fragment 
 * occupies a fixed size regardless of actual payload:
 * - Each fragment = sizeof(ProxyMsgHeader) + PROXY_MSG_MAX_SIZE
 * - Total memory = number of fragments × fixed fragment size
 * 
 * Number of fragments is calculated using ceiling division to ensure all data is covered.
 * 
 * @param data_size Size of the data payload to be sent (in bytes)
 * @return size_t Total shared queue memory required (in bytes)
 */
#define PROXY_MSG_TOTAL_MEM_SIZE(data_size) \
    ( \
        /* Calculate number of fragments (ceiling division) */ \
        ( ((data_size) + PROXY_MSG_MAX_SIZE - 1) / PROXY_MSG_MAX_SIZE ) \
        * (sizeof(ProxyMsgHeader) + PROXY_MSG_MAX_SIZE) /* Fixed size per fragment */ \
    )


typedef enum {
    ACTION_TYPE_COMMAND = 0,  // Command
    ACTION_TYPE_RESPONSE      // Response
} ActionType;


/**
 * @brief Device message header structure
 * 
 * This structure defines the header format for device-related messages, containing metadata such as
 * protocol version, message type, message identifier, signaling type, and payload length.
 */
typedef struct {
    uint16_t version;        // Protocol version. Currently unused; set to 1.
    uint16_t msg_type;       // Message type. Possible values: Disable (0), Enable (1), Query (2)
    uint16_t msg_id;         // Message ID. Used to match commands with their corresponding responses.
    uint16_t action_type;    // Signaling type. Possible values: Command (0), Response (1)
    uint16_t payload_len;    // Payload length in bytes.
} __attribute__((packed)) DevMsgHeader;


typedef enum {
    DEV_MSG_DISABLE = 0,  // Disable
    DEV_MSG_ENABLE,       // Enable
    DEV_MSG_QUERY         // Query
} DevMsgType;

//  Check if the device message type is valid
#define IS_VALID_DEV_MSG_TYPE(dev_msg_type) \
    ((dev_msg_type) == DEV_MSG_DISABLE || \
     (dev_msg_type) == DEV_MSG_ENABLE || \
     (dev_msg_type) == DEV_MSG_QUERY)


/**
 * @brief Device message mask structure
 * 
 * Used to store the content of the "Enable"/"Disable" commands, specifically representing the mask 
 * that indicates which devices are selected for enabling or disabling.
 */
typedef struct {
    uint16_t data;   // Content of the "Enable"/"Disable" commands, representing the mask that indicates which devices are selected to enable or disable.
} __attribute__((packed)) DevMsgMask;



/**
 * @brief Device message report structure
 * 
 * This structure contains the response data for the "Query" command, including 
 * status information, error details, and the active device mask.
 */
typedef struct {
    uint8_t status;    // Status code indicating the overall result of the operation
    uint8_t error;     // Error code providing specific details about any errors encountered
    uint16_t data;     // Response data from the "Query" command, returning the mask indicating which devices are active
} __attribute__((packed)) DevMsgReport;


/**
 * Calculate the payload length of a device message based on its type and action type.
 * 
 * @param dev_msg_type    Device message type (DEV_MSG_ENABLE, DEV_MSG_DISABLE or DEV_MSG_QUERY)
 * @param action_type     Action type (ACTION_TYPE_COMMAND or ACTION_TYPE_RESPONSE)
 * @return                Payload length in bytes, or PROXY_MSG_INVALID_LEN if type is invalid
 */
#define DEV_MSG_PAYLOAD_LEN(dev_msg_type, action_type) \
( \
    /* Check if device message type is valid */ \
    (dev_msg_type == DEV_MSG_ENABLE) ? \
        ( \
            /* Check if action type is valid */ \
            (action_type == ACTION_TYPE_COMMAND)  ? 2 : \
            (action_type == ACTION_TYPE_RESPONSE) ? 2 : \
            PROXY_MSG_INVALID_LEN  /* Invalid action type */ \
        ) : \
    (dev_msg_type == DEV_MSG_DISABLE) ? \
        ( \
            (action_type == ACTION_TYPE_COMMAND)  ? 2 : \
            (action_type == ACTION_TYPE_RESPONSE) ? 2 : \
            PROXY_MSG_INVALID_LEN  /* Invalid action type */ \
        ) : \
    (dev_msg_type == DEV_MSG_QUERY) ? \
        ( \
            (action_type == ACTION_TYPE_COMMAND)  ? 0 : \
            (action_type == ACTION_TYPE_RESPONSE) ? 4 : \
            PROXY_MSG_INVALID_LEN  /* Invalid action type */ \
        ) : \
    PROXY_MSG_INVALID_LEN  /* Invalid device message type */ \
)


/* 
 * Get payload length directly from DevMsgHeader struct
 * euses DEV_MSG_PAYLOAD_LEN to avoid duplicate logic
 */
#define DEV_MSG_HEADER_PAYLOAD_LEN(header) \
    DEV_MSG_PAYLOAD_LEN((header)->msg_type, (header)->action_type)


/**
 * @brief Strategy message header structure
 * 
 * This structure defines the header format for strategy-related messages, containing metadata such as
 * protocol version, message type, message identifier, signaling type, and payload length.
 */
typedef struct {
    uint16_t version;       // Protocol version. Currently unused; set to 1.
    uint16_t msg_type;      // Message type. Possible values: Set (0), Query (1)
    uint16_t msg_id;        // Message ID. Used to match commands with their corresponding responses.
    uint16_t action_type;   // Signaling type. Possible values: Command (0), Response (1)
    uint16_t payload_len;   // Payload length in bytes.
} __attribute__((packed)) StrgyMsgHeader;


typedef enum {
    STRGY_MSG_SET = 0,       // Set
    STRGY_MSG_QUERY          // Query
} StrgyMsgType;


// Check if the strategy message type is valid
#define IS_VALID_STRGY_MSG_TYPE(strgy_msg_type) \
    ((strgy_msg_type) == STRGY_MSG_SET || \
     (strgy_msg_type) == STRGY_MSG_QUERY)


/**
 * Calculate the payload length of a strategy message based on its type and action type.
 * 
 * @param strgy_msg_type  Strategy message type (STRGY_MSG_SET or STRGY_MSG_QUERY)
 * @param action_type     Action type (ACTION_TYPE_COMMAND or ACTION_TYPE_RESPONSE)
 * @return                Payload length in bytes, or PROXY_MSG_INVALID_LEN if type is invalid
 */
#define STRGY_MSG_PAYLOAD_LEN(strgy_msg_type, action_type) \
( \
    /* Check strategy message type first */ \
    (strgy_msg_type == STRGY_MSG_SET) ? \
        ( \
            /* Determine length for SET message based on action type */ \
            (action_type == ACTION_TYPE_COMMAND)  ? 2 :  /* SET command → 2 bytes */ \
            (action_type == ACTION_TYPE_RESPONSE) ? 2 :  /* SET response → 2 bytes */ \
            PROXY_MSG_INVALID_LEN  /* Invalid action type for SET message */ \
        ) : \
    (strgy_msg_type == STRGY_MSG_QUERY) ? \
        ( \
            /* Determine length for QUERY message based on action type */ \
            (action_type == ACTION_TYPE_COMMAND)  ? 0 :  /* QUERY command → 0 bytes */ \
            (action_type == ACTION_TYPE_RESPONSE) ? 4 :  /* QUERY response → 4 bytes */ \
            PROXY_MSG_INVALID_LEN  /* Invalid action type for QUERY message */ \
        ) : \
    PROXY_MSG_INVALID_LEN  /* Invalid strategy message type */ \
)


/* 
 * Get payload length directly from StrgyMsgHeader struct
 * euses STRGY_MSG_PAYLOAD_LEN to avoid duplicate logic
 */
#define STRGY_MSG_HEADER_PAYLOAD_LEN(header) \
    DEV_MSG_PAYLOAD_LEN((header)->msg_type, (header)->action_type)


/**
 * @brief Strategy command enable message structure
 * 
 * This structure defines the format for strategy command enable messages, specifically containing
 * parameters required when enabling a specified strategy. It is used to传递 configuration details
 * for strategy activation.
 */
typedef struct {
    uint16_t        strgy_para;    // Strategy parameter. Possible values: 0 (Round Robin), 1 (Select device with highest current available bandwidth), 2 (Select device with lowest current latency)
} __attribute__((packed)) StrgyCMDEnableMessage;


/**
 * @brief Strategy message report structure
 * 
 * This structure contains the response data for strategy-related "Query" commands, including
 * a status code, error code, and the active strategy information returned by the query.
 */
typedef struct {
    uint8_t    status;      // Status code indicating the overall result of the strategy operation (e.g., success or failure)
    uint8_t    error;       // Error code providing specific details if the strategy operation encountered an error (0 for no error)
    uint16_t   data[];      // Flexible array member acting as a placeholder. Stores the response data from the "Query" command, specifically the active strategy code.
} __attribute__((packed)) StrgyMsgReport;


/**
 * @brief Session message header structure
 * 
 * This structure defines the header format for session-related messages, containing metadata such as
 * protocol version, message type, signaling type, IP version, and payload length. It is used for
 * managing session operations like creation and closure.
 */
typedef struct {
    uint16_t   version;        // Protocol version. Currently unused; set to 1.
    uint16_t   msg_type;       // Message type. Possible values: Create (0), Close (1)
    uint16_t   action_type;    // Signaling type. Possible values: Command (0), Response (1)
    uint16_t   ip_version;     // IP version. Possible values: SESS_IPV4_PROTO (4), SESS_IPV6_PROTO (6)
    uint16_t   payload_len;    // Payload length in bytes.
} __attribute__((packed)) SessMsgHeader;



/**
 * @brief IPv4 session parameter structure
 * 
 * This structure contains parameters required for establishing or managing an IPv4-based session,
 * including device identification, transport protocol, IPv4 address, and corresponding port information.
 */
typedef struct{
    uint16_t            dev_id;         // Device identifier, uniquely identifies the target device in the session
    uint16_t            trans_proto;    // Transport protocol used for the session (e.g., TCP, UDP)
    struct IPv4Address  ipv4_addr;      // IPv4 address structure containing the device's IPv4 address information
    uint16_t            port;           // Port number associated with the IPv4 address for the session
} __attribute__((packed)) SessParaIPv4;



/**
 * @brief IPv6 session parameter structure
 * 
 * This structure contains parameters required for establishing or managing an IPv6-based session,
 * including device identification, transport protocol, IPv6 address, and corresponding port information.
 */
typedef struct{
    uint16_t            dev_id;         // Device identifier, uniquely identifies the target device in the session
    uint16_t            trans_proto;    // Transport protocol used for the session (e.g., TCP, UDP)
    struct IPv6Address  ipv6_addr;      // IPv6 address structure containing the device's IPv6 address information
    uint16_t            port;           // Port number associated with the IPv6 address for the session
} __attribute__((packed)) SessParaIPv6;


typedef enum {
    SESS_MSG_CLOSE = 0,             // Close
    SESS_MSG_CREATE                 // Create
} SessMsgType;


typedef enum {
    SESS_NON_IP_PROTO = 0,       // None-IP protocol
    SESS_IPV4_PROTO   = 4,       // IPv4
    SESS_IPV6_PROTO   = 6        // IPv6
} SessIpProtoVersion;


typedef enum {
    SESS_UDP_PROTO = 0,            // UDP
    SESS_TCP_PROTO = 1,            // TCP
    SESS_FASTPATH_PROTO = 2        // XDP or eBPF
} SessTranProto;

// Check if the session message type is valid
#define IS_VALID_SESS_MSG_TYPE(sess_msg_type) \
    ((sess_msg_type) == SESS_MSG_CREATE || \
     (sess_msg_type) == SESS_MSG_CLOSE)


// Check if the IP protocol verion is valid
#define IS_VALID_SESS_IP_VERSION(ip_version) \
    ((ip_version) == SESS_IPV4_PROTO || \
     (ip_version) == SESS_IPV6_PROTO)



/**
 * Calculate the payload length of a session message based on its type, action type, and IP protocol version.
 * 
 * @param sess_msg_type  Session message type (SESS_MSG_CREATE or SESS_MSG_CLOSE)
 * @param action_type    Action type (ACTION_TYPE_COMMAND or ACTION_TYPE_RESPONSE)
 * @param ip_version     IP protocol version (SESS_IPV4_PROTO or SESS_IPV6_PROTO)
 * @return               Payload length in bytes, or PROXY_MSG_INVALID_LEN if type/version is invalid
 */
#define SESS_MSG_PAYLOAD_LEN(sess_msg_type, action_type, ip_version) \
( \
    /* Check session message type first */ \
    (sess_msg_type == SESS_MSG_CLOSE) ? \
        ( \
            utils_print("type is SESS_MSG_CLOSE\n"),\
            /* Determine length for CLOSE message based on action type */ \
            (action_type == ACTION_TYPE_COMMAND)  ? 0 :  /* CLOSE command → 0 bytes */ \
            (action_type == ACTION_TYPE_RESPONSE) ? 2 :  /* CLOSE response → 2 bytes (status + error) */ \
            PROXY_MSG_INVALID_LEN  /* Invalid action type for CLOSE message */ \
        ) : \
    (sess_msg_type == SESS_MSG_CREATE) ? \
        ( \
            utils_print("type is SESS_MSG_CREATE, action type is %d, ip_version is %d\n",action_type,  ip_version),\
            /* Determine length for CREATE message based on action type */ \
            (action_type == ACTION_TYPE_RESPONSE) ? 2 :  /* CREATE response → 2 bytes (status + error) */ \
            (action_type == ACTION_TYPE_COMMAND)  ? \
                ( \
                    /* Determine length for CREATE command based on IP version */ \
                    (ip_version == SESS_IPV4_PROTO) ? 10 :  /* IPv4 → 10 bytes (session params) */ \
                    (ip_version == SESS_IPV6_PROTO) ? 22 :  /* IPv6 → 22 bytes (session params) */ \
                    PROXY_MSG_INVALID_LEN  /* Invalid IP version for CREATE command */ \
                ) : \
            PROXY_MSG_INVALID_LEN  /* Invalid action type for CREATE message */ \
        ) : \
    PROXY_MSG_INVALID_LEN  /* Invalid session message type */ \
)

/* 
 * Get payload length directly from StrgySessHeader struct
 * euses STRGY_MSG_PAYLOAD_LEN to avoid duplicate logic
 */
#define SESS_MSG_HEADER_PAYLOAD_LEN(header) \
    SESS_MSG_PAYLOAD_LEN((header)->msg_type, (header)->action_type, (header)->ip_version)

typedef struct {
    struct IPv4Address  ipv4_addr;       // IPv4 address
    uint16_t            port;   	     // Transport layer port；
} __attribute__((packed)) IPv4PortTuple;

typedef struct {
    struct IPv6Address  ipv6_addr;       // IPv6 address
    uint16_t            port;   	                // Transport layer port
} __attribute__((packed)) IPv6PortTuple;

typedef union {
    IPv4PortTuple ipv4_port_tuple;
    IPv6PortTuple ipv6_port_tuple;
}IPPortTuple;

/*
 * The structure of the session create-response message's payload.
 */
// Corresponds to the status field in the structure, indicating the overall status of the session operation (success/failure)
typedef enum {
    SESS_OP_STATUS_SUCCESS = 0,  // Session operation succeeded
    SESS_OP_STATUS_FAIL    = 1,   // Session operation failed
    SESS_OP_STATUS_NUM     = 2   // Total number of enumeration members
} SessOpStatus;


// Corresponds to the code field in the structure, indicating specific reason codes for operation results
typedef enum {
    SESS_OP_CODE_SUCCESS                = 0,  // Operation succeeded
    SESS_OP_CODE_NO_PERMISSION          = 1,  // No permission to perform the operation
    SESS_OP_CODE_DEVICE_ERROR           = 2,  // Device error occurred
    SESS_OP_CODE_RESOURCE_INSUFFICIENT  = 3,  // Insufficient resources
    SESS_OP_CODE_NETWORK_UNREACHABLE    = 4,  // Network is unreachable
    SESS_OP_CODE_MAX                    = 5   // Total number of enumeration members
} SessOpCode;


// Structure for session operation response data, containing status and specific reason code
typedef struct SessOpRespData_ {
    uint8_t             status;  // Corresponding to SessOpStatus enumeration (overall operation status)
    uint8_t             code;    // Corresponding to SessOpCode enumeration (specific reason code for operation result)
} __attribute__((packed)) SessOpRespData;

/*
 * Session message parameter structure, used to describe core parameters related to session establishment,
 * including device identification, transport protocol type, and IP-port combination and other key information.
 */
struct SessMsgPara{
// Frontend session ID, used to deliver information for establishing a new session.
    uint16_t        frontend_sess_id;
// Backend session ID, used to deliver information for establishing a new session.
    uint16_t        backend_sess_id;
// Device ID, of type uint16_t, with value range 0x0000-0xFFFF; when set to 0xFFFF, it indicates entering vertical handover mode.
    uint16_t        dev_id;
// IP version, of type uint16_t, of type uint16_t, supported values include: 4 (IPv4), 6 (IPv6).
    uint16_t        ip_version;
// Transport layer protocol type, of type uint16_t, supported values include: SESS_UDP_PROTO (0, UDP protocol), SESS_TCP_PROTO(1, TCP protocol), SESS_FASTPATH_PROTO(2, FastPath protocol).
    uint16_t        trans_proto;
// Tuple of IP address and port number, used to describe the combination of IP address and corresponding port number of the communication endpoint.
    IPPortTuple     ip_port_tuple;
};


/**
 * @brief Session parameters structure for IPv4-based sessions
 * @details Contains parameters required to establish and manage an IPv4 session,
 *          including transport protocol, device selection, and destination endpoint.
 *          Uses packed alignment to ensure contiguous memory layout.
 */
typedef struct {
//    uint16_t            transport_layer_proto;  /**< Transport layer protocol (2 bytes, e.g., SESS_TCP_PROTO for TCP, SESS_UDP_PROTO for UDP) */
    uint16_t            device_selection;       /**< Device selection identifier (2 bytes) */
    uint16_t            transport_layer_proto;  /**< Transport layer protocol (2 bytes, e.g., SESS_TCP_PROTO for TCP, SESS_UDP_PROTO for UDP) */
    IPv4PortTuple       dest_endpoint;          /**< Destination endpoint containing IPv4 address and port */
} __attribute__((packed)) SessIPv4Params;



/**
 * @brief Session parameters structure for IPv6-based sessions
 * @details Contains parameters required to establish and manage an IPv6 session,
 *          including transport protocol, device selection, and destination endpoint.
 *          Uses packed alignment to ensure contiguous memory layout.
 */
typedef struct {
//    uint16_t            transport_layer_proto;  /**< Transport layer protocol (2 bytes, e.g., 6 for TCP, 17 for UDP) */
    uint16_t            device_selection;       /**< Device selection identifier (2 bytes) */
    uint16_t            transport_layer_proto;  /**< Transport layer protocol (2 bytes, e.g., 6 for TCP, 17 for UDP) */
    IPv6PortTuple       dest_endpoint;          /**< Destination endpoint containing IPv6 address and port */
} __attribute__((packed)) SessIPv6Params;


typedef struct{
    ProxyMsgHeader  outer_header;
//    ProxyMsgType    msg_type;
    union {        // Nested union to reduce memory usage (avoids redundant space)
        DevMsgHeader    dev_hdr;    // Device message header member
        StrgyMsgHeader  strgy_hdr;  // Strategy message header member
        SessMsgHeader   sess_hdr;   // Session message header member
    } inner_header; // Nested union alias for easy access to specific headers
} GeneralProxyMsgHeader;

struct BackendEngine_;
struct SharedMemoryPoolQueue;

typedef enum {
    MEMORY_ALLOC_SHARED,    // Allocate in shared memory
    MEMORY_ALLOC_CALLER     // Allocated by caller
} MemoryAllocMode;

int build_proxy_general_message(struct BackendEngine_ *engine, GeneralProxyMsgHeader *header, const uint8_t *payload, size_t payload_len, 
                                uint8_t **result_msg, MemoryAllocMode alloc_mode, struct SharedMemoryPoolQueue *ring_buf);
int build_proxy_dev_message(DevMsgHeader *header, const uint8_t *payload, size_t payload_len, uint8_t **result_msg);
int build_proxy_strgy_message(StrgyMsgHeader *header, const uint8_t *payload, size_t payload_len, uint8_t **result_msg);
int build_proxy_sess_message(SessMsgHeader *header, const uint8_t *payload, size_t payload_len, uint8_t **result_msg);
int build_proxy_data_message(ProxyMsgHeader *header, const uint8_t *payload, size_t payload_len, uint8_t **result_msg);


#endif