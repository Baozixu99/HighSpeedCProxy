#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <errno.h>

#ifndef PF_CAN
#define PF_CAN 29
#endif
#ifndef AF_CAN
#define AF_CAN PF_CAN
#endif

int main() {
    int can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    ssize_t nbytes;
    
    // 1. 创建CAN套接字
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // 2. 指定CAN接口并获取接口索引
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("IOCTL SIOCGIFINDEX failed - ensure can0 interface exists");
        close(can_socket);
        return 1;
    }
    printf("can0 interface index: %d\n", ifr.ifr_ifindex);
    
    // 3. 绑定套接字到CAN接口
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(can_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(can_socket);
        return 1;
    }
    
    // 4. 准备CAN数据帧
    frame.can_id = 0x123;  // CAN标识符
    frame.can_dlc = 5;     // 数据长度(1-8字节)
    frame.data[0] = 'H';
    frame.data[1] = 'e';
    frame.data[2] = 'l';
    frame.data[3] = 'l';
    frame.data[4] = 'o';
    // Initialize remaining bytes to avoid undefined behavior
    memset(&frame.data[5], 0, 3); // Initialize unused bytes to 0
    
    // 5. 发送CAN帧
    nbytes = write(can_socket, &frame, sizeof(struct can_frame));
    if (nbytes < 0) {
        perror("Write failed");
        close(can_socket);
        return 1;
    } else if (nbytes != sizeof(struct can_frame)) {
        fprintf(stderr, "Write incomplete: expected %zu bytes, got %zd bytes\n", 
                sizeof(struct can_frame), nbytes);
        close(can_socket);
        return 1;
    }
    
    printf("Sent CAN frame: ID=0x%X, DLC=%d, Data=Hello\n", 
           frame.can_id, frame.can_dlc);

    // 6. 接收CAN帧
    nbytes = read(can_socket, &frame, sizeof(struct can_frame));
    if (nbytes < 0) {
        if (errno == EINTR) {
            // Interrupted by signal, continue to check running flag
            return 1;
        }
        perror("Read failed");
        return 1;
    }
        
    if (nbytes < sizeof(struct can_frame)) {
        fprintf(stderr, "Incomplete CAN frame received: %zd bytes\n", nbytes);
        return 1;
    }
        
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
    close(can_socket);
    return 0;
}