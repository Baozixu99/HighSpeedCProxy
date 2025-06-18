#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/epoll.h>

struct channel
{
    int sock_fd;
    struct epoll_event events;  //感兴趣的事件
    struct epoll_event revents; //实际发生的事件
};

#endif