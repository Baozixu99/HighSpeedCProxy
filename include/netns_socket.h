#ifndef NETNS_SOCKET_H
#define NETNS_SOCKET_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "message.h"
#include "common_utils.h"

#define ERROR_SOCKET_FD         -1

/*
 * Optimized general socket address structure: contains type identifier + specific address
 */
typedef struct {
    sa_family_t family;  // Address family: AF_INET (for IPv4) or AF_INET6 (for IPv6)
    socklen_t   sa_addr_len;     // Length of the address structure (e.g., sizeof(sockaddr_in))
    union {
        struct sockaddr_in  ipv4_addr;  // IPv4 address (used when family=AF_INET)
        struct sockaddr_in6 ipv6_addr;  // IPv6 address (used when family=AF_INET6)
        struct sockaddr     sa_addr;    // General address structure (for API conversion)
    } addr;
} UniSockAddr;

int __create_socket_netns(int ns_id, int domain, int type, int protocol);
int create_socket_netns(int ns_id, struct SessMsgPara *sess_para, int *fd);
void set_nonblocking(int sockfd);
int connect_socket_netns(int fd, struct SessMsgPara *sess_para);


#endif /* NETNS_SOCKET_H */