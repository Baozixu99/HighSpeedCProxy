#ifndef DEV_H
#define DEV_H

#include <stdint.h> 
#include <sys/queue.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include "message.h"
#include "session.h"


#define MAX_DEV_NAME                            32  // Maximum length of device name
#define MAX_HS_DEV_NUM                          16
#define MAX_IOT_DEV_NUM                         16
#define MAX_DEVICE_PROPERTY_NAME_LENGTH         8
#define MAX_SESS_NODE_NUM                       10

#define DEV_ID_AUTO_HANDOVER                    0xFF

/*
 * High Speed Network device type enumeration
 * Contains five common types of network devices
 */
typedef enum {
    TRADITIONAL_ETHERNET = 0,   // Traditional Ethernet device
    TSN,                        // Time-Sensitive Networking device
    WIFI,                       // WiFi wireless network device
    LTE_MODULE,                 // 4G module (LTE technology standard)
    NR_MODULE                   // 5G module (NR technology standard)
} HSNetDevType;



/**
 * Macro to check if a device type is one of the defined network device types
 * @param dev_type The device type to check
 * @return 1 if valid, 0 otherwise
 */
#define IS_VALID_HS_NET_DEV_TYPE(dev_type) \
    ((dev_type) == TRADITIONAL_ETHERNET || \
     (dev_type) == TSN || \
     (dev_type) == WIFI || \
     (dev_type) == LTE_MODULE || \
     (dev_type) == NR_MODULE)


/*
 * High Speed Network Device status enumeration
 * Represents the possible operational states of a device
 */
typedef enum {
    HS_NET_DEV_INACTIVE = 0,         // High speed network device is inactive and operating normally
    HS_NET_DEV_ACTIVE,               // High speed network device is active and operating normally
} HSNetDevStatus;

/**
 * Macro to check if a HSNetDevStatus value is valid
 * @param dev_status The HSNetDevStatus value to check
 * @return 1 if valid (one of the defined enum values), 0 otherwise
 */
#define IS_VALID_HS_NET_DEV_STATUS(dev_status) \
    ((dev_status) == HS_NET_DEV_INACTIVE || (dev_status) == HS_NET_DEV_ACTIVE)


struct HighSpeedNetDevStat {
    uint64_t rx_packets; // Number of received packets
    uint64_t rx_bytes; // Number of received bytes
    uint64_t rx_errors; // Number of receive errors
    uint64_t tx_packets; // Number of transmitted packets
    uint64_t tx_bytes; // Number of transmitted bytes
    uint64_t tx_errors; // Number of transmit errors
    uint64_t rtt_mean; // Mean round-trip time (ms)
    uint64_t rtt_std; // Standard deviation of round-trip time
};


struct SessionNode {
    struct BackendSession   *sess;
    TAILQ_ENTRY(SessionNode) entry;  // Linked list node
};


TAILQ_HEAD(NodeQueue, UdpNode);
TAILQ_HEAD(ConnectionQueue, TcpConnection);
TAILQ_HEAD(SessionNodeQueue, SessionNode);


#define ERROR_NAMESPACE_ID                      -1

struct HighSpeedNetDevice {
// Device identification information
    int                         dev_id;
    int                         dev_type;
    const char                  *ns_name;
    int                         ns_id;
    int                         dev_status;
// Device attribute information
    char    name[MAX_DEV_NAME];
// Network connection information
    struct  SessionNodeQueue     udp_node_queue;
    struct  SessionNodeQueue     tcp_node_queue;
    struct  SessionNodeQueue     free_node_queue;
    union   IPAddress           address;
// Performance statistics information
    struct  HighSpeedNetDevStat stat;
// Tcp listening port and socket
    int                         tcp_listening_port;
    int                         tcp_listener;
};

struct HighSpeedNetDeviceSet {
    struct HighSpeedNetDevice hs_net_dev[MAX_HS_DEV_NUM];
};


#define GET_NS_ID(net_dev, dev_id)                       \
    ({                                                   \
        int _ns_id = ERROR_NAMESPACE_ID;                 \
        do {                                             \
            /* Check net_dev pointer validity */         \
            assert(net_dev != NULL && "net_dev is NULL");\
            /* Check dev_id range (0 <= dev_id < maximum device count) */ \
            assert((dev_id >= 0) && (dev_id < MAX_HS_DEV_NUM) && "invalid dev_id"); \
            /* Check if device status is active */       \
            assert(net_dev->hs_net_dev[dev_id].dev_status == HS_NET_DEV_ACTIVE && \
                   "device is not active");               \
            /* All checks passed, get ns_id */            \
            _ns_id = net_dev->hs_net_dev[dev_id].ns_id;   \
        } while(0);                                       \
        _ns_id;                                           \
    })


