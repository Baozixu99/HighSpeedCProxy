#ifndef POLL_H
#define POLL_H

#include "channel.h"

struct poller {
    int epfd;
};

void poller_run(struct poller* reactor);
void channel_set(NetChannel* ch, int fd, CALLBACK callback, void *arg);
void channel_set_sess(NetChannel* ch, struct BackendSession* sess);
int event_add(int ep_fd, int events, NetChannel *ch);
int event_del(int ep_fd, NetChannel *ch);
int poller_destory(struct poller *reactor);
int poller_init(struct poller* reactor);

int recv_cb(int fd, int events, void *arg);
int send_cb(int fd, int events, void *arg);

#endif

