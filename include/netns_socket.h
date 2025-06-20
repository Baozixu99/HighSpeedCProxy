#ifndef NETNS_SOCKET_H
#define NETNS_SOCKET_H

int create_socket_netns(int ns_fd);
void set_nonblocking(int sockfd);
void add_socket_to_epoll(int epoll_fd, int sock_fd, uint32_t events);

#endif /* NETNS_SOCKET_H */