/**
 * @def HIGH_SPEED_NET_DEV_POP_FREE_SESSION_NODE(_dev)
 * @brief Pop and remove a struct SessionNode from the free_node queue of struct HighSpeedNetDevice
 * @details This macro safely acquires the head node of the free_node TAILQ queue, and removes the node from the queue if it exists.
 *          It provides null pointer protection to avoid program crash caused by invalid pointer access.
 * @param _dev  Pointer to struct HighSpeedNetDevice (NULL is allowed, will return NULL directly if _dev is NULL)
 * @return struct SessionNode* 
 *         - Non-NULL: Successfully get a free SessionNode (has been removed from free_node queue)
 *         - NULL: free_node queue is empty OR input _dev pointer is invalid (NULL)
 */
#define HIGH_SPEED_NET_DEV_POP_FREE_SESSION_NODE(_dev)                          \
({                                                                              \
    struct SessionNode *__ret_node = NULL;                                      \
    /* 1. Check if the input pointer is valid to avoid null pointer dereference */ \
    if ((_dev) != NULL) {                                                       \
        /* 2. Get the head node of the free_node queue (standard TAILQ operation) */ \
        __ret_node = TAILQ_FIRST(&(_dev)->free_node_queue);                           \
        /* 3. If the node exists, remove it from the free_node queue */          \
        if (__ret_node != NULL) {                                               \
            TAILQ_REMOVE(&(_dev)->free_node_queue, __ret_node, entry);                 \
        }                                                                       \
    }                                                                           \
    __ret_node; /* Return the acquired node (NULL means invalid/empty queue) */  \
})


/**
 * @def HIGH_SPEED_NET_DEV_PUSH_FREE_SESSION_NODE(_dev, _node)
 * @brief Push a struct SessionNode back to the free_node queue of struct HighSpeedNetDevice (reverse operation of HIGH_SPEED_NET_DEV_POP_FREE_SESSION_NODE)
 * @details This macro safely inserts the specified SessionNode into the tail of free_node TAILQ queue after parameter validity check.
 *          It is the exact reverse operation of popping a node from free_node, and is used to recycle unused SessionNode instances.
 * @param _dev  Pointer to struct HighSpeedNetDevice (NULL is allowed, no operation will be performed if _dev is NULL)
 * @param _node Pointer to struct SessionNode to be pushed back (NULL is allowed, no operation will be performed if _node is NULL)
 * @note No return value; only performs insertion when both _dev and _node are valid
 */
#define HIGH_SPEED_NET_DEV_PUSH_FREE_SESSION_NODE(_dev, _node)                  \
do {                                                                           \
    /* Parameter validity check: avoid null pointer dereference */             \
    if ((_dev) != NULL && (_node) != NULL) {                                   \
        /* Insert the node to the tail of free_node queue (standard TAILQ operation) */ \
        TAILQ_INSERT_TAIL(&(_dev)->free_node_queue, (_node), entry);                 \
        /* Optional: Reset sess pointer to NULL (recommended for node recycling) */ \
        /* (_node)->sess = NULL; */                                             \
    }                                                                           \
} while (0)

/**
 * @def HIGH_SPEED_NET_DEV_INSERT_SESSION_NODE(_dev, _node, _proto_type)
 * @brief Insert a struct SessionNode into the corresponding queue (udp_node_queue/tcp_node_queue) of struct HighSpeedNetDevice according to proto_type
 * @details 1. Comprehensive parameter validity check (dev/node/sess non-NULL, proto_type is SESS_UDP_PROTO/SESS_TCP_PROTO).
 *          2. Insert condition: Parameters are valid AND sess_dev_link_state of _node->sess is NOT set with BACKEND_SESS_LINKED_TO_DEV_NODE.
 *          3. Post-insert operation: Automatically set BACKEND_SESS_LINKED_TO_DEV_NODE in sess_dev_link_state after successful insertion.
 *          4. Return rule: 
 *             - BACKEND_PROXY_PROCESS_OK: Parameters are valid (whether inserted or not, even if sess_dev_link_state is already set).
 *             - BACKEND_PROXY_PROCESS_ERROR: Invalid parameters or unsupported proto_type.
 * @param _dev        Pointer to struct HighSpeedNetDevice (non-NULL required)
 * @param _node       Pointer to struct SessionNode to be inserted (non-NULL required, and _node->sess must be non-NULL)
 * @param _proto_type Protocol type (must be SESS_UDP_PROTO or SESS_TCP_PROTO)
 * @return int        BACKEND_PROXY_PROCESS_OK: Valid parameters (inserted and state set, or already linked); BACKEND_PROXY_PROCESS_ERROR: Invalid parameter or unsupported proto_type
 */
