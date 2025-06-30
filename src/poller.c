#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string.h>
#include <errno.h>

#include "poller.h"
#include "channel.h"

/*
* usage:
* 1.cretae poller
*   struct poller *reactor = (struct poller *) malloc(sizeof(struct poller));
*   poller_init(reactor);
*
* 2.create socket
* 
* 3.cerate channel
*   struct channel *ch = (struct channel*) malloc(sizeof(struct channel));
*   memset(ch, 0, sizeof(struct channel));
*   channel_set(ch, fd, send_cb, reactor);
*   event_add(reactor->epfd, EPOLLOUT | EPOLLET, ch);
*
* 4.event loop run
*   poller_run(reactor)
*
* 5.destory poller
*   poller_destory(reactor);
*/

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024


void poller_run(struct poller* reactor)
{
    struct epoll_event events[MAX_EVENTS + 1];
    int nready = epoll_wait(reactor->epfd, events, MAX_EVENTS, -1);
    if (nready < 0) 
    {
        printf("epoll_wait error, exit\n");
        return;
    }
    for (int i = 0; i < nready; i++) 
    {
        struct channel *ch = (struct channel *) events[i].data.ptr;
        if ((events[i].events & EPOLLIN) && (ch->events & EPOLLIN)) 
        {
                ch->callback(ch->sock_fd, events[i].events, ch);
        }
        if ((events[i].events & EPOLLOUT) && (ch->events & EPOLLOUT)) 
        {
                ch->callback(ch->sock_fd, events[i].events, ch);
        }
    }
}

void channel_set(struct channel *ch, int fd, CALLBACK *callback, void *arg)
{
    ch->sock_fd = fd;
    ch->callback = callback;
    ch->events = 0;
    ch->arg = arg;
}

int event_add(int ep_fd, int events, struct channel *ch)
{
    struct epoll_event ev = {0, {0}};
    ev.data.ptr = ch;
    ev.events = ch->events = events;
    int op;
    if (ch->status == 1) {
        op = EPOLL_CTL_MOD;
    }
    else {
        op = EPOLL_CTL_ADD;
        ch->status = 1;
    }

    if (epoll_ctl(ep_fd, op, ch->sock_fd, &ev) < 0) {
        printf("event add failed [fd=%d], events[%d],err:%s,err:%d\n",ch->sock_fd, events, strerror(errno), errno);
        return -1;
    }
    return 0;
}

int event_del(int ep_fd, struct channel *ch) 
{
    struct epoll_event ep_ev = {0, {0}};
    if (ch->status != 1) {
        return -1;
    }
    ep_ev.data.ptr = ch;
    ch->status = 0;
    epoll_ctl(ep_fd, EPOLL_CTL_DEL, ch->sock_fd, NULL);
    // free(ch);
    return 0;
}

int recv_cb(int fd, int events, void *arg) 
{
    struct channel *ch = (struct channel *) arg;
    struct poller* reactor = (struct poller *)(ch->arg);
    int len;

    char buf[5];
    memset(buf, 0, sizeof(buf));
    // 循环读数据
    while(1)
    {
        int len = recv(fd, buf, sizeof(buf), 0);
        if(len == 0)
        {
            printf("Disconnect...\n");
            event_del(reactor->epfd, ch);
            close(fd);
            break;
        }
        else if(len > 0)
        {
            write(STDOUT_FILENO, buf, len);
            // todo, process data
        }
        else
        {
            // len == -1
            if(errno == EAGAIN)
            {
                printf("read complete...\n");
                break;
            }
            else
            {
                perror("recv");
                exit(0);
            }
        }
    }
    channel_set(ch, fd, send_cb, reactor);
    event_add(reactor->epfd, EPOLLOUT | EPOLLET, ch);
    return len;
}

int send_cb(int fd, int events, void *arg) {
    struct channel *ch = (struct channel *) arg;
    struct poller* reactor = (struct poller *)(ch->arg);

    int len;
    // todo, send data
    // channel_set(ch, fd, recv_cb, reactor);
    // event_add(reactor->epfd, EPOLLIN | EPOLLET, ch);

    return len;
}

int poller_init(struct poller* reactor)
{
    if (reactor == NULL) return -1;
    memset(reactor, 0, sizeof(struct poller));

    reactor->epfd = epoll_create(1);
    if (reactor->epfd <= 0)
    {
        printf("create epfd in %s err %s\n", __func__, strerror(errno));
        return -2;
    }
    return 0;
}

int poller_destory(struct poller *reactor) 
{
    close(reactor->epfd);
    return 0;
}