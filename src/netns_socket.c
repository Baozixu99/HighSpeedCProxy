#define _GNU_SOURCE 
#include <sched.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "netns_socket.h"
#include "session_pool.h"


int create_socket_fastpath(int ns_id, struct SessMsgPara *para, int *fd){
    int new_fd, ret;
/* 
 * to do.
 */
    return ret;
}

int __create_socket_netns(int ns_id, int domain, int type, int protocol)
{
    int orig_netns;
    int newfd;
    // save origin netns
    orig_netns = open("/proc/self/ns/net", O_RDONLY);
    if(orig_netns != 0)
    {
        error_print("Open self netns failed!\n");
        return ERROR_SOCKET_FD;
    }

    //create socket in dst netns
    if(setns(ns_id, CLONE_NEWNET) == -1)
    {
        error_print("Set dest netns failed!\n");
        return ERROR_SOCKET_FD;
    }

    newfd = socket(domain, type, protocol);

    //back to origin netns
    if(setns(orig_netns, CLONE_NEWNET) == -1)
    {
        error_print("Back to origin netns failed!\n");
        return ERROR_SOCKET_FD;
    }
    close(orig_netns);
    return newfd;
}


int create_socket_netns(int ns_id, struct SessMsgPara *para, int *fd){
    int domain, type, protocol, new_fd, ret;

    if(NULL == para || NULL == fd){

    }
    
    if(SESS_IPV4_PROTO == para->ip_version){
        domain = AF_INET;
    }else if (SESS_IPV6_PROTO == para->ip_version){
        domain = AF_INET6;
    }else{
/*
 * Unsupported IP version.
 */
        error_print("create_socket_netns failed: the IP version is not supported!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    utils_print("In %s, trans_proto = %d\n", __func__, para->trans_proto);

    if(SESS_TCP_PROTO == para->trans_proto){
        type     = SOCK_STREAM;
        protocol = IPPROTO_TCP;
    }else if (SESS_UDP_PROTO == para->trans_proto){
        type     = SOCK_DGRAM;
        protocol = IPPROTO_UDP;
    }else if (SESS_FASTPATH_PROTO == para->trans_proto){
/*
 * XDP/eBPF.
 */
        return create_socket_fastpath(ns_id, para, fd);
    }else{
/*
 * Unsupported transparent protocol.
 */
        error_print("create_socket_netns failed: the transport protocol is not supported!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    new_fd = __create_socket_netns(ns_id, domain, type, protocol);

    if(ERROR_SOCKET_FD == new_fd){
        error_print("create_socket_netns failed: the socket creation procedure is not completed successfully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    *fd = new_fd;

    return BACKEND_PROXY_PROCESS_OK;
}


void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}


int connect_socket_netns(int fd, struct SessMsgPara *sess_para){
    UniSockAddr uni_addr;
    size_t      addr_len;
    IPv4PortTuple *ipv4_port_tuple;
    IPv6PortTuple *ipv6_port_tuple;

    memset(&uni_addr, 0, sizeof(UniSockAddr));

    if(SESS_IPV4_PROTO == sess_para->ip_version){
        ipv4_port_tuple                  = &sess_para->ip_port_tuple.ipv4_port_tuple;
        uni_addr.family                  = uni_addr.addr.ipv4_addr.sin_family = AF_INET;
        uni_addr.addr.ipv4_addr.sin_port = htons(ipv4_port_tuple->port);
        uni_addr.sa_addr_len             = sizeof(struct sockaddr_in);
        COPY_IPV4_TO_IN(&uni_addr.addr.ipv4_addr.sin_addr, &ipv4_port_tuple->ipv4_addr);
    }else if(SESS_IPV6_PROTO == sess_para->ip_version){
        ipv6_port_tuple                    = &sess_para->ip_port_tuple.ipv6_port_tuple;
        uni_addr.family                    = uni_addr.addr.ipv6_addr.sin6_family = AF_INET6;
        uni_addr.addr.ipv6_addr.sin6_port  = htons(ipv6_port_tuple->port);
        uni_addr.sa_addr_len               = sizeof(struct sockaddr_in6);
        COPY_IPV6_TO_IN6(&uni_addr.addr.ipv6_addr.sin6_addr, &ipv6_port_tuple->ipv6_addr);
    }else{
        error_print("connect_socket_netns returns an error because the IP version is not valid!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(0 != connect(fd, &uni_addr.addr.sa_addr, uni_addr.sa_addr_len)){
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    return BACKEND_PROXY_PROCESS_OK;
}