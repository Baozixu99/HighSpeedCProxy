#ifndef DEV_H
#define DEV_H

#include <stdint.h> 
#include <sys/queue.h>

#define MAX_DEV_NAME 32  // Maximum length of device name
#define MAX_HS_DEV_NUM 16

struct IPv4Address {
    uint8_t data[4];  
};

struct IPv6Address {
    uint8_t data[16]; 
};

union IPAddressData {
    struct IPv4Address ipv4_addr;
    struct IPv6Address ipv6_addr; 
};

struct IPAddress {
    int dev_id; 
    union IPAddressData data;
};


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


struct HighSpeedNetDevice {
// Device identification information
    int dev_id;
    int dev_type;
    int ns_id;
    int active;
// Device attribute information
    char name[MAX_DEV_NAME];
// Network connection information
    struct ConnectionQueue conn_q;
    struct IPAddress address;
// Performance statistics information
    struct HighSpeedNetDevStat stat;
};

struct HighSpeedNetDeviceSet {
    struct HighSpeedNetDevice hs_net_dev[MAX_HS_DEV_NUM];
};

#endif /* DEV_H */