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
#include <sys/epoll.h>

#include "netns_socket.h"

int create_socket_netns(int ns_fd)
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

    newfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

void add_socket_to_epoll(int epoll_fd, int sock_fd, uint32_t events) {
    struct epoll_event event;
    event.data.fd = sock_fd;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event) == -1) {
        printf("epoll_ctl: add failed!");
        exit(EXIT_FAILURE);
    }
}