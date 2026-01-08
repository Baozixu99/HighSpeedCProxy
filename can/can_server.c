#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#ifndef PF_CAN
#define PF_CAN 29
#endif
#ifndef AF_CAN
#define AF_CAN PF_CAN
#endif

// Global flag for graceful shutdown
static volatile sig_atomic_t running = 1;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    running = 0;
    printf("\nShutting down CAN server...\n");
}

int main() {
    int can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    ssize_t nbytes;
    int enable_own_receipt = 1; // 启用接收自己发送的帧
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 1. 创建CAN套接字
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // 设置套接字为非阻塞模式，以便能响应信号
    int flags = fcntl(can_socket, F_GETFL, 0);
    fcntl(can_socket, F_SETFL, flags | O_NONBLOCK);
    
    // 2. 设置接收自己发送的帧（可选）
    if (setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
                   &enable_own_receipt, sizeof(enable_own_receipt)) < 0) {
        perror("setsockopt CAN_RAW_RECV_OWN_MSGS failed");
    }
    
    // 3. 设置过滤规则（可选）- 只接收特定ID的帧
    struct can_filter filter[1];
    filter[0].can_id = 0x123;      // 要过滤的CAN ID
    filter[0].can_mask = 0x7FF;    // 标准帧掩码
    // 对于扩展帧：filter[0].can_id = 0x123 | CAN_EFF_FLAG;
    // 掩码：filter[0].can_mask = 0x1FFFFFFF;
    
    if (setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER,
                   &filter, sizeof(filter)) < 0) {
        perror("setsockopt CAN_RAW_FILTER failed");
        close(can_socket);
        return 1;
    }
    
    // 4. 绑定到特定CAN接口
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("IOCTL SIOCGIFINDEX failed - ensure can0 interface exists");
        close(can_socket);
        return 1;
    }
    
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex; // Use the actual interface index
    
    if (bind(can_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(can_socket);
        return 1;
    }
    
    printf("Server started on can0, waiting for CAN frames (Press Ctrl+C to stop)...\n");
    
    // 5. 循环接收CAN帧
    while (running) {
        nbytes = read(can_socket, &frame, sizeof(struct can_frame));
        
        if (nbytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有数据可读，短暂休眠以响应信号
                usleep(10000); // 休眠10毫秒
                continue;
            } else if (errno == EINTR) {
                // Interrupted by signal, continue to check running flag
                continue;
            }
            perror("Read failed");
            continue;
        }
        
        if (nbytes < sizeof(struct can_frame)) {
            fprintf(stderr, "Incomplete CAN frame received: %zd bytes\n", nbytes);
            continue;
        }
        
        // 6. 解析并显示接收到的CAN帧
        printf("Received CAN frame:\n");
        printf("  CAN ID: 0x%X", frame.can_id);
        if (frame.can_id & CAN_EFF_FLAG) {
            printf(" (Extended)");
        }
        printf("\n");
        printf("  Data Length: %d\n", frame.can_dlc);
        printf("  Data: ");
        
        for (int i = 0; i < frame.can_dlc && i < 8; i++) {
            printf("%02X ", frame.data[i]);
        }
        printf("\n  ASCII: ");
        for (int i = 0; i < frame.can_dlc && i < 8; i++) {
            if (frame.data[i] >= 32 && frame.data[i] <= 126) {
                printf("%c", frame.data[i]);
            } else {
                printf(".");
            }
        }
        printf("\n---\n");
        
        // 简单响应机制（可选）
        if (frame.can_dlc > 0 && frame.data[0] == 'H') {  // 如果收到特定数据
            struct can_frame response;
            response.can_id = 0x124;  // 响应ID
            response.can_dlc = 5;     // Correct length for "World"
            memcpy(response.data, "World", 5);
            // Initialize remaining bytes to avoid undefined values
            if (response.can_dlc < 8) {
                memset(&response.data[response.can_dlc], 0, 8 - response.can_dlc);
            }
            
            ssize_t response_bytes = write(can_socket, &response, sizeof(response));
            if (response_bytes < 0) {
                perror("Failed to send response frame");
            } else if (response_bytes != sizeof(response)) {
                fprintf(stderr, "Partial response frame sent: %zd of %zu bytes\n", 
                        response_bytes, sizeof(response));
            } else {
                printf("Sent response frame successfully\n");
            }
        }
    }
    
    close(can_socket);
    printf("CAN server stopped successfully.\n");
    return 0;
}