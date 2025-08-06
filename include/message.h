#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

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
    uint8_t version;             // Protocol version, not used currently, set to 1
    uint8_t proxy_msg_type;      // Proxy message type, divided into device message (0), strategy message (1), session message (2), and data message (3)
    uint16_t frontend_sess_id;   // 消息ID，用于匹配请求和响应
    uint16_t backend_sess_id;    // 消息ID，用于匹配请求和响应
    uint16_t payload_len;        // 负载长度，取值范围为1~4088，确保不超过一个物理页。
} __attribute__((packed)) ProxyMsgHeader;


typedef enum {
    ACTION_TYPE_COMMAND = 0,  // 指令
    ACTION_TYPE_RESPONSE      // 回应
} ActionType;


typedef struct {
    uint16_t version;        // 协议版本，目前不用管，设置为1
    uint16_t msg_type;       // 消息类型，分为禁用（0）、启用（1）和查询（2）
    uint16_t msg_id;         // 消息ID，用于匹配指令和回应
    uint16_t action_type;    // 指令（0）或者回应（1）。 
    uint16_t payload_len;    // 负载长度
} __attribute__((packed)) DevMsgHeader;


typedef enum {
    DEV_MSG_DISABLE = 0,  // 禁用
    DEV_MSG_ENABLE,       // 启用
    DEV_MSG_QUERY         // 查询
} DevMsgType;

// 检查设备消息类型是否合法
#define IS_VALID_DEV_MSG_TYPE(dev_msg_type) \
    ((dev_msg_type) == DEV_MSG_DISABLE || \
     (dev_msg_type) == DEV_MSG_ENABLE || \
     (dev_msg_type) == DEV_MSG_QUERY)


// 设备消息载荷长度
#define DEV_MSG_PAYLOAD_LEN(dev_msg_type, action_type) \
((dev_msg_type == DEV_MSG_ENABLE) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 2 : ((action_type == ACTION_TYPE_RESPONSE) ? 2 : -1)) : \
((dev_msg_type == DEV_MSG_DISABLE) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 2 : ((action_type == ACTION_TYPE_RESPONSE) ? 2 : -1)) : \
((dev_msg_type == DEV_MSG_QUERY) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 0 : ((action_type == ACTION_TYPE_RESPONSE) ? 4 : -1)) : \
-1)))


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
} __attribute__((packed)) StratMsgHeader;


typedef enum {
    STRAT_MSG_SET = 0,       // 设置
    STRAT_MSG_QUERY          // 查询
} StrgyMsgType;


// 检查策略消息类型是否合法
#define IS_VALID_STRAT_MSG_TYPE(strat_msg_type) \
    ((strat_msg_type) == STRAT_MSG_SET || \
     (strat_msg_type) == STRAT_MSG_QUERY)

// 策略消息载荷长度
#define STRAT_MSG_PAYLOAD_LEN(strat_msg_type, action_type) \
((strat_msg_type == STRAT_MSG_SET) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 2 : ((action_type == ACTION_TYPE_RESPONSE) ? 2 : -1)) : \
((strat_msg_type == STRAT_MSG_QUERY) ? \
    ((action_type == ACTION_TYPE_COMMAND) ? 0 : ((action_type == ACTION_TYPE_RESPONSE) ? 4 : -1)) : \
-1))

typedef struct {
    StratMsgHeader header;   // 消息头部
    uint16_t cmd_type;        // 命令类型。0启用指定策略，1查询当前策略
    uint16_t strgy_para;     // 策略参数（0：Round Robin；1：选取当前可用带宽最高设备；2.选取当前延时最低设备）
} __attribute__((packed))StrgyCMDMessage;


typedef struct {
    uint8_t status;     // 状态码
    uint8_t error;      //  错误原因
    uint8_t data[];     //  占位。data指向「查询」指令的「回应」会返回策略码。
} __attribute__((packed))StrgyMsgReport;

typedef struct {
    uint16_t version;        // 协议版本，目前不用管，设置为1
    uint16_t msg_type;       // 消息类型，分为新建（0）和关闭（1）；
    uint16_t action_type;    // 指令（0）或者回应（1）。
    uint16_t payload_len;    // 负载长度
} __attribute__((packed)) SessMsgHeader;

typedef enum {
    SESS_MSG_CREATE = 0,       // 新建
    SESS_MSG_CLOSE             // 关闭
} SessMsgType;


typedef enum {
    SESS_IPV4_PROTO = 4,       // IPv4
    SESS_IPV6_PROTO = 6        // IPv6
} SessIpProtoVersion;


typedef enum {
    SESS_UDP_PROTO = 0,       // UDP
    SESS_TCP_PROTO = 1        // TCP
} SessTranProto;

// 检查会话消息类型是否合法
#define IS_VALID_SESS_MSG_TYPE(sess_msg_type) \
    ((sess_msg_type) == SESS_MSG_CREATE || \
     (sess_msg_type) == SESS_MSG_CLOSE)


// 检查IP版本是否合法
#define IS_VALID_SESS_IP_VERSION(ip_version) \
    ((ip_version) == SESS_IPV4_PROTO || \
     (ip_version) == SESS_IPV6_PROTO)

// 会话消息载荷长度
#define SESS_MSG_PAYLOAD_LEN(sess_msg_type, action_type, ip_version) \
({ \
    int _len = -1;  /* 初始值：非法长度 */ \
    if ((sess_msg_type) == SESS_MSG_CLOSE) { \
        if ((action_type) == ACTION_TYPE_COMMAND) { \
            _len = 0;  /* 关闭+指令 → 无内容 */ \
        } else if ((action_type) == ACTION_TYPE_RESPONSE) { \
            _len = 2;  /* 关闭+回应 → 2字节（状态码+原因） */ \
        } \
    } else if ((sess_msg_type) == SESS_MSG_CREATE) { \
        if ((action_type) == ACTION_TYPE_RESPONSE) { \
            _len = 2;  /* 新建+回应 → 2字节（状态码+原因） */ \
        } else if ((action_type) == ACTION_TYPE_COMMAND) { \
            if ((ip_version) == SESS_IPV4_PROTO) { \
                _len = 10; /* IPv4 → 10字节会话信息 */ \
            } else if ((ip_version) == SESS_IPV6_PROTO) { \
                _len = 22; /* IPv6 → 22字节会话信息 */ \
            } \
        } \
    } \
    _len;  /* 返回计算结果 */ \
})


typedef struct {
    uint8_t ip_proto_ver;        // IP协议版本，分为IPv4（4）和IPv6（6）
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