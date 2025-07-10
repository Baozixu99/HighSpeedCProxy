/*
*
* TODO: modify to suit Qos,just for test now.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/tcp.h> 

#define BUFFER_SIZE 4096
#define PORT 80
#define TIMEOUT_SEC 5
#define BAIDU_DOMAIN "www.baidu.com"
#define HTTP_REQUEST "GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: keep-alive\r\n\r\n"

// 错误处理函数
void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// 设置套接字超时
void set_socket_timeout(int sockfd, int seconds) {
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        error("setsockopt(SO_RCVTIMEO) failed");
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        error("setsockopt(SO_SNDTIMEO) failed");
    }
}

int rtt_stats() {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    printf("正在解析域名 %s...\n", BAIDU_DOMAIN);
    
    // 解析域名获取IP地址
    server = gethostbyname(BAIDU_DOMAIN);
    if (server == NULL) {
        fprintf(stderr, "无法解析主机名: %s\n", BAIDU_DOMAIN);
        exit(EXIT_FAILURE);
    }

    // 创建TCP套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("创建套接字失败");
    }
    
    printf("域名解析成功. 百度IP: %s\n", inet_ntoa(*((struct in_addr*)server->h_addr_list[0])));
    
    // 设置套接字超时
    set_socket_timeout(sockfd, TIMEOUT_SEC);

    // 配置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    // 连接到服务器
    printf("正在连接百度(%s:%d)...\n", inet_ntoa(server_addr.sin_addr), PORT);
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        error("连接服务器失败");
    }
    printf("TCP连接建立成功！\n");

    int cnt = 0;
    FILE *output_file = fopen("baidu_response.html", "aw");
    if (!output_file) {
        perror("创建输出文件失败");
    }
    while (cnt ++ < 50)
    {
         // 发送HTTP请求
        printf("发送HTTP请求 %d...\n", cnt);
        if (write(sockfd, HTTP_REQUEST, strlen(HTTP_REQUEST)) < 0) {
            error("发送请求失败");
        }
        printf("HTTP %d 请求发送成功\n", cnt);

        // 接收数据
        printf("接收数据中...\n\n");
        
        int total_bytes = 0;
        int header_found = 0;
        
        while (1) {
            n = read(sockfd, buffer, BUFFER_SIZE - 1);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("\n接收超时\n");
                    break;
                }
                perror("读取数据错误");
                break;
            } else if (n == 0) {
                printf("\n服务器关闭连接\n");
                break;
            }

            total_bytes += n;
            buffer[n] = '\0'; // 确保字符串结束
            
            // 将数据写入文件
            if (output_file) {
                fwrite(buffer, 1, n, output_file);
            }
            
            // 如果还没有找到HTTP头结束，则打印头部信息
            if (!header_found) {
                char *header_end = strstr(buffer, "\r\n\r\n");
                if (header_end) {
                    // 计算头部的长度
                    int header_length = header_end - buffer + 4;
                    // 打印前500字节的HTTP头
                    if (header_length > 500) header_length = 500;
                    printf("---- HTTP %d 响应头 ----\n", cnt);
                    for (int i = 0; i < header_length; i++) {
                        putchar(buffer[i]);
                    }
                    printf("\n\n（省略响应正文...）\n");
                    header_found = 1;
                } else if (total_bytes < 500) {
                    // 如果头还没结束，而且没超过500字节，打印缓冲区内容
                    printf("%s", buffer);
                }
            }

            struct tcp_info tcpinfo;
            socklen_t tcp_info_length = sizeof(tcpinfo);
	        if (getsockopt(sockfd, IPPROTO_TCP, TCP_INFO, (void*)&tcpinfo, &tcp_info_length) < 0){
                printf("tcp_info set failed!");
            }
            printf("tcpi_snd_cwnd %u tcpi_snd_mss %u tcpi_rtt %.2f ± %.2f ms\n",tcpinfo.tcpi_snd_cwnd, 
                tcpinfo.tcpi_snd_mss, tcpinfo.tcpi_rtt / 1000.0, tcpinfo.tcpi_rttvar / 1000.0);
            // 打印进度
            printf("\r已接收: %d bytes\n", total_bytes);
            fflush(stdout);
        }
        
        printf("\n\n http %d 数据接收完成. 共 %d bytes\n", cnt, total_bytes);
        if (output_file) {
            printf("完整响应 %d 已保存到 baidu_response.html\n", cnt);
        }
    }
    
    // 关闭套接字
    close(sockfd);
    fclose(output_file);
    printf("连接已关闭\n");
    
    return EXIT_SUCCESS;
}
