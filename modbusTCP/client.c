#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <modbus/modbus.h>
enum {
    TCP_PORT = 502,
};
int main(void)
{
    modbus_t *ctx;
    uint16_t regs[10];  // 用于存储读取的寄存器值
    uint8_t bits[10];   // 用于存储读取的线圈值
    int rc;

    // 创建Modbus TCP客户端上下文，连接到本地服务器
    ctx = modbus_new_tcp("192.168.137.2", TCP_PORT);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create the Modbus TCP context: %s\n", modbus_strerror(errno));
        return -1;
    }

    // 设置调试模式（可选）
    modbus_set_debug(ctx, TRUE);

    // 连接到服务器
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    printf("Connected to Modbus TCP server\n");

    // 1. 读取保持寄存器 (功能码 0x03)
    printf("\n--- Reading Holding Registers ---\n");
    rc = modbus_read_registers(ctx, 0, 5, regs);
    if (rc == -1) {
        fprintf(stderr, "Failed to read registers: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully read %d registers:\n", rc);
        for (int i = 0; i < rc; i++) {
            printf("Register[%d] = %d\n", i, regs[i]);
        }
    }

    // 2. 读取线圈 (功能码 0x01)
    printf("\n--- Reading Coils ---\n");
    rc = modbus_read_bits(ctx, 0, 5, bits);
    if (rc == -1) {
        fprintf(stderr, "Failed to read coils: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully read %d coils:\n", rc);
        for (int i = 0; i < rc; i++) {
            printf("Coil[%d] = %d\n", i, bits[i]);
        }
    }

    // 3. 写入单个寄存器 (功能码 0x06)
    printf("\n--- Writing Single Register ---\n");
    rc = modbus_write_register(ctx, 5, 999);
    if (rc == -1) {
        fprintf(stderr, "Failed to write register: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully wrote register[5] = 999\n");
    }

    // 4. 再次读取寄存器验证写入结果
    printf("\n--- Reading Register After Write ---\n");
    rc = modbus_read_registers(ctx, 5, 1, regs);
    if (rc == -1) {
        fprintf(stderr, "Failed to read register: %s\n", modbus_strerror(errno));
    } else {
        printf("Register[5] = %d\n", regs[0]);
    }

    // 5. 写入多个寄存器 (功能码 0x10)
    printf("\n--- Writing Multiple Registers ---\n");
    uint16_t write_regs[3] = {100, 200, 300};
    rc = modbus_write_registers(ctx, 6, 3, write_regs);
    if (rc == -1) {
        fprintf(stderr, "Failed to write multiple registers: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully wrote 3 registers starting at address 6\n");
    }

    // 6. 读取刚写入的多个寄存器
    printf("\n--- Reading Registers After Multiple Write ---\n");
    rc = modbus_read_registers(ctx, 6, 3, regs);
    if (rc == -1) {
        fprintf(stderr, "Failed to read registers: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully read %d registers:\n", rc);
        for (int i = 0; i < rc; i++) {
            printf("Register[%d] = %d\n", i + 6, regs[i]);
        }
    }

    // 7. 写入单个线圈 (功能码 0x05)
    printf("\n--- Writing Single Coil ---\n");
    rc = modbus_write_bit(ctx, 5, 1);
    if (rc == -1) {
        fprintf(stderr, "Failed to write coil: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully wrote coil[5] = 1\n");
    }

    // 8. 读取线圈验证写入结果
    printf("\n--- Reading Coil After Write ---\n");
    rc = modbus_read_bits(ctx, 5, 1, bits);
    if (rc == -1) {
        fprintf(stderr, "Failed to read coil: %s\n", modbus_strerror(errno));
    } else {
        printf("Coil[5] = %d\n", bits[0]);
    }

    // 9. 写入多个线圈 (功能码 0x0F)
    printf("\n--- Writing Multiple Coils ---\n");
    uint8_t write_bits[3] = {1, 0, 1};
    rc = modbus_write_bits(ctx, 7, 3, write_bits);
    if (rc == -1) {
        fprintf(stderr, "Failed to write multiple coils: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully wrote 3 coils starting at address 7\n");
    }

    // 10. 读取输入寄存器 (功能码 0x04)
    printf("\n--- Reading Input Registers ---\n");
    rc = modbus_read_input_registers(ctx, 0, 5, regs);
    if (rc == -1) {
        fprintf(stderr, "Failed to read input registers: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully read %d input registers:\n", rc);
        for (int i = 0; i < rc; i++) {
            printf("Input Register[%d] = %d\n", i, regs[i]);
        }
    }

    // 11. 读取离散输入 (功能码 0x02)
    printf("\n--- Reading Discrete Inputs ---\n");
    rc = modbus_read_input_bits(ctx, 0, 5, bits);
    if (rc == -1) {
        fprintf(stderr, "Failed to read discrete inputs: %s\n", modbus_strerror(errno));
    } else {
        printf("Successfully read %d discrete inputs:\n", rc);
        for (int i = 0; i < rc; i++) {
            printf("Discrete Input[%d] = %d\n", i, bits[i]);
        }
    }

    // 断开连接
    modbus_close(ctx);
    modbus_free(ctx);

    printf("\nDisconnected from server\n");
    return 0;
}