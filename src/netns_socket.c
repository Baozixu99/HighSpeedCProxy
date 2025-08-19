#define _GNU_SOURCE 
#include <sched.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

#include "netns_socket.h"
#include "session_pool.h"

int create_socket_netns(int ns_fd, int domain, int type, int protocol)
{
    int orig_netns;
    int newfd;
    // save origin netns
    orig_netns = open("/proc/self/ns/net", O_RDONLY);
    if(orig_netns != 0)
    {
        printf("Open self netns failed!\n");
        exit(1);
    }

    //create socket in dst netns
    if(setns(ns_fd, CLONE_NEWNET) == -1)
    {
        printf("Set dest netns failed!\n");
        exit(1);
    }

    newfd = socket(domain, type, protocol);

    //back to origin netns
    if(setns(orig_netns, CLONE_NEWNET) == -1)
    {
        printf("Back to origin netns failed!\n");
        exit(1);
    }
    close(orig_netns);
    return newfd;
}

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}


int connect_socket_netns(int fd, struct SessMsgPara *sess_para){
    return BACKEND_PROXY_PROCESS_OK;
}