/*
 * Modbus TCP Simulation Server using libmodbus 3.1.12
 * This server simulates a Modbus device responding to various requests
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <modbus/modbus.h>

enum {
    TCP_PORT = 502,
};

int main(void)
{
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    int server_socket;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

    // Create a new Modbus TCP context
    ctx = modbus_new_tcp("192.168.1.101", TCP_PORT);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create the Modbus TCP context: %s\n", modbus_strerror(errno));
        return -1;
    }

    // Set debug mode (optional)
    modbus_set_debug(ctx, TRUE);

    // Initialize the Modbus mapping
    mb_mapping = modbus_mapping_new(
        50,  // Number of discrete inputs (bits)
        50,  // Number of coils (bits)
        50,  // Number of input registers
        50   // Number of holding registers
    );

    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the Modbus mapping: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    // Set initial values for some registers
    for (int i = 0; i < 10; i++) {
        mb_mapping->tab_registers[i] = i * 3;  // Example: register values
    }

    // Set initial values for some coils
    for (int i = 0; i < 10; i++) {
        mb_mapping->tab_bits[i] = i % 2;  // Example: alternating bit values
    }

    // Start the server listening
    server_socket = modbus_tcp_listen(ctx, 1);
    if (server_socket == -1) {
        fprintf(stderr, "Failed to listen: %s\n", modbus_strerror(errno));
        modbus_mapping_free(mb_mapping);
        modbus_free(ctx);
        return -1;
    }

    printf("Modbus TCP server listening on port %d\n", TCP_PORT);
    printf("Waiting for connections...\n");

    // Accept client connections and handle requests
    for (;;) {
        if (modbus_tcp_accept(ctx, &server_socket) == -1) {
            fprintf(stderr, "Error accepting connection: %s\n", modbus_strerror(errno));
            break;
        }

        int rc;
        while ((rc = modbus_receive(ctx, query)) != -1) {
            if (rc == 0) {
                // A query was handled successfully
                continue;
            }

            // Process the received query and send response
            rc = modbus_reply(ctx, query, rc, mb_mapping);
            if (rc == -1) {
                fprintf(stderr, "Error replying: %s\n", modbus_strerror(errno));
                break;
            }
        }
        
        printf("Client disconnected\n");
    }

    printf("Closing server socket\n");
    if (server_socket != -1) {
        close(server_socket);
    }

    // Cleanup
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}