#ifndef DEV_H
#define DEV_H

#include <stdint.h> 
#include <sys/queue.h>
#include <string.h>
#include <netinet/in.h>

#define MAX_DEV_NAME                            32  // Maximum length of device name
#define MAX_HS_DEV_NUM                          16
#define MAX_DEVICE_PROPERTY_NAME_LENGTH         8

struct IPv4Address;
struct IPv6Address;
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