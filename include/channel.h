#ifndef CHANNEL_H
#define CHANNEL_H

struct BackendSession;

typedef int (*CALLBACK)(int, int, void *);

typedef struct NetChannel_ {
    int                     sock_fd;
    int                     events;  // Events of interest
    void                    *arg;   // Pointer to poller (reactor)
    int                     status;  // Status flag: 1 for MOD, 0 for ADD
    CALLBACK                callback;  // Callback function
    struct BackendSession   *sess;  // Pointer to associated BackendSession structure
} NetChannel;


#endif