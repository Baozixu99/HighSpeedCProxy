#ifndef DEV_H
#define DEV_H

#include <stdint.h> 
#include <sys/queue.h>

#define MAX_DEV_NAME 32  // 设备名称最大长度
#define MAX_HS_DEV_NUM 8

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
    uint64_t rx_packets;   // 接收数据包数量
    uint64_t rx_bytes;     // 接收字节数
    uint64_t rx_errors;    // 接收错误数
    uint64_t tx_packets;   // 发送数据包数量
    uint64_t tx_bytes;     // 发送字节数
    uint64_t tx_errors;    // 发送错误数
    uint64_t rtt_mean;     // 往返时间平均值（ms）
    uint64_t rtt_std;      // 往返时间标准差
};

struct TcpConnection {
    int conn_fd;  
    TAILQ_ENTRY(TcpConnection) entry;  // 链表节点
};

TAILQ_HEAD(ConnectionQueue, TcpConnection);

struct HighSpeedNetDevice {
    // 设备标识信息
    int dev_id;
    int dev_type;
    int ns_id;
    
    // 设备属性信息
    char name[MAX_DEV_NAME];
    
    // 网络连接信息
    struct ConnectionQueue conn_q;
    struct IPAddress address;         
    // 性能统计信息
    struct HighSpeedNetDevStat stat;
};


struct HighSpeedNetDeviceSet {
    struct HighSpeedNetDevice hs_net_dev[MAX_HS_DEV_NUM];
};

#endif /* DEV_H */