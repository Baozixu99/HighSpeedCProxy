#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

typedef struct {
    uint8_t version;             // 协议版本，目前不用管，设置为1
    uint8_t proxy_msg_type;      // 代理消息类型，分为设备消息（0）、策略消息（1）、会话消息（2）和数据消息（3）
    uint16_t frontend_sess_id;   // 消息ID，用于匹配请求和响应
    uint16_t backend_sess_id;    // 消息ID，用于匹配请求和响应
    uint16_t payload_len;        // 负载长度，取值范围为1~4088，确保不超过一个物理页。
} __attribute__((packed)) ProxyMsgHeader;


typedef struct {
    uint16_t version;        // 协议版本，目前不用管，设置为1
    uint16_t msg_type;       // 消息类型，分为禁用（0）、启用（1）和查询（2）
    uint16_t msg_id;         // 消息ID，用于匹配指令和回应
    uint16_t action_type;    // 指令（0）或者回应（1）。 
    uint16_t payload_len;    // 负载长度
} __attribute__((packed)) DevMsgHeader;


typedef struct {
    uint8_t status;    // 状态码
    uint8_t error;     //  错误原因
    uint8_t data[0];   //  占位。data指向「查询」指令的「回应」会返回设备码。
} __attribute__((packed)) DevMsgReport;


typedef struct {
    uint16_t version;       // 协议版本，目前不用管，设置为1
    uint16_t msg_type;      // 消息类型，分为配置（0）和查询（1）
    uint16_t msg_id;        // 消息ID，用于匹配请求和响应
    uint16_t action_type;   // 指令（0）或者回应（1）。 
    uint16_t payload_len;   // 负载长度。
} __attribute__((packed)) PolicyMsgHeader;

typedef struct {
    PolicyMsgHeader header;   // 消息头部
    uint16_t cmd_type;        // 命令类型。0启用指定策略，1查询当前策略
    uint16_t policy_para;     // 策略参数（0：Round Robin；1：选取当前可用带宽最高设备；2.选取当前延时最低设备）
} __attribute__((packed))PolicyCMDMessage;


typedef struct {
    uint8_t status;     // 状态码
    uint8_t error;      //  错误原因
    uint8_t data[];     //  占位。data指向「查询」指令的「回应」会返回策略码。
} __attribute__((packed))PolicyMsgReport;

typedef struct {
    uint16_t version;        // 协议版本，目前不用管，设置为1
    uint16_t msg_type;       // 消息类型，分为新建（0）和关闭（1）；
    uint16_t action_type;    // 指令（0）或者回应（1）。
    uint16_t payload_len;    // 负载长度
} __attribute__((packed)) SessMsgHeader;

typedef struct {
    uint8_t ip_proto;        // IP协议版本，分为IPv4（0）和IPv6（1）
    uint16_t trans_proto;    // 传输层类型，分为UDP（0）和TCP（1）；
    uint16_t dev_id;         // 设备ID，选择为0xFF时，代表进入垂直切换模式。
    uint8_t  data[0];        // 占位。根据IP版本选择IPv4PortTurple或者IPv6PortTurple解读。
} __attribute__((packed)) SessMsgPara;

typedef struct {
    uint32_t ipv4;        // IPv4地址
    uint16_t port;   	   // 传输层端口；
} __attribute__((packed)) IPv4PortTurple;

typedef struct {
    uint8_t ipv6[16];        // IPv6地址
    uint16_t port;   	   // 传输层端口；
} __attribute__((packed)) IPv6PortTurple;

#endif