#ifndef DEV_H
#define DEV_H

#include <stdint.h> 
#include <sys/queue.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include "message.h"

#define MAX_DEV_NAME                            32  // Maximum length of device name
#define MAX_HS_DEV_NUM                          32
#define MAX_DEVICE_PROPERTY_NAME_LENGTH         8


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

struct TcpConnection {
    int conn_fd;  
    TAILQ_ENTRY(TcpConnection) entry;  // Linked list node
};

TAILQ_HEAD(ConnectionQueue, TcpConnection);


#define ERROR_NAMESPACE_ID                      -1

struct HighSpeedNetDevice {
// Device identification information
    int dev_id;
    int dev_type;
    int ns_id;
    int dev_status;
// Device attribute information
    char name[MAX_DEV_NAME];
// Network connection information
    struct ConnectionQueue conn_q;
    union IPAddress address;
// Performance statistics information
    struct HighSpeedNetDevStat stat;
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

#endif /* DEV_H */