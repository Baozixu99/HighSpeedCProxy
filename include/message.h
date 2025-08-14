#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define PROXY_PROTO_VERSION_1                            1
#define PROXY_MSG_TYPE_DEV                               0
#define PROXY_MSG_TYPE_STRGY                             1
#define PROXY_MSG_TYPE_SESS                              2
#define PROXY_MSG_TYPE_DATA                              3



#define PROXY_PROTO_DEV_VERSION_1                        1
#define PROXY_PROTO_STRGY_VERSION_1                      1
#define PROXY_PROTO_SESS_VERSION_1                       1



#define PROXY_MSG_HDR_SIZE                               8
#define PROXY_MSG_MIN_SIZE                               1
#define PROXY_MSG_MAX_SIZE                               4088

typedef struct {
    uint8_t version;             // Protocol version, not used currently, set to 1.
    uint8_t proxy_msg_type;      // Proxy message type, divided into device message (0), strategy message (1), session message (2), and data message (3).
    uint16_t frontend_sess_id;   // Frontend session ID, used to match frontend and backend sessions in the frontend proxy.
    uint16_t backend_sess_id;    // Backend session ID, used to match frontend and backend sessions in the backend proxy.
    uint16_t payload_len;        // Payload length, range from 1 to 4088, ensure it does not exceed one physical page.
} __attribute__((packed)) ProxyMsgHeader;


typedef enum {
    ACTION_TYPE_COMMAND = 0,  // Command
    ACTION_TYPE_RESPONSE      // Response
} ActionType;


typedef struct {
    uint16_t version;        // Protocol version, not used currently, set to 1.
    uint16_t msg_type;       // Message type, values: Disable (0), Enable (1), Query (2)
    uint16_t msg_id;         // Message ID, used to match commands and responses
    uint16_t action_type;    // Signaling type: Command (0) / Response (1) 
    uint16_t payload_len;    // Payload length
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


#if 0
// Payload length of device message
#define DEV_MSG_PAYLOAD_LEN(dev_msg_type, action_type) \
((dev_msg_type == DEV_MSG_ENABLE) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 2 : ((action_type == ACTION_TYPE_RESPONSE) ? 2 : -1)) : \
((dev_msg_type == DEV_MSG_DISABLE) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 2 : ((action_type == ACTION_TYPE_RESPONSE) ? 2 : -1)) : \
((dev_msg_type == DEV_MSG_QUERY) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 0 : ((action_type == ACTION_TYPE_RESPONSE) ? 4 : -1)) : \
-1)))
#endif

#if 1
#define DEV_MSG_PAYLOAD_LEN(dev_msg_type, action_type) \
( \
    /* Check if device message type is valid */ \
    (dev_msg_type == DEV_MSG_ENABLE) ? \
        ( \
            /* Check if action type is valid */ \
            (action_type == ACTION_TYPE_COMMAND)  ? 2 : \
            (action_type == ACTION_TYPE_RESPONSE) ? 2 : \
            -1  /* Invalid action type */ \
        ) : \
    (dev_msg_type == DEV_MSG_DISABLE) ? \
        ( \
            (action_type == ACTION_TYPE_COMMAND)  ? 2 : \
            (action_type == ACTION_TYPE_RESPONSE) ? 2 : \
            -1  /* Invalid action type */ \
        ) : \
    (dev_msg_type == DEV_MSG_QUERY) ? \
        ( \
            (action_type == ACTION_TYPE_COMMAND)  ? 0 : \
            (action_type == ACTION_TYPE_RESPONSE) ? 4 : \
            -1  /* Invalid action type */ \
        ) : \
    -1  /* Invalid device message type */ \
)
#endif

typedef struct {
    uint8_t status;    // Status code
    uint8_t error;     // Error code
    uint8_t data[0];   // Placeholder. data points to the response of the "Query" command, which returns the device code.
} __attribute__((packed)) DevMsgReport;


typedef struct {
    uint16_t version;       // Protocol version, not in use currently, set to 1
    uint16_t msg_type;      // Message type, values: Set (0), Query (1)
    uint16_t msg_id;        // Message ID, used to match commands and responses
    uint16_t action_type;   // Signaling type: Command (0) / Response (1) 
    uint16_t payload_len;   // Payload length
} __attribute__((packed)) StrgyMsgHeader;


typedef enum {
    STRAT_MSG_SET = 0,       // Set
    STRAT_MSG_QUERY          // Query
} StrgyMsgType;


// Check if the strategy message type is valid
#define IS_VALID_STRAT_MSG_TYPE(strat_msg_type) \
    ((strat_msg_type) == STRAT_MSG_SET || \
     (strat_msg_type) == STRAT_MSG_QUERY)

