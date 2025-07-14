/*
*
* TODO: modify to suit Qos,just for test now.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 256
#define DEFAULT_INTERVAL 2

typedef struct {
    char* name;             // 网卡名称
    unsigned long rx_bytes;     // 接收字节数
    unsigned long rx_packets;   // 接收数据包数
    unsigned long tx_bytes;     // 发送字节数
    unsigned long tx_packets;   // 发送数据包数
    unsigned long errors;       // 错误数
    unsigned long tx_dropped;      // 丢弃数据包数
} NetworkStats;

NetworkStats *stats;

// 使用ethtool命令获取网卡统计信息的函数
void get_ethtool_stats(const char *interface) {
    // 构造ethtool命令
    char command[128];
    snprintf(command, sizeof(command), "ethtool -S %s 2>/dev/null", interface);
    
    // 执行命令并获取输出
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        perror("Failed to run ethtool");
    }

    // 读取命令输出
    char *result = malloc(BUFFER_SIZE);
    if (!result) {
        perror("Memory allocation failed");
        pclose(pipe);
    }
    *result = '\0'; // 确保空终止

    stats->name = (char*)malloc(strlen(interface) + 1);
    strncpy(stats->name, interface, strlen(interface)+1);

    while (fgets(result, BUFFER_SIZE, pipe) != NULL) {

        if (strstr(result, ":") == NULL) {
            continue;
        }
        char *key = result;
        char *value = strchr(result, ':');
        if (value) {
            *value = '\0';
            value++;
            // 跳过值的空格
            while (*value == ' ' || *value == '\t') value++;
            // 根据关键字处理不同的统计项
            if (strstr(key, "rx_bytes")) {
                stats->rx_bytes = strtoul(value, NULL, 10);
            } else if (strstr(key, "rx_packets")) {
                stats->rx_packets = strtoul(value, NULL, 10);
            } else if (strstr(key, "tx_bytes")) {
                stats->tx_bytes = strtoul(value, NULL, 10);
            } else if (strstr(key, "tx_packets")) {
                stats->tx_packets = strtoul(value, NULL, 10);
            } else if (strstr(key, "errors")) {
                stats->errors = strtoul(value, NULL, 10);
            } else if (strstr(key, "tx_dropped")) {
                stats->tx_dropped = strtoul(value, NULL, 10);
            }
        }
    }   
    pclose(pipe);
}


int ethtool_stats(int argc, char *argv[]) {
    int interval = DEFAULT_INTERVAL;
    
    // 解析命令行参数
    if (argc == 2) {
        interval = atoi(argv[1]);
        if (interval <= 0) {
            fprintf(stderr, "Invalid interval. Using default %d seconds.\n", DEFAULT_INTERVAL);
            interval = DEFAULT_INTERVAL;
        }
    }
    
    printf("Monitoring network statistics using ethtool every %d seconds...\n", interval);
    printf("Press Ctrl+C to exit.\n\n");
    
    // 获取网卡接口列表
    char* interface = "ens33";
    stats = (NetworkStats*)malloc(sizeof(NetworkStats));
    if(!stats){
        printf("Stats allocate failed!\n");
    }
    memset(stats, 0, sizeof(NetworkStats));
    // 主监控循环
    while (1) {
        time_t now = time(NULL);
        printf("\n[%s] Network Statistics (ethtool):\n", ctime(&now));
        
        get_ethtool_stats(interface);
        if (stats != NULL) {
           printf("name: %s\n", stats->name);
           printf("rx_packets: %lu\n", stats->rx_packets);
           printf("tx_packets: %lu\n", stats->tx_packets);
           printf("rx_bytes: %lu\n", stats->rx_bytes);
           printf("tx_bytes: %lu\n", stats->tx_bytes);
           printf("tx_dropped: %lu\n", stats->tx_dropped);
        }
        sleep(interval);
    }
    
    return 0;
}