#ifndef CHANNEL_H
#define CHANNEL_H

#include "session.h"

typedef int (*CALLBACK)(int, int, void *);
struct channel{
    int sock_fd;
    int events;  //感兴趣的事件
    void* arg;   //poller(reactor)
    int status;   //1 MOD, 0 ADD
    CALLBACK callback;
    struct BackendSession* sess;
};

typedef struct NetChannel_{
    int sock_fd;
    int events;  //感兴趣的事件
    void* arg;   //poller(reactor)
    int status;   //1 MOD, 0 ADD
    CALLBACK callback;
    struct BackendSession* sess;
}NetChannel;


#endif