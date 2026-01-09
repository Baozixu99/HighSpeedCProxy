#ifndef POLL_H
#define POLL_H

#include "channel.h"

struct BackendEngine_;


typedef struct NetPoller_{
    int epfd;
}NetPoller;

void poller_run(struct BackendEngine_ *eng, NetPoller *reactor);
void channel_set(NetChannel *ch, int fd, CALLBACK callback, void *arg);
void channel_set_sess(NetChannel *ch, struct BackendSession *sess);
int event_add(int ep_fd, int events, NetChannel *ch);
int event_del(int ep_fd, NetChannel *ch);
int poller_destory(NetPoller *reactor);
int poller_init(NetPoller *reactor);

int recv_cb(int fd, int events, void *arg);
int send_cb(int fd, int events, void *arg);


#endif

