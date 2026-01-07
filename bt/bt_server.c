#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <errno.h>

int main(int argc, char **argv) {
    // 1. 创建L2CAP Socket (SOCK_SEQPACKET确保可靠传输)
    int server_sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (server_sock < 0) {
        perror("Failed to create server socket");
        exit(1);
    }

    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options");
        close(server_sock);
        exit(1);
    }

    // 2. 设置并绑定服务器地址
    struct sockaddr_l2 loc_addr = { 0 };
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_psm = htobs(0x1001); // 设置PSM，例如0x1001
    bacpy(&loc_addr.l2_bdaddr, BDADDR_ANY); // 绑定到本地任意蓝牙适配器

    int bind_result = bind(server_sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    if (bind_result < 0) {
        perror("Failed to bind server socket");
        close(server_sock);
        exit(1);
    }

    // 3. 开始监听连接
    int listen_result = listen(server_sock, 1); // 允许1个连接排队
    if (listen_result < 0) {
        perror("Failed to listen");
        close(server_sock);
        exit(1);
    }
    printf("L2CAP server listening on PSM: 0x%04X\n", 0x1001);

    // 4. 接受客户端连接
    struct sockaddr_l2 client_addr;
    socklen_t opt_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &opt_len);
    if (client_sock < 0) {
        perror("Failed to accept connection");
        close(server_sock);
        exit(1);
    }

    char buf[1024] = { 0 };
    char addr_str[18] = { 0 };
    ba2str(&client_addr.l2_bdaddr, addr_str);
    printf("Accepted connection from %s\n", addr_str);

    // 5. 接收并回显数据
    ssize_t bytes_read;
    while ((bytes_read = read(client_sock, buf, sizeof(buf) - 1)) > 0) {
        printf("Received[%zd bytes]: %.*s\n", bytes_read, (int)bytes_read, buf);
        
        // 将收到的数据原样发回给客户端（回显），并检查写入结果
        ssize_t bytes_written = write(client_sock, buf, bytes_read);
        if (bytes_written < 0) {
            perror("Failed to write to client");
            break;
        } else if (bytes_written != bytes_read) {
            fprintf(stderr, "Partial write: expected %zd bytes, wrote %zd bytes\n", 
                    bytes_read, bytes_written);
            break;
        }
        
        memset(buf, 0, sizeof(buf)); // 清空缓冲区
    }
    
    if (bytes_read < 0) {
        perror("Read error");
    } else if (bytes_read == 0) {
        printf("Client disconnected\n");
    }

    // 6. 关闭连接
    close(client_sock);
    close(server_sock);
    printf("Server stopped.\n");
    return 0;
}