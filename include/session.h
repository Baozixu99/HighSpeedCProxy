#ifndef SESSION_H
#define SESSION_H

#include <stdint.h> 
#include <sys/queue.h>
#include "uthash.h"
#include "channel.h"

#define BACKEND_PROXY_PROCESS_OK               0
#define BACKEND_PROXY_PROCESS_ERROR            1

struct ControlMsg{
    uint16_t dev_id;
};

struct SessMsgSeg {
    uint16_t len;
    uint16_t type;
    uint8_t *data;
    TAILQ_ENTRY(SessMsgSeg) entry;
};

TAILQ_HEAD(SessMsgQueue, SessMsgSeg);

struct BackendProtocolProcess; 

struct BackendSession {
    
    int sess_type;
    int sock_fd;
    
    uint16_t frontend_sess_id;
    uint16_t backend_sess_id;  // hash key
    
    // 状态机状态
    int state_f2b;          // 前端到后端状态
    int state_b2a;          // 后端到前端状态
    
    // 消息队列
    struct SessMsgQueue queue_f2b;  // 前端到后端消息队列
    struct SessMsgQueue queue_b2a;  // 后端到前端消息队列
    
    // 队列链接节点
    TAILQ_ENTRY(BackendSession) entries_f2b;   // 前端到后端活动队列节点
    TAILQ_ENTRY(BackendSession) entries_active; // 全局活动会话队列节点
    
    // 协议处理
    struct BackendProtocolProcess *protocol_process; // 协议处理模块指针
    
    // 私有数据指针（用于存储会话特定数据）
    void *pri_data;

    struct channel* channel;
    
    UT_hash_handle hh;
};

struct BackendProtocolProcess {

    int (*connect)(struct BackendSession* sess);
    
    int (*accept)(struct BackendSession* sess);
    
    int (*read)(struct BackendSession* sess, uint8_t* data, uint32_t size);
    
    int (*write)(struct BackendSession* sess, const uint8_t* data, uint32_t size);
    
    int (*close)(struct BackendSession* sess);

};

TAILQ_HEAD(BackendSessionQueue, BackendSession);

struct BackendSessionID {
    uint16_t id;
    TAILQ_ENTRY(BackendSessionID) entry;
};
TAILQ_HEAD(BackendSessionIDQueue, BackendSessionID);

struct BackendSession* create_session(int dev_id, struct ControlMsg* cmsg);
int session_send(struct BackendSession* sess, const uint8_t* data, uint32_t size);
int session_recv(struct BackendSession* sess, uint8_t* data, uint32_t size);
void delete_session(struct BackendSession* sess);

#endif