#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main(int argc, char **argv)
{
    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int sk, status;
    char *message = "Hello, L2CAP!";
    char dest[18] = "01:23:45:67:89:AB"; // 默认目标地址
    char buffer[1024] = {0}; // 添加接收缓冲区

    // 检查参数，允许用户命令行输入目标地址
    if(argc > 1) {
        strncpy(dest, argv[1], 17);
        dest[17] = '\0';
    } else {
        printf("使用方法: %s <目标蓝牙地址>\n", argv[0]);
        printf("示例: %s 11:22:33:44:55:66\n", argv[0]);
        exit(1);
    }

    // 1. 创建L2CAP Socket
    sk = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sk < 0) {
        perror("创建Socket失败");
        exit(1);
    }

    // 2. 绑定Socket到本地蓝牙适配器
    loc_addr.l2_family = AF_BLUETOOTH;
    bacpy(&loc_addr.l2_bdaddr, BDADDR_ANY); // 关键步骤：绑定到任意本地适配器

    status = bind(sk, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    if (status < 0) {
        perror("绑定Socket失败");
        close(sk);
        exit(1);
    }
    
    // 3. 设置要连接的服务端地址
    rem_addr.l2_family = AF_BLUETOOTH;
    rem_addr.l2_psm = htobs(0x1001); // PSM必须与服务端一致
    str2ba(dest, &rem_addr.l2_bdaddr); // 将字符串地址转换为二进制格式

    // 4. 连接到服务端
    printf("正在连接到 %s...\n", dest);
    status = connect(sk, (struct sockaddr *)&rem_addr, sizeof(rem_addr));
    if (status == 0) {
        printf("连接成功！\n");

        // 5. 发送数据
        ssize_t bytes_written = write(sk, message, strlen(message));
        if (bytes_written > 0) {
            printf("消息发送成功，发送了 %zd 字节。\n", bytes_written);
            
            // 6. 接收服务器回显的数据
            ssize_t bytes_read = read(sk, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0'; // 确保字符串结束符
                printf("收到服务器回显: %.*s (%zd 字节)\n", (int)bytes_read, buffer, bytes_read);
            } else if (bytes_read == 0) {
                printf("服务器连接已关闭\n");
            } else {
                perror("接收数据失败");
            }
        } else {
            perror("发送失败");
        }
    } else {
        perror("连接失败");
        close(sk);
        exit(1);
    }

    // 7. 关闭连接
    close(sk);
    return 0;
}