#define HIGH_SPEED_NET_DEV_INSERT_SESSION_NODE(_dev, _node, _proto_type)       \
({                                                                              \
    int __ret_code = BACKEND_PROXY_PROCESS_ERROR;                              \
    /* 1. Comprehensive parameter validity check (avoid null pointer dereference) */\
    if ((_dev) != NULL && (_node) != NULL && (_node)->sess != NULL) {          \
        /* 2. Judge if proto_type is in supported range */                       \
        if (((_proto_type) == SESS_UDP_PROTO) || ((_proto_type) == SESS_TCP_PROTO)) { \
            __ret_code = BACKEND_PROXY_PROCESS_OK; /* Valid params, return OK unconditionally */ \
            /* 3. Check if sess_dev_link_state does NOT have the state bit set */ \
            if (!((_node)->sess->sess_dev_link_state & BACKEND_SESS_LINKED_TO_DEV_NODE)) { \
                /* 4. Insert into corresponding queue (udp_node_queue/tcp_node_queue) */ \
                if ((_proto_type) == SESS_UDP_PROTO) {                         \
                    TAILQ_INSERT_TAIL(&(_dev)->udp_node_queue, (_node), entry); \
                } else { /* Exclusively SESS_TCP_PROTO (already verified) */    \
                    TAILQ_INSERT_TAIL(&(_dev)->tcp_node_queue, (_node), entry); \
                }                                                               \
                /* 5. Set BACKEND_SESS_LINKED_TO_DEV_NODE bit after successful insertion (core requirement) */ \
                (_node)->sess->sess_dev_link_state |= BACKEND_SESS_LINKED_TO_DEV_NODE; \
            }                                                                   \
            /* If state is already set: do nothing, keep return code as OK */    \
        }                                                                       \
    }                                                                           \
    __ret_code; /* Return final operation status code */                        \
})


/**
 * @def HIGH_SPEED_NET_DEV_REMOVE_SESSION_NODE(_dev, _node, _proto_type)
 * @brief Remove a struct SessionNode from the corresponding queue (udp_node_queue/tcp_node_queue) of struct HighSpeedNetDevice according to proto_type
 * @details 1. Comprehensive parameter validity check (dev/node/sess non-NULL, proto_type is SESS_UDP_PROTO/SESS_TCP_PROTO) first.
 *          2. Remove condition: Parameters are valid AND sess_dev_link_state of _node->sess is set with BACKEND_SESS_LINKED_TO_DEV_NODE.
 *          3. Post-remove operation: Automatically clear BACKEND_SESS_LINKED_TO_DEV_NODE bit in sess_dev_link_state after successful removal.
 *          4. No return value: Only execute the removal and state clear operation when conditions are met, do nothing if any condition is not satisfied.
 * @param _dev        Pointer to struct HighSpeedNetDevice (non-NULL required)
 * @param _node       Pointer to struct SessionNode to be removed (non-NULL required, and _node->sess must be non-NULL)
 * @param _proto_type Protocol type (must be SESS_UDP_PROTO or SESS_TCP_PROTO)
 */
