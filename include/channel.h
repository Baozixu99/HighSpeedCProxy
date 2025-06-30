#ifndef CHANNEL_H
#define CHANNEL_H

typedef int (*CALLBACK)(int, int, void *);
struct channel{
    int sock_fd;
    int events;  //感兴趣的事件
    void* arg;   //poller(reactor)
    int status;   //1 MOD, 0 ADD
    CALLBACK callback;
};

#endif