// Payload length of strategy message
#define STRAT_MSG_PAYLOAD_LEN(strat_msg_type, action_type) \
((strat_msg_type == STRAT_MSG_SET) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 2 : ((action_type == ACTION_TYPE_RESPONSE) ? 2 : -1)) : \
((strat_msg_type == STRAT_MSG_QUERY) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 0 : ((action_type == ACTION_TYPE_RESPONSE) ? 4 : -1)) : \
-1))

typedef struct {
    StrgyMsgHeader header;   // Strategy message header
    uint16_t cmd_type;       // Command type. 0: Enable specified strategy; 1: Query current strategy
    uint16_t strat_para;     //  Strategy parameter (0: Round Robin; 1: Select device with highest current available bandwidth; 2: Select device with lowest current latency)
} __attribute__((packed))StrgyCMDMessage;


typedef struct {
    uint8_t status;     // Status code
    uint8_t error;      // Error code
    uint8_t data[];     //  Placeholder. data points to the response of the "Query" command and returns the active strategy code.
} __attribute__((packed))StrgyMsgReport;

typedef struct {
    uint16_t version;        // Protocol version, not in use currently, set to 1
    uint16_t msg_type;       // Message type, values: Create (0), Close (1)
    uint16_t action_type;    // Signaling type: Command (0) / Response (1)
    uint16_t payload_len;    // Payload length
} __attribute__((packed)) SessMsgHeader;

typedef enum {
    SESS_MSG_CREATE = 0,       // Create
    SESS_MSG_CLOSE             // Close
} SessMsgType;


typedef enum {
    SESS_NON_IP_PROTO = 0,       // None-IP protocol
    SESS_IPV4_PROTO   = 4,       // IPv4
    SESS_IPV6_PROTO   = 6        // IPv6
} SessIpProtoVersion;


typedef enum {
    SESS_UDP_PROTO = 0,            // UDP
    SESS_TCP_PROTO = 1,            // TCP
    SESS_FASTPATH_PROTO = 2        // XDP/eBPF
} SessTranProto;

// Check if the session message type is valid
#define IS_VALID_SESS_MSG_TYPE(sess_msg_type) \
    ((sess_msg_type) == SESS_MSG_CREATE || \
     (sess_msg_type) == SESS_MSG_CLOSE)


// Check if the IP protocol verion is valid
#define IS_VALID_SESS_IP_VERSION(ip_version) \
    ((ip_version) == SESS_IPV4_PROTO || \
     (ip_version) == SESS_IPV6_PROTO)


// Payload length of session message
#define SESS_MSG_PAYLOAD_LEN(sess_msg_type, action_type, ip_version) \
({ \
    int _len = -1;  /* Initial value: invalid length */ \
    if ((sess_msg_type) == SESS_MSG_CLOSE) { \
        if ((action_type) == ACTION_TYPE_COMMAND) { \
            _len = 0;  /* Close + Command → No content */ \
        } else if ((action_type) == ACTION_TYPE_RESPONSE) { \
            _len = 2;  /* Close + Response → 2 bytes (status code + reason) */ \
        } \
    } else if ((sess_msg_type) == SESS_MSG_CREATE) { \
        if ((action_type) == ACTION_TYPE_RESPONSE) { \
            _len = 2;  /* Create + Response → 2 bytes (status code + reason) */ \
        } else if ((action_type) == ACTION_TYPE_COMMAND) { \
            if ((ip_version) == SESS_IPV4_PROTO) { \
                _len = 10; /* IPv4 → 10 bytes of session information */ \
            } else if ((ip_version) == SESS_IPV6_PROTO) { \
                _len = 22; /* IPv6 → 22 bytes of session information */ \
            } \
        } \
    } \
    _len;  /* Return the calculated result */ \
})


typedef struct {
    uint8_t ip_proto_ver;    // IP protocol version, values: IPv4 (4) and IPv6 (6)
    uint16_t trans_proto;    // Transport layer type, values: UDP (0) and TCP (1);
    uint16_t dev_id;         // Device ID; when set to 0xFF, indicates entering vertical handover mode.
    uint8_t  data[0];        // Placeholder. Interpreted as IPv4PortTurple or IPv6PortTurple based on IP version.
} __attribute__((packed)) SessMsgPara;

typedef struct {
    uint32_t ipv4;        // IPv4 address
    uint16_t port;   	  // Transport layer port；
} __attribute__((packed)) IPv4PortTurple;

typedef struct {
    uint8_t ipv6[16];      // IPv6 address
    uint16_t port;   	   // Transport layer port
} __attribute__((packed)) IPv6PortTurple;

#endif