#define HIGH_SPEED_NET_DEV_REMOVE_SESSION_NODE(_dev, _node, _proto_type)       \
do {                                                                           \
    /* 1. Comprehensive parameter validity check (avoid null pointer dereference) */ \
    if ((_dev) != NULL && (_node) != NULL && (_node)->sess != NULL) {          \
        /* 2. Judge if proto_type is in supported range */                       \
        if (((_proto_type) == SESS_UDP_PROTO) || ((_proto_type) == SESS_TCP_PROTO)) { \
            /* 3. Check if sess_dev_link_state has the state bit set (only remove when linked) */ \
            if (((_node)->sess->sess_dev_link_state & BACKEND_SESS_LINKED_TO_DEV_NODE)) { \
                /* 4. Remove from corresponding queue (udp_node_queue/tcp_node_queue) */ \
                if ((_proto_type) == SESS_UDP_PROTO) {                         \
                    TAILQ_REMOVE(&(_dev)->udp_node_queue, (_node), entry);     \
                } else { /* Exclusively SESS_TCP_PROTO (already verified) */    \
                    TAILQ_REMOVE(&(_dev)->tcp_node_queue, (_node), entry);     \
                }                                                               \
                /* 5. Clear BACKEND_SESS_LINKED_TO_DEV_NODE bit after successful removal (logic closure) */ \
                (_node)->sess->sess_dev_link_state &= ~BACKEND_SESS_LINKED_TO_DEV_NODE; \
            }                                                                   \
            /* If state is not set: do nothing */                               \
        }                                                                       \
    }                                                                           \
} while (0)


/**
 * @brief IoT device type enumeration (matches IotProtoType in message definition)
 */
typedef enum {
    IOT_DEV_TYPE_UNKNOWN = 0,    // Unknown IoT device type
    IOT_DEV_TYPE_BLUETOOTH,      // Bluetooth device (BLE/Classic Bluetooth)
    IOT_DEV_TYPE_ZIGBEE,         // Zigbee device (802.15.4)
    IOT_DEV_TYPE_CAN,            // CAN bus device (CAN 2.0/CAN FD)
    IOT_DEV_TYPE_LORA,           // LoRa/LoRaWAN device
    IOT_DEV_TYPE_POWERLINK,      // OpenPowerLink (Ethernet POWERLINK) device
    IOT_DEV_TYPE_MODBUSTCP       //  Modbus TCP device (Modbus protocol over TCP/IP).
} IotDevType;

/**
 * @brief IoT device status enumeration
 */
typedef enum IotDevStatus_{
    IOT_DEV_STATUS_OFFLINE = 0,  // Device offline (disconnected/unavailable)
    IOT_DEV_STATUS_ONLINE,       // Device online (connected/available)
    IOT_DEV_STATUS_ERROR,        // Device error (fault/abnormal state)
    IOT_DEV_STATUS_CONFIGURING   // Device being configured (temporary state)
} IotDevStatus;

/**
 * @brief Bluetooth device specific parameters
 */
typedef struct BluetoothDevAttr_{
    uint16_t    bt_port;         // Bluetooth port/channel number
    uint8_t     bt_mac[6];       // Bluetooth MAC address (6 bytes)
    uint8_t     bt_version;      // Bluetooth version (0x01=BLE 5.0, 0x02=BLE 5.1, etc.)
    uint16_t    conn_interval;   // BLE connection interval (unit: ms)
} BluetoothDevAttr;

/**
 * @brief CAN device specific parameters
 */
typedef struct CANDevAttr_{
    uint16_t    can_port;        // CAN port number
    uint32_t    can_bitrate;     // CAN bus bitrate (bps, e.g., 500000 for 500Kbps)
    uint8_t     can_mode;        // CAN mode (0x01=normal, 0x02=loopback, 0x03=silent)
    uint32_t    can_filter_id;   // CAN filter ID (for frame filtering)
} CANDevAttr;

/**
 * @brief Zigbee device specific parameters
 */
typedef struct ZigbeeDevAttr_{
    uint16_t    zigbee_pan_id;   // Zigbee PAN ID
    uint8_t     zigbee_channel;  // Zigbee channel (range: 11-26)
    uint8_t     zigbee_mac[8];   // Zigbee MAC address (8 bytes)
    uint8_t     zigbee_role;     // Zigbee role (0x01=coordinator, 0x02=router, 0x03=end device)
} ZigbeeDevAttr;

/**
 * @brief LoRa device specific parameters
 */
typedef struct LoRaDevAttr_{
    uint16_t    lora_port;       // LoRa port number
    uint8_t     lora_freq_band;  // LoRa frequency band (0x01=EU868, 0x02=US915, 0x03=CN470, etc.)
    uint8_t     lora_sf;         // LoRa spreading factor (range: 7-12)
    uint8_t     lora_cr;         // LoRa coding rate (range: 1-4, corresponds to 4/5 ~ 4/8)
    uint32_t    lora_dev_eui;    // LoRa device EUI (unique identifier)
} LoRaDevAttr;

