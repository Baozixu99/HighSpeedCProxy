#ifndef SENARIO_TEST_H_
#define SENARIO_TEST_H_

#include "engine.h"
#include "backend_proto.h"
#include "message.h"
#include "shared_mem_io.h"

typedef enum {
    RESULT_MSG_VALID_SHARED,    // Valid message address in shared memory mode
    RESULT_MSG_VALID_CALLER,    // Valid message address in caller-allocated mode
    RESULT_MSG_INVALID_FULL,    // Invalid address due to full shared memory queue (addr_ptr set to UINT64_MAX)
    RESULT_MSG_INVALID_FAILED   // Invalid address due to message construction failure
} ResultMsgStatus;

/**
 * @brief Structure defining supported return value items of result_msg (strictly tied to "Backend Protocol Stack Unit Test.doc")
 * @details Each field corresponds to core requirements/scenarios in the document, 
 *          used to standardize and describe valid/invalid return values of result_msg
 */
typedef struct ResultMsgSupportedItem {
    ResultMsgStatus status;          // Status identifier for result_msg's return value (maps to document test scenarios)
    uint64_t        addr_value;      // Specific address value (or symbolic value) of result_msg (follows document's explicit definitions)
    const char*     status_desc;     // Detailed description linking to test cases in "Backend Protocol Stack Unit Test.doc"
    MemoryAllocMode applicable_mode; // Applicable memory allocation mode (aligns with document's "dual-mode" design)
} ResultMsgSupportedItem;


/**
 * @brief Macro to convert a dotted-decimal IPv4 string to an IPv4Address structure
 * @details Parses a string in "xxx.xxx.xxx.xxx" format, validates each octet range (0-255),
 *          and populates the IPv4Address structure with the binary representation.
 *          Provides error messages to stderr for invalid formats or out-of-range values.
 * 
 * @param ip_str Input string in dotted-decimal IPv4 format (e.g., "192.168.1.1")
 * @param addr_struct Output IPv4Address structure to be populated with parsed values
 */
#define IPV4_STR_TO_ADDR(ip_str, addr_struct) do { \
    unsigned int a, b, c, d;  /**< Temporary storage for parsed octet values */ \
    \
    /* Attempt to parse 4 octets from the input string */ \
    if (sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) { \
        \
        /* Validate all octets are within the 0-255 range */ \
        if (a <= 255 && b <= 255 && c <= 255 && d <= 255) { \
            /* Populate the structure with validated octets */ \
            (addr_struct).data[0] = (uint8_t)a; \
            (addr_struct).data[1] = (uint8_t)b; \
            (addr_struct).data[2] = (uint8_t)c; \
            (addr_struct).data[3] = (uint8_t)d; \
        } else { \
            /* Handle octet values outside valid range */ \
            error_print("IPV4_STR_TO_ADDR failed: Invalid IPv4 address!"); \
        } \
    } else { \
        /* Handle invalid string format (not matching xxx.xxx.xxx.xxx) */ \
        error_print("IPV4_STR_TO_ADDR failed: Invalid IPv4 format!"); \
    } \
} while(0)


/**
 * @brief Macro to convert an IPv4:port string to an IPv4PortTuple structure
 * @details Parses a string in "xxx.xxx.xxx.xxx:port" format, splits it into IP address
 *          and port components, validates both parts, and populates the IPv4PortTuple.
 *          Port numbers must be in the range 0-65535.
 * 
 * @param ip_port_str Input string in "xxx.xxx.xxx.xxx:port" format (e.g., "192.168.1.100:8080")
 * @param tuple_struct Output IPv4PortTuple structure to be populated with parsed values
 */
#define IPV4_PORT_STR_TO_TUPLE(ip_port_str, tuple_struct) do { \
    char ip_str[16];  /* Buffer to store the IP address part (max IPv4 string length is 15) */ \
    unsigned int port; \
    \
    /* Parse the IP address and port from the input string */ \
    if (sscanf(ip_port_str, "%15[^:]:%u", ip_str, &port) == 2) { \
        \
        /* Convert and validate the IP address part */ \
        IPV4_STR_TO_ADDR(ip_str, (tuple_struct).ipv4_addr); \
        \
        /* Validate the port number (0-65535 range) */ \
        if (port > 65535) { \
            error_print("IPV4_PORT_STR_TO_TUPLE failed: invalid port number: must be 0-65535)!"); \
        } else { \
            (tuple_struct).port = (uint16_t)port; \
        } \
    } else { \
        error_print("IPV4_PORT_STR_TO_TUPLE failed: invalid IP:port format!"); \
    } \
} while(0)


int scenario_msg_inject(BackendEngine *engine,
                        GeneralProxyMsgHeader *msg_header,
                        const uint8_t *msg_payload,
                        size_t msg_payload_len,
                        MemoryAllocMode alloc_mode,
                        uint8_t **result_msg,
                        char *result_desc,
                        size_t desc_len);


int test_proxy_scenario_multi_type_msg_build(BackendEngine *engine);



int device_msg_inject(BackendEngine *engine);

int strategy_msg_inject(BackendEngine *engine);

int session_msg_inject(BackendEngine *engine);

int data_msg_inject(BackendEngine *engine);


int test_proxy_scenario_msg_read_from_rx_queue(BackendEngine *engine);

#endif