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

#define ENABLE_UDP_TEST 1 

/**
 * @brief Connectivity test function specifically for UDP sockets.
 * 
 * This function sends a test message to a fixed IP/Port, waits for a reply 
 * with a 1-second timeout, and logs success or warning messages.
 * 
 * @param fd The already created UDP socket file descriptor.
 * @return 0 indicates the test flow completed (regardless of success/failure). 
 *         -1 indicates a critical error (e.g., invalid parameters).
 */
static int test_udp_connectivity(int fd) {
    struct sockaddr_in server_addr;
    struct timeval timeout;
    char send_buf[] = "test msg";
    char recv_buf[1024];
    socklen_t addr_len;
    int ret;
    const char *target_ip = "192.168.1.101";
    int target_port = 8888;

    utils_print("[UDP TEST] Starting connectivity test to %s:%d...\n", target_ip, target_port);

    // 1. Prepare the destination address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(target_port);
    
    if (inet_pton(AF_INET, target_ip, &server_addr.sin_addr) <= 0) {
        utils_print("[UDP TEST] Invalid IP address format: %s\n", target_ip);
        return -1;
    }

    // 2. Send test data "test msg"
    // UDP uses sendto; no prior connect() is required.
    ret = sendto(fd, send_buf, strlen(send_buf), 0, 
                 (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    if (ret < 0) {
        utils_print("[UDP TEST] Send failed! (errno: %d, %s)\n", errno, strerror(errno));
        return -1;
    }
    utils_print("[UDP TEST] Sent '%s' (%d bytes) to %s:%d\n", send_buf, ret, target_ip, target_port);

    // 3. Set receive timeout to 1 second
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        utils_print("[UDP TEST] Failed to set SO_RCVTIMEO! (errno: %d, %s)\n", errno, strerror(errno));
        return -1;
    }

    // 4. Block and wait for a reply
    memset(recv_buf, 0, sizeof(recv_buf));
    addr_len = sizeof(server_addr); // Reset length before call
    
    // Note: UDP is connectionless. recvfrom waits for a packet from any source, 
    // but we expect a reply from the target we just sent to.
    ret = recvfrom(fd, recv_buf, sizeof(recv_buf) - 1, 0, 
                   (struct sockaddr *)&server_addr, &addr_len);

    if (ret < 0) {
        // Check if the failure was due to a timeout
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // === Core Requirement: Print warning if timeout occurs ===
            utils_print("[UDP TEST] WARNING: Receive timeout! No message received from %s:%d within 1 second.\n", 
                        target_ip, target_port);
        } else {
            // Other errors
            utils_print("[UDP TEST] Recv failed! (errno: %d, %s)\n", errno, strerror(errno));
        }
        // Note: Timeout is not necessarily a fatal error for UDP (packets may be lost).
        // Return 0 to indicate the test flow completed.
        return 0; 
    } else {
        // Successfully received data
        recv_buf[ret] = '\0'; // Ensure null-termination
        
        // Optional: Print the sender's address
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &server_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        
        utils_print("[UDP TEST] Success! Received %d bytes from %s:%d: '%s'\n", 
                    ret, ip_str, ntohs(server_addr.sin_port), recv_buf);
        return 0;
    }
}


int create_socket_fastpath(int ns_id, struct SessMsgPara *para, int *fd){
//    int new_fd;
    int ret;
/* 
 * to do.
 */

    ret = 0;

    return ret;
}

int __create_socket_netns(int ns_id, int domain, int type, int protocol)
{
    int orig_netns;
    int newfd;
    // save origin netns
    orig_netns = open("/proc/self/ns/net", O_RDONLY);
    if(orig_netns == -1)
    {
        error_print("Open self netns failed!\n");
        return ERROR_SOCKET_FD;
    }

    //create socket in dst netns
    if(setns(ns_id, CLONE_NEWNET) == -1)
    {
        error_print("Set dest netns failed!\n");
        close(orig_netns);
        return ERROR_SOCKET_FD;
    }

    newfd = socket(domain, type, protocol);
    if(newfd == -1) {
        // Need to switch back to original namespace even on socket() failure
        setns(orig_netns, CLONE_NEWNET);
        close(orig_netns);
        return ERROR_SOCKET_FD;
    }

#if ENABLE_UDP_TEST
    // Perform test only for UDP Sockets (SOCK_DGRAM)
    if (domain == AF_INET && type == SOCK_DGRAM) {
        utils_print("[UDP TEST] Label 1\n");
        test_udp_connectivity(newfd);
        
        // After the test, the socket remains usable (UDP is connectionless).
        // We return the valid FD to the caller for further use.
        utils_print("[UDP TEST] Test finished. Returning valid socket fd %d.\n", newfd);
    }
#endif    

    //back to origin netns
    if(setns(orig_netns, CLONE_NEWNET) == -1)
    {
        error_print("Back to origin netns failed!\n");
        close(orig_netns);
        close(newfd);
        return ERROR_SOCKET_FD;
    }
    close(orig_netns);

    // ================= UDP Test Logic =================
#if ENABLE_UDP_TEST
    // Perform test only for UDP Sockets (SOCK_DGRAM)
    if (domain == AF_INET && type == SOCK_DGRAM) {
        utils_print("[UDP TEST] Label 2\n");
        test_udp_connectivity(newfd);
        
        // After the test, the socket remains usable (UDP is connectionless).
        // We return the valid FD to the caller for further use.
        utils_print("[UDP TEST] Test finished. Returning valid socket fd %d.\n", newfd);
    }
#endif

    return newfd;
}


int create_socket_netns(int ns_id, struct SessMsgPara *para, int *fd){
    int domain, type, protocol, new_fd;

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
    utils_print("In %s, new_fd = %d\n", __func__, new_fd);
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
//    size_t      addr_len;
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
        error_print("connect_socket_netns failed: the IP version is not valid!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(0 != connect(fd, &uni_addr.addr.sa_addr, uni_addr.sa_addr_len)){
        error_print("connect_socket_netns failed: failed to connect target IP:Port!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET, &uni_addr.addr.ipv4_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    utils_print("In %s, fd = %d\n", __func__, fd);
    utils_print("In %s, ip = %s, port = %d\n", __func__, ip_str, ntohs(uni_addr.addr.ipv4_addr.sin_port));
    set_nonblocking(fd);

    return BACKEND_PROXY_PROCESS_OK;
}