/**
 * @brief OpenPowerLink device specific parameters (Industrial Real-Time Ethernet)
 */
typedef struct PowerLinkDevAttr_ {
    uint16_t    plk_port;        // POWERLINK port number (corresponding to Ethernet interface)
    uint8_t     plk_mac[6];      // Device MAC address (6 bytes)
    uint16_t    plk_node_id;     // POWERLINK NodeID (range: 1-240)
    uint8_t     plk_role;        // POWERLINK role (0: MN (Managing Node), 1: CN (Controlled Node))
    uint32_t    plk_cycle_ms;    // Real-time cycle time (unit: ms, typically 1~10ms)
    uint16_t    plk_rx_pdo_len;  // Length of received PDO (Process Data Object)
    uint16_t    plk_tx_pdo_len;  // Length of transmitted PDO (Process Data Object)
} PowerLinkDevAttr;


/**
 * @brief ModbusTCP device specific parameters (Industrial Ethernet)
 */
typedef struct ModbusTCPDevAttr_{
    uint16_t    mb_port;         // ModbusTCP port number (default: 502)
    uint8_t     mb_mac[6];       // Device MAC address (6 bytes)
    uint8_t     mb_unit_id;      // Modbus Unit ID / Slave ID (range: 0-255)
    uint8_t     mb_proto_ver;    // Modbus protocol version (0x00: ModbusTCP, 0x01: ModbusRTU over TCP)
    uint32_t    mb_timeout_ms;   // ModbusTCP communication timeout (unit: ms)
    uint8_t     mb_retry_cnt;    // Modbus request retry count (0 = no retry)
} ModbusTCPDevAttr;

/**
 * @brief Union for IoT device specific attributes (memory optimization)
 */
typedef union {
    BluetoothDevAttr    bt_attr;         // Bluetooth device attributes
    CANDevAttr          can_attr;        // CAN device attributes
    ZigbeeDevAttr       zigbee_attr;     // Zigbee device attributes
    LoRaDevAttr         lora_attr;       // LoRa device attributes
    PowerLinkDevAttr    plk_attr;        // OpenPowerLink device attributes
    ModbusTCPDevAttr    mb_attr;         // ModbusTCP device attributes
} IotDevSpecificAttr;

/**
 * @brief IoT device performance statistics
 */
typedef struct IotDevStat_{
    uint64_t    tx_packets;      // Total transmitted packets
    uint64_t    rx_packets;      // Total received packets
    uint64_t    tx_bytes;        // Total transmitted bytes
    uint64_t    rx_bytes;        // Total received bytes
    uint32_t    error_count;     // Total error count
    uint64_t    last_active_ts;  // Last active timestamp (ms since epoch)
} IotDevStat;

/**
 * @brief IoT device configuration (loaded from config file, initialized on startup)
 */
typedef struct {
    int         working_mode;    // 0=client mode, 1=server/gateway mode
    char        config_path[256];// Path to device configuration file
    int         auto_connect;    // 0=manual connect, 1=auto connect on startup
} IotDevConfig;

/**
 * @brief Main structure for IoT device (Bluetooth/CAN/Zigbee/LoRa/POWERLINK)
 * @note Refer to HighSpeedNetDevice design, optimized for IoT characteristics
 */
typedef struct IotDevice_ {
    // Device identification information
    int                         dev_id;             // Unique device ID (global)
    int                         dev_type;           // IoT device type, equal to IoT Protocol type.
    char                        *ns_name;           // Namespace name (for device grouping)
    IotDevStatus                dev_status;         // Device online/offline status

    // Device attribute information
    char                        name[MAX_DEV_NAME]; // Device name (human-readable)
    IotDevConfig                config;             // Device configuration (from config file)
    IotDevSpecificAttr          specific_attr;      // Protocol-specific attributes

    // Session connection information
    struct SessionNodeQueue     iot_sess_queue;     // IoT session queue (single queue for simplicity)
    int                         sess_id;            // Associated session ID (initialized on startup)

    // Performance statistics information
    IotDevStat                  stat;               // Device performance statistics

    // Device hardware/port information
    int                         physical_port;      // Physical port number (e.g., /dev/ttyUSB0 mapped to ID)
    int                         fd;                 // Device file descriptor (for hardware access)
} IotDevice;



typedef struct IoTDeviceSet_ {
    IotDevice iot_dev[MAX_IOT_DEV_NUM];
}IoTDeviceSet;

#endif /* DEV_H */