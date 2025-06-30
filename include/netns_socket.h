#ifndef NETNS_SOCKET_H
#define NETNS_SOCKET_H

#include <stdint.h>

int create_socket_netns(int ns_fd);
void set_nonblocking(int sockfd);


#endif /* NETNS_SOCKET_H */