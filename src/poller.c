#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string.h>
#include <errno.h>

#include "engine.h"
#include "channel.h"
#include "poller.h"
#include "session.h"
#include "session_pool.h"
#include "common_utils.h"


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

#if 0
void poller_run(NetPoller *reactor)
{
    struct epoll_event events[MAX_EVENTS + 1];
    int nready = epoll_wait(reactor->epfd, events, MAX_EVENTS, 0);
    if (nready < 0) 
    {
        printf("epoll_wait error, exit\n");
        return;
    }
    for (int i = 0; i < nready; i++) 
    {
        NetChannel *ch = (NetChannel *) events[i].data.ptr;
        if ((events[i].events & EPOLLERR) || 
            (events[i].events & EPOLLHUP))
        {
                printf ("epoll error\n");
                // todo, close fd,session
                // close (events[i].data.fd);
                continue;
        }
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
#endif

void poller_run(struct BackendEngine_ *eng, NetPoller *reactor)
{
    struct BackendSession           *sess;
    struct epoll_event              events[MAX_EVENTS + 1];
    struct BackendSessionPoolOps    *sess_pool_ops;
    NetChannel                      *ch;
    int                             nready, ret;

    nready = epoll_wait(reactor->epfd, events, MAX_EVENTS, 0);
    
    if (nready < 0) 
    {
        printf("epoll_wait error, exit\n");
        return;
    }
    
    sess_pool_ops = eng->sess_pool->ops;
    
    for (int i = 0; i < nready; i++) 
    {
        ch      = (NetChannel *) events[i].data.ptr;
        sess    = ch->sess;
        if ((events[i].events & EPOLLERR) || 
            (events[i].events & EPOLLHUP))
        {
                error_print("poller_run: epoll error occurs!");
                // todo, close fd,session
                // close (events[i].data.fd);

                continue;
        }
        if ((events[i].events & EPOLLIN) && (ch->events & EPOLLIN)) 
        {
            
/*
 * Call the data_process_nns function pointer in the session pool's operation set (sess_pool_ops), which attempts to read data via the socket maintained by this session,
 * and organize it into the backend-to-frontend message queue, which will be sent to the shared memory TX queue.
 * 
 * The return value corresponds to three scenarios:
 * Returns BACKEND_PROXY_PROCESS_OK: All data has been read successfully.
 * Returns BACKEND_PROXY_PROCESS_AGAIN: Not all data has been read, and no errors occurred.
 * Returns BACKEND_PROXY_PROCESS_ERROR: An error occurred during the reading process.
 */
            ret = sess_pool_ops->data_process_nns(sess);


            if(BACKEND_PROXY_PROCESS_AGAIN == ret){

            }

            if(BACKEND_PROXY_PROCESS_ERROR == ret){

            }
//            ch->callback(ch->sock_fd, events[i].events, ch);
        }
#if 0
        if ((events[i].events & EPOLLOUT) && (ch->events & EPOLLOUT)) 
        {
                ch->callback(ch->sock_fd, events[i].events, ch);
        }
#endif
    }
}

void channel_set(NetChannel *ch, int fd, CALLBACK callback, void *arg)
{
    ch->sock_fd = fd;
    ch->callback = callback;
    ch->events = 0;
    ch->arg = arg;
}

void channel_set_sess(NetChannel *ch, struct BackendSession* sess)
{
    ch->sess = sess;
}

int event_add(int ep_fd, int events, NetChannel *ch)
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
    return BACKEND_PROXY_PROCESS_OK;
}

int event_del(int ep_fd, NetChannel *ch) 
{
    struct epoll_event ep_ev = {0, {0}};
    if (ch->status != 1) {
        return -1;
    }
    ep_ev.data.ptr = ch;
    ch->status = 0;
    epoll_ctl(ep_fd, EPOLL_CTL_DEL, ch->sock_fd, NULL);
    // free(ch);
    return BACKEND_PROXY_PROCESS_OK;
}

int recv_cb(int fd, int events, void *arg) 
{
    NetChannel *ch = (NetChannel *) arg;
    NetPoller *reactor = (NetPoller *)(ch->arg);
    int len = -1;

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
            if(errno == EAGAIN ||
               errno == EWOULDBLOCK)
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
    NetChannel *ch = (NetChannel *) arg;
    NetPoller *reactor = (NetPoller *)(ch->arg);

    int len = -1;
    // todo, send data
    // channel_set(ch, fd, recv_cb, reactor);
    // event_add(reactor->epfd, EPOLLIN | EPOLLET, ch);

    return len;
}

int poller_init(NetPoller *reactor)
{
    if (reactor == NULL) return -1;
    memset(reactor, 0, sizeof(NetPoller));

    reactor->epfd = epoll_create(MAX_EVENTS);
    if (reactor->epfd <= 0)
    {
        printf("create epfd in %s err %s\n", __func__, strerror(errno));
        return BACKEND_PROXY_PROCESS_ERROR;
    }
    return BACKEND_PROXY_PROCESS_OK;
}

int poller_destory(NetPoller *reactor) 
{
    close(reactor->epfd);
    return BACKEND_PROXY_PROCESS_OK;
}