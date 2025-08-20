#ifndef NETNS_SOCKET_H
#define NETNS_SOCKET_H

#include <stdint.h>
#include "message.h"
#include "common_utils.h"

#define ERROR_SOCKET_FD         -1

int create_socket_netns(int ns_id, int domain, int type, int protocol);
void set_nonblocking(int sockfd);
int connect_socket_netns(int fd, struct SessMsgPara *sess_para);

#endif /* NETNS_SOCKET_H */