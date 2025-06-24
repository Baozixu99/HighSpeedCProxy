
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "netns_socket.h"
#include "dev.h"
#include "session.h"
#include "session_pool.h"
#include "channel.h"
#include "message.h"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

extern void print_pool(struct BackendSessionPool* s_pool);
extern void high_speed_delete_all_sess(struct BackendSessionPool* s_pool);

struct epoll_event events[MAX_EVENTS];

void epoll_run(int epoll_fd)
{
    while (1)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) 
        {
            printf("epoll_wait:no ready fds!\n");
            continue;
        }
        for (int i = 0; i < nfds; i++) {
            int sockfd = events[i].data.fd;
            
            if (events[i].events & EPOLLERR) 
            {
                int error = 0;
                socklen_t len = sizeof(error);
                if( getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == 0)
                {
                    printf("Socket %d error: %s\n", sockfd, strerror(error));
                }
                
                //todo,remove sockfd and close session
                // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
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
                    printf("Connection failed: %s\n", strerror(error));
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
}


int main(int argc, char** argv)
{
    high_speed_pool = (struct BackendSessionPool*)malloc(sizeof(struct BackendSessionPool));
    high_speed_init_pool(high_speed_pool);

    struct BackendSession* s1 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s1->backend_sess_id = 1;
    high_speed_pool->ops->insert_sess(high_speed_pool, s1);
    print_pool(high_speed_pool);

    struct BackendSession* s2 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s2->backend_sess_id = 2;
    
    struct BackendSession* s3 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s3->backend_sess_id = 3;

    struct BackendSession* s4 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s4->backend_sess_id = 4;

    high_speed_pool->ops->insert_sess(high_speed_pool, s1);
    high_speed_pool->ops->insert_sess(high_speed_pool, s2);
    high_speed_pool->ops->insert_sess(high_speed_pool, s3);
    high_speed_pool->ops->insert_sess(high_speed_pool, s4);

    print_pool(high_speed_pool);
    // high_speed_delete_all_sess(high_speed_pool);
   
    struct BackendSession* s5 = high_speed_pool->ops->search_sess(high_speed_pool, (uint16_t)3);
    if (s5) printf("%d\n", s5->backend_sess_id);

    high_speed_pool->ops->delete_sess(high_speed_pool, s3);

    struct BackendSession* s6 = high_speed_pool->ops->search_sess(high_speed_pool, (uint16_t)4);
    if (s6) printf("%d\n", s6->backend_sess_id);

    print_pool(high_speed_pool);
    printf("hello!\n");
    return 0;
}