#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "poller.h"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024
struct epoll_event events[MAX_EVENTS];


void epoll_run(int epoll_fd)
{
     int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) 
    {
        printf("epoll_wait:no ready fds!\n");
        return;
    }
    for (int i = 0; i < nfds; i++) 
    {
        int sockfd = events[i].data.fd;

        if (events[i].events & EPOLLERR) 
        {
            //todo,remove sockfd and close session
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
            close(sockfd);
        }

        if (events[i].events & EPOLLOUT) 
        {
            int error = 0;
            socklen_t len = sizeof(error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
            if (error == 0) {
                printf("Connected to server via socket %d\n", sockfd);
                    
                //todo, send data to outside
                const char *msg = "Hello Server";
                send(sockfd, msg, strlen(msg), 0);

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = sockfd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &ev);
            } else {
                //todo,remove sockfd and close session
                close(sockfd);
            }
        }

        if (events[i].events & EPOLLIN) 
        {
            char buf[BUFFER_SIZE];
            ssize_t count = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if (count > 0) 
            {
                buf[count] = '\0';
                    //todo, recv data to front
                printf("Received from %d: %s\n", sockfd, buf);
            } else if (count == 0) { 
                printf("Server closed connection\n");
                //todo,remove sockfd and close session
                close(sockfd);
            } else {
                 
                //todo,remove sockfd and close session
                close(sockfd);
            }
        }
    }
}