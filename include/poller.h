#ifndef POLL_H
#define POLL_H

#include "channel.h"

struct poller {
    int epfd;
};

void poller_run(struct poller* reactor);
void channel_set(struct channel *ch, int fd, CALLBACK *callback, void *arg);
int event_add(int ep_fd, int events, struct channel *ch);
int event_del(int ep_fd, struct channel *ch);
int poller_destory(struct poller *reactor);
int poller_init(struct poller* reactor);

int recv_cb(int fd, int events, void *arg);
int send_cb(int fd, int events, void *arg);

#endif

