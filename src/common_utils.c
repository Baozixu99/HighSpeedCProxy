#include "common_utils.h"
#include "message.h"
#include "session.h"
#include <inttypes.h>

#define PRINT_SEPARATOR() printf("--------------------------------------------------\n")

void error_print(char *error){
    printf("%s\n", error);
}


/**
 * @brief Print debug information with variable arguments
 * @param format  Format string (same syntax as printf), must not be NULL
 * @param ...     Variable arguments matching the format string
 * @return        On success: number of characters printed (if debug enabled) or 0 (if debug disabled)
 *                On failure: -1 (e.g., NULL format string or vprintf error)
 * @note          The function behavior depends on UTILS_ENABLE_DEBUG macro:
 *                - Enabled: forward to vprintf and return its result
 *                - Disabled: do nothing and return 0
 */
int debug_print(const char *format, ...) {
    // Return value variable, initialized to 0 (default success)
    int ret = 0;

#if UTILS_ENABLE_DEBUG
    // Validate input parameter: return error if format is NULL
    if (format == NULL) {
        ret = -1;  // Mark error status
        return ret;
    }

    va_list args;
    va_start(args, format);
    // Call vprintf and get its return value: success=char count, failure=-1
    ret = vprintf(format, args);
    va_end(args);
#else
    // Do nothing when debug is disabled, return 0 (indicates "no operation success")
    ret = 0;
#endif

    // Unified return statement to ensure non-void function always has return value
    return ret;
}


/**
 * @brief Get string representation of Proxy Message Type.
 */
const char* get_proxy_type_str(uint8_t type) {
    switch (type) {
        case PROXY_MSG_TYPE_DEV: return "DEV (Device)";
        case PROXY_MSG_TYPE_STRGY: return "STRGY (Strategy)";
        case PROXY_MSG_TYPE_SESS: return "SESS (Session)";
        case PROXY_MSG_TYPE_DATA: return "DATA (Data)";
        default: return "UNKNOWN";
    }
}


/**
 * @brief Get string representation of Device Message Type.
 */
const char* get_dev_msg_type_str(uint16_t type) {
    switch (type) {
        case DEV_MSG_DISABLE: return "DISABLE";
        case DEV_MSG_ENABLE: return "ENABLE";
        case DEV_MSG_QUERY: return "QUERY";
        default: return "UNKNOWN";
    }
}


/**
 * @brief Get string representation of Action Type.
 */
const char* get_action_type_str(uint16_t type) {
    switch (type) {
        case ACTION_TYPE_COMMAND: return "COMMAND";
        case ACTION_TYPE_RESPONSE: return "RESPONSE";
        default: return "UNKNOWN";
    }
}

// Helper for Strategy Message Type string
const char* get_strgy_msg_type_str(uint16_t type) {
    switch (type) {
        case STRGY_MSG_SET: return "SET";
        case STRGY_MSG_QUERY: return "QUERY";
        default: return "UNKNOWN";
    }
}


// Helper for Strategy Parameter string
const char* get_strgy_param_str(uint16_t param) {
    switch (param) {
        case STRGY_PARAM_ROUND_ROBIN: return "Round Robin";
        case STRGY_PARAM_MAX_BANDWIDTH: return "Max Bandwidth";
        case STRGY_PARAM_MIN_LATENCY: return "Min Latency";
        default: return "Unknown Param";
    }
}



// Helper for Session Message Type
const char* get_sess_msg_type_str(uint16_t type) {
    switch (type) {
        case SESS_MSG_CLOSE: return "CLOSE";
        case SESS_MSG_CREATE: return "CREATE";
        default: return "UNKNOWN";
    }
}

// Helper for Session IP Version
const char* get_sess_ip_version_str(uint16_t version) {
    switch (version) {
        case SESS_NON_IP_PROTO: return "Non-IP";
        case SESS_IPV4_PROTO: return "IPv4";
        case SESS_IPV6_PROTO: return "IPv6";
        default: return "Unknown";
    }
}

// Helper for Session Transport Protocol
const char* get_sess_trans_proto_str(uint16_t proto) {
    switch (proto) {
        case SESS_UDP_PROTO: return "UDP";
        case SESS_TCP_PROTO: return "TCP";
        case SESS_FASTPATH_PROTO: return "FastPath (XDP/eBPF)";
        default: return "Unknown Proto";
    }
}

// Helper for Session Operation Status
const char* get_sess_op_status_str(uint8_t status) {
    switch (status) {
        case SESS_OP_STATUS_SUCCESS: return "SUCCESS";
        case SESS_OP_STATUS_FAIL: return "FAIL";
        default: return "UNKNOWN";
    }
}

// Helper for Session Operation Code
const char* get_sess_op_code_str(uint8_t code) {
    switch (code) {
        case SESS_OP_CODE_SUCCESS: return "Success";
        case SESS_OP_CODE_NO_PERMISSION: return "No Permission";
        case SESS_OP_CODE_DEVICE_ERROR: return "Device Error";
        case SESS_OP_CODE_RESOURCE_INSUFFICIENT: return "Resource Insufficient";
        case SESS_OP_CODE_NETWORK_UNREACHABLE: return "Network Unreachable";
        default: return "Unknown Code";
    }
}


/**
 * @brief Print a block of hexadecimal data.
 * @param label Label to print before the hex data.
 * @param data Pointer to the data buffer.
 * @param len Length of the data to print.
 */
void print_hex_block(const char* label, const uint8_t* data, size_t len) {
    if (len == 0) return;
    printf("  %s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n             ");
    }
    printf("\n");
}


/**
 * @brief Print a 16-bit mask in binary format.
 * @param label Label to print.
 * @param mask The 16-bit mask value.
 */
void print_binary_mask(const char* label, uint16_t mask) {
    printf("  %s: 0x%04X (", label, mask);
    for (int i = 15; i >= 0; i--) {
        printf("%d", (mask >> i) & 1);
        if (i == 8) printf(" ");
    }
    printf(")\n");
}


/**
 * @brief Main entry point: Dispatches to specific handlers based on message type.
 * 
 * Implements Scheme A: Passes the original header pointer to all sub-functions,
 * allowing them full context access (both Proxy and Business headers).
 * 
 * @param header Pointer to the raw buffer starting at ProxyMsgHeader.
 */
void proxy_msg_content_dump(const uint8_t* header){
    if (!header) {
        printf("[ERROR] Null pointer provided.\n");
        return;
    }

    const ProxyMsgHeader* proxy_hdr = (const ProxyMsgHeader*)header;

    printf("\n>>> PROXY MESSAGE DUMP <<<\n");
    printf("Version: %u\n", proxy_hdr->version);
    printf("Type   : %s (%u)\n", get_proxy_type_str(proxy_hdr->proxy_msg_type), proxy_hdr->proxy_msg_type);
    printf("FE Ses : %u\n", proxy_hdr->frontend_sess_id);
    printf("BE Ses : %u\n", proxy_hdr->backend_sess_id);
    printf("Payload: %u bytes\n", proxy_hdr->payload_len);

    // Dispatch logic: Pass the original header pointer directly
    switch (proxy_hdr->proxy_msg_type) {
        case PROXY_MSG_TYPE_DEV:
            proxy_dev_msg_content_dump(header); // Pass original pointer
            break;
        case PROXY_MSG_TYPE_STRGY:
            proxy_strgy_msg_content_dump(header); // Pass original pointer
            break;
        case PROXY_MSG_TYPE_SESS:
            proxy_sess_msg_content_dump(header); // Pass original pointer
            break;
        case PROXY_MSG_TYPE_DATA:
            proxy_data_msg_content_dump(header); // Pass original pointer
            break;
        default:
            printf("  [ERROR] Unknown message type %u\n", proxy_hdr->proxy_msg_type);
            break;
    }
}


/**
 * @brief Dump content of a Device Message.
 * 
 * This function assumes 'header' points to the start of the ProxyMsgHeader.
 * It internally calculates offsets to access DevMsgHeader and Payload.
 * 
 * @param header Pointer to the raw buffer starting at ProxyMsgHeader.
 */
void proxy_dev_msg_content_dump(const uint8_t* header){
    // 1. Parse Proxy Header (to get total length and session info)
    const ProxyMsgHeader* proxy_hdr = (const ProxyMsgHeader*)header;
    
    if (proxy_hdr->payload_len < sizeof(DevMsgHeader)) {
        printf("  [ERROR] Proxy payload too short for DevMsgHeader\n");
        return;
    }

    // 2. [Key Step] Calculate offset internally to get DevMsgHeader
    // Since 'header' points to ProxyMsgHeader, we add its size to reach DevMsgHeader
    const DevMsgHeader* dev_hdr = (const DevMsgHeader*)(header + sizeof(ProxyMsgHeader));
    
    // 3. Calculate Payload start address
    const uint8_t* payload_start = header + sizeof(ProxyMsgHeader) + sizeof(DevMsgHeader);
    
    // Calculate actual available space in the buffer
    uint16_t available_space = proxy_hdr->payload_len - sizeof(DevMsgHeader);

    printf("  --- Device Message Details ---\n");
    // Access Proxy layer info for context logging
    printf("  [Link] Frontend Session: %u, Backend Session: %u\n", 
           proxy_hdr->frontend_sess_id, proxy_hdr->backend_sess_id);
           
    printf("  Dev Version: %u\n", dev_hdr->version);
    printf("  Msg Type: %s (%u)\n", get_dev_msg_type_str(dev_hdr->msg_type), dev_hdr->msg_type);
    printf("  Msg ID: %u\n", dev_hdr->msg_id);
    printf("  Action Type: %s (%u)\n", get_action_type_str(dev_hdr->action_type), dev_hdr->action_type);
    printf("  Dev Payload Len: %u\n", dev_hdr->payload_len);

    if (dev_hdr->payload_len > available_space) {
        printf("  [WARNING] Declared payload len (%u) > Available space (%u). Truncating read.\n", 
               dev_hdr->payload_len, available_space);
    }

    uint16_t m_type = dev_hdr->msg_type;
    uint16_t a_type = dev_hdr->action_type;
    // Determine safe read length
    uint16_t read_len = (dev_hdr->payload_len < available_space) ? dev_hdr->payload_len : available_space;

    // --- Logic Branches ---
    if (a_type == ACTION_TYPE_COMMAND) {
        if (m_type == DEV_MSG_DISABLE || m_type == DEV_MSG_ENABLE) {
            printf("  [CMD] Operation: %s Device(s)\n", m_type == DEV_MSG_DISABLE ? "Disable" : "Enable");
            if (read_len >= sizeof(DevMsgMask)) {
                const DevMsgMask* mask = (const DevMsgMask*)payload_start;
                print_binary_mask("Device Mask", mask->data);
            } else {
                printf("  [ERROR] Payload too short for Mask (Need 2 bytes)\n");
            }
        } 
        else if (m_type == DEV_MSG_QUERY) {
            printf("  [CMD] Operation: Query Devices\n");
            if (read_len > 0) {
                printf("  [WARNING] Query CMD should have 0 payload, but found %u bytes.\n", read_len);
                print_hex_block("Unexpected Data", payload_start, read_len);
            } else {
                printf("  [INFO] Payload is empty (Correct)\n");
            }
        }
    } 
    else if (a_type == ACTION_TYPE_RESPONSE) {
        if (read_len < 2) {
            printf("  [ERROR] Response payload too short (Need Result+Error)\n");
            return;
        }

        uint8_t result = payload_start[0];
        uint8_t err_code = payload_start[1];
        
        printf("  [RSP] Result: %s (0x%02X)\n", result == 1 ? "SUCCESS" : "FAILED", result);
        printf("  [RSP] Error Code: 0x%02X\n", err_code);

        if (m_type == DEV_MSG_DISABLE || m_type == DEV_MSG_ENABLE) {
            printf("  [RSP] Context: Enable/Disable Response\n");
            if (read_len > 2) print_hex_block("Extra Data", payload_start + 2, read_len - 2);
        } 
        else if (m_type == DEV_MSG_QUERY) {
            printf("  [RSP] Context: Query Response\n");
            if (read_len >= 4) {
                uint16_t active_mask = *(uint16_t*)(payload_start + 2);
                print_binary_mask("Active Devices", active_mask);
                if (read_len > 4) print_hex_block("Extra Data", payload_start + 4, read_len - 4);
            } else {
                printf("  [ERROR] Query Response payload too short (Need 4 bytes)\n");
            }
        }
    }
    printf("  ------------------------------\n");
}


/**
 * @brief Dump content of a Strategy Message (Placeholder).
 * @param header Pointer to the raw buffer starting at ProxyMsgHeader.
 */
void proxy_strgy_msg_content_dump(const uint8_t* header){
    // 1. Parse Proxy Header
    const ProxyMsgHeader* proxy_hdr = (const ProxyMsgHeader*)header;
    
    if (proxy_hdr->payload_len < sizeof(StrgyMsgHeader)) {
        printf("  [ERROR] Proxy payload too short for StrgyMsgHeader\n");
        return;
    }

    // 2. Calculate offset to get StrgyMsgHeader
    const StrgyMsgHeader* strgy_hdr = (const StrgyMsgHeader*)(header + sizeof(ProxyMsgHeader));
    
    // 3. Calculate Payload start address
    const uint8_t* payload_start = header + sizeof(ProxyMsgHeader) + sizeof(StrgyMsgHeader);
    
    // Calculate actual available space
    uint16_t available_space = proxy_hdr->payload_len - sizeof(StrgyMsgHeader);

    printf("  --- Strategy Message Details ---\n");
    printf("  [Link] Frontend Session: %u, Backend Session: %u\n", 
           proxy_hdr->frontend_sess_id, proxy_hdr->backend_sess_id);
           
    printf("  Strgy Version: %u\n", strgy_hdr->version);
    printf("  Msg Type: %s (%u)\n", get_strgy_msg_type_str(strgy_hdr->msg_type), strgy_hdr->msg_type);
    printf("  Msg ID: %u\n", strgy_hdr->msg_id);
    printf("  Action Type: %s (%u)\n", get_action_type_str(strgy_hdr->action_type), strgy_hdr->action_type);
    printf("  Strgy Payload Len: %u\n", strgy_hdr->payload_len);

    if (strgy_hdr->payload_len > available_space) {
        printf("  [WARNING] Declared payload len (%u) > Available space (%u). Truncating read.\n", 
               strgy_hdr->payload_len, available_space);
    }

    uint16_t m_type = strgy_hdr->msg_type;
    uint16_t a_type = strgy_hdr->action_type;
    uint16_t read_len = (strgy_hdr->payload_len < available_space) ? strgy_hdr->payload_len : available_space;

    // --- Logic Branches ---
    if (a_type == ACTION_TYPE_COMMAND) {
        if (m_type == STRGY_MSG_SET) {
            printf("  [CMD] Operation: Set Strategy\n");
            if (read_len >= sizeof(StrgyCMDEnableMessage)) {
                const StrgyCMDEnableMessage* cmd = (const StrgyCMDEnableMessage*)payload_start;
                printf("  [CMD] Strategy Parameter: %s (%u)\n", 
                       get_strgy_param_str(cmd->strgy_para), cmd->strgy_para);
                
                if (read_len > sizeof(StrgyCMDEnableMessage)) {
                    print_hex_block("Extra Data", payload_start + sizeof(StrgyCMDEnableMessage), read_len - sizeof(StrgyCMDEnableMessage));
                }
            } else {
                printf("  [ERROR] Payload too short for Set Command (Need 2 bytes)\n");
            }
        } 
        else if (m_type == STRGY_MSG_QUERY) {
            printf("  [CMD] Operation: Query Strategy\n");
            if (read_len > 0) {
                printf("  [WARNING] Query CMD should have 0 payload, but found %u bytes.\n", read_len);
                print_hex_block("Unexpected Data", payload_start, read_len);
            } else {
                printf("  [INFO] Payload is empty (Correct)\n");
            }
        }
        else {
            printf("  [UNKNOWN] Unknown Strategy Msg Type in CMD: %u\n", m_type);
        }
    } 
    else if (a_type == ACTION_TYPE_RESPONSE) {
        if (read_len < 2) {
            printf("  [ERROR] Response payload too short (Need Result+Error)\n");
            return;
        }

        uint8_t result = payload_start[0];
        uint8_t err_code = payload_start[1];
        
        printf("  [RSP] Result: %s (0x%02X)\n", result == 1 ? "SUCCESS" : "FAILED", result);
        printf("  [RSP] Error Code: 0x%02X\n", err_code);

        if (m_type == STRGY_MSG_SET) {
            printf("  [RSP] Context: Set Strategy Response\n");
            if (read_len > 2) {
                print_hex_block("Extra Data", payload_start + 2, read_len - 2);
            }
        } 
        else if (m_type == STRGY_MSG_QUERY) {
            printf("  [RSP] Context: Query Strategy Response\n");
            if (read_len >= 4) {
                // Read current active strategy ID (2 bytes)
                uint16_t active_strgy_id = *(uint16_t*)(payload_start + 2);
                printf("  [RSP] Current Active Strategy: %s (%u)\n", 
                       get_strgy_param_str(active_strgy_id), active_strgy_id);
                
                if (read_len > 4) {
                    print_hex_block("Extra Data", payload_start + 4, read_len - 4);
                }
            } else {
                printf("  [ERROR] Query Response payload too short (Need 4 bytes: Result+Error+StrgyID)\n");
            }
        }
        else {
            printf("  [UNKNOWN] Unknown Strategy Msg Type in RSP: %u\n", m_type);
        }
    }
    else {
        printf("  [ERROR] Unknown Action Type: %u\n", a_type);
    }
    printf("  ------------------------------\n");
}


/**
 * @brief Dump content of a Session Message (Placeholder).
 * @param header Pointer to the raw buffer starting at ProxyMsgHeader.
 */
void proxy_sess_msg_content_dump(const uint8_t* header){
    // 1. Parse Proxy Header
    const ProxyMsgHeader* proxy_hdr = (const ProxyMsgHeader*)header;
    
    if (proxy_hdr->payload_len < sizeof(SessMsgHeader)) {
        printf("  [ERROR] Proxy payload too short for SessMsgHeader\n");
        return;
    }

    // 2. Calculate offset to get SessMsgHeader
    const SessMsgHeader* sess_hdr = (const SessMsgHeader*)(header + sizeof(ProxyMsgHeader));
    
    // 3. Calculate Payload start address
    const uint8_t* payload_start = header + sizeof(ProxyMsgHeader) + sizeof(SessMsgHeader);
    
    // Calculate actual available space
    uint16_t available_space = proxy_hdr->payload_len - sizeof(SessMsgHeader);

    printf("  --- Session Message Details ---\n");
    printf("  [Link] Frontend Session: %u, Backend Session: %u\n", 
           proxy_hdr->frontend_sess_id, proxy_hdr->backend_sess_id);
           
    printf("  Sess Version: %u\n", sess_hdr->version);
    printf("  Msg Type: %s (%u)\n", get_sess_msg_type_str(sess_hdr->msg_type), sess_hdr->msg_type);
    printf("  Action Type: %s (%u)\n", get_action_type_str(sess_hdr->action_type), sess_hdr->action_type);
    printf("  IP Version: %s (%u)\n", get_sess_ip_version_str(sess_hdr->ip_version), sess_hdr->ip_version);
    printf("  Sess Payload Len: %u\n", sess_hdr->payload_len);

    if (sess_hdr->payload_len > available_space) {
        printf("  [WARNING] Declared payload len (%u) > Available space (%u). Truncating read.\n", 
               sess_hdr->payload_len, available_space);
    }

    uint16_t m_type = sess_hdr->msg_type;
    uint16_t a_type = sess_hdr->action_type;
    uint16_t ip_ver = sess_hdr->ip_version;
    uint16_t read_len = (sess_hdr->payload_len < available_space) ? sess_hdr->payload_len : available_space;

    // Special Case: Non-IP Protocol
    if (ip_ver == SESS_NON_IP_PROTO) {
        printf("  [INFO] Non-IP Protocol detected. No further payload interpretation performed.\n");
        if (read_len > 0) {
            print_hex_block("Raw Non-IP Payload", payload_start, read_len);
        }
        printf("  ------------------------------\n");
        return;
    }

    // --- Logic Branches ---
    if (a_type == ACTION_TYPE_COMMAND) {
        if (m_type == SESS_MSG_CREATE) {
            printf("  [CMD] Operation: Create Session\n");
            
            if (ip_ver == SESS_IPV4_PROTO) {
                if (read_len < sizeof(SessParaIPv4)) {
                    printf("  [ERROR] Payload too short for IPv4 Create Params (Need %zu bytes)\n", sizeof(SessParaIPv4));
                    return;
                }
                const SessParaIPv4* params = (const SessParaIPv4*)payload_start;
                printf("  [CMD] Device ID: %u\n", params->dev_id);
                printf("  [CMD] Transport: %s (%u)\n", get_sess_trans_proto_str(params->trans_proto), params->trans_proto);
                printf("  [CMD] IPv4 Address: %u.%u.%u.%u\n", 
                       params->ipv4_addr.data[0], params->ipv4_addr.data[1], 
                       params->ipv4_addr.data[2], params->ipv4_addr.data[3]);
                printf("  [CMD] Port: %u\n", params->port);
                
                if (read_len > sizeof(SessParaIPv4)) {
                    print_hex_block("Extra Data", payload_start + sizeof(SessParaIPv4), read_len - sizeof(SessParaIPv4));
                }
            } 
            else if (ip_ver == SESS_IPV6_PROTO) {
                if (read_len < sizeof(SessParaIPv6)) {
                    printf("  [ERROR] Payload too short for IPv6 Create Params (Need %zu bytes)\n", sizeof(SessParaIPv6));
                    return;
                }
                const SessParaIPv6* params = (const SessParaIPv6*)payload_start;
                printf("  [CMD] Device ID: %u\n", params->dev_id);
                printf("  [CMD] Transport: %s (%u)\n", get_sess_trans_proto_str(params->trans_proto), params->trans_proto);
                
                // Print IPv6 Address
                printf("  [CMD] IPv6 Address: ");
                for (int i = 0; i < 16; i += 2) {
                    printf("%02x%02x", params->ipv6_addr.data[i], params->ipv6_addr.data[i+1]);
                    if (i < 14) printf(":");
                }
                printf("\n");
                
                printf("  [CMD] Port: %u\n", params->port);

                if (read_len > sizeof(SessParaIPv6)) {
                    print_hex_block("Extra Data", payload_start + sizeof(SessParaIPv6), read_len - sizeof(SessParaIPv6));
                }
            }
            else {
                printf("  [ERROR] Unknown IP Version %u for Create Command\n", ip_ver);
            }
        } 
        else if (m_type == SESS_MSG_CLOSE) {
            printf("  [CMD] Operation: Close Session\n");
            if (read_len > 0) {
                printf("  [WARNING] Close CMD should have 0 payload, but found %u bytes.\n", read_len);
                print_hex_block("Unexpected Data", payload_start, read_len);
            } else {
                printf("  [INFO] Payload is empty (Correct)\n");
            }
        }
        else {
            printf("  [UNKNOWN] Unknown Session Msg Type in CMD: %u\n", m_type);
        }
    } 
    else if (a_type == ACTION_TYPE_RESPONSE) {
        // Both Create and Close Responses use SessOpRespData
        if (m_type == SESS_MSG_CREATE || m_type == SESS_MSG_CLOSE) {
            printf("  [RSP] Context: %s Session Response\n", m_type == SESS_MSG_CREATE ? "Create" : "Close");
            
            if (read_len < sizeof(SessOpRespData)) {
                printf("  [ERROR] Response payload too short (Need Status+Code)\n");
                return;
            }

            const SessOpRespData* resp = (const SessOpRespData*)payload_start;
            
            printf("  [RSP] Status: %s (%u)\n", get_sess_op_status_str(resp->status), resp->status);
            printf("  [RSP] Code: %s (%u)\n", get_sess_op_code_str(resp->code), resp->code);

            if (read_len > sizeof(SessOpRespData)) {
                print_hex_block("Extra Data", payload_start + sizeof(SessOpRespData), read_len - sizeof(SessOpRespData));
            }
        }
        else {
            printf("  [UNKNOWN] Unknown Session Msg Type in RSP: %u\n", m_type);
        }
    }
    else {
        printf("  [ERROR] Unknown Action Type: %u\n", a_type);
    }
    printf("  ------------------------------\n");
}


/**
 * @brief Dump content of a Data Message (Placeholder).
 * @param header Pointer to the raw buffer starting at ProxyMsgHeader.
 */
void proxy_data_msg_content_dump(const uint8_t* header){
    const ProxyMsgHeader* proxy_hdr = (const ProxyMsgHeader*)header;
    printf("  --- Data Message (Transparent) ---\n");
    printf("  [Link] Session IDs: FE=%u, BE=%u\n", proxy_hdr->frontend_sess_id, proxy_hdr->backend_sess_id);
    if (proxy_hdr->payload_len > 0) {
        const uint8_t* payload = header + sizeof(ProxyMsgHeader);
        size_t print_len = (proxy_hdr->payload_len > 32) ? 32 : proxy_hdr->payload_len;
        print_hex_block("Data Preview", payload, print_len);
        if (proxy_hdr->payload_len > 32) printf("  ... (%u bytes total)\n", proxy_hdr->payload_len);
    }
}




/**
 * @brief Parses the custom proxy protocol packet and prints details directly.
 * 
 * @note This function does NOT take a buffer length parameter. It relies entirely 
 * on the 'payload_len' field in the ProxyMsgHeader. Ensure the input buffer is 
 * valid and large enough to avoid segmentation faults.
 * 
 * @param buffer Pointer to the input buffer containing the full packet.
 * @return int: BACKEND_PROXY_PROCESS_OK on success, BACKEND_PROXY_PROCESS_ERROR on failure.
 */
int parse_proxy_protocol_and_print(const uint8_t *buffer) {
    if (!buffer) {
        printf("[PROXY_PARSE_ERR] Input buffer is NULL.\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    /* 1. Parse Proxy Header */
    const ProxyMsgHeader *hdr = (const ProxyMsgHeader *)buffer;
    
    if (hdr->version != 1) {
        printf("[PROXY_PARSE_ERR] Invalid protocol version: %u (Expected 1).\n", hdr->version);
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    const uint8_t *payload_start = buffer + sizeof(ProxyMsgHeader);
    
    printf("=== Proxy Packet Received ===\n");
    printf("Header: Ver=%u, Type=%u, F-SID=%u, B-SID=%u, PayloadLen=%u\n",
           hdr->version, hdr->proxy_msg_type, hdr->frontend_sess_id, 
           hdr->backend_sess_id, hdr->payload_len);

    /* 2. Dispatch based on message type */
    switch (hdr->proxy_msg_type) {
        case PROXY_MSG_TYPE_DEV: {
            if (hdr->payload_len < sizeof(DevMsgHeader)) {
                printf("[PROXY_PARSE_ERR] Device message payload too short.\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }
            const DevMsgHeader *dev_hdr = (const DevMsgHeader *)payload_start;
            
            const char *type_str = (dev_hdr->msg_type == DEV_MSG_DISABLE) ? "DISABLE" : 
                                   (dev_hdr->msg_type == DEV_MSG_ENABLE) ? "ENABLE" : "QUERY";
            const char *action_str = (dev_hdr->action_type == ACTION_TYPE_COMMAND) ? "CMD" : "RESP";

            printf("[DEVICE MSG] Type=%s(%u), Action=%s(%u), MsgID=%u, SubLen=%u\n", 
                   type_str, dev_hdr->msg_type, action_str, dev_hdr->action_type, 
                   dev_hdr->msg_id, dev_hdr->payload_len);

            size_t content_offset = sizeof(DevMsgHeader);
            /* Check for underflow just in case, though logic above should prevent it */
            if (hdr->payload_len < content_offset) { 
                 return BACKEND_PROXY_PROCESS_ERROR; 
            }
            
            uint16_t content_len = hdr->payload_len - content_offset;
            const uint8_t *content = payload_start + content_offset;

            if (content_len > 0) {
                printf("  Content Hex: ");
                for (uint16_t i = 0; i < content_len && i < 16; i++) {
                    printf("%02X ", content[i]);
                }
                if (content_len > 16) printf("...");
                printf("\n");
                
                if (dev_hdr->action_type == ACTION_TYPE_RESPONSE && content_len >= 2) {
                    printf("  -> Result: Success=%u, ErrCode=%u\n", content[0], content[1]);
                }
            } else {
                printf("  Content: (Empty)\n");
            }
            break;
        }

        case PROXY_MSG_TYPE_STRGY: {
            if (hdr->payload_len < sizeof(StrgyMsgHeader)) {
                printf("[PROXY_PARSE_ERR] Strategy message payload too short.\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }
            const StrgyMsgHeader *strgy_hdr = (const StrgyMsgHeader *)payload_start;
            
            const char *type_str = (strgy_hdr->msg_type == STRGY_MSG_SET) ? "SET" : "QUERY";
            const char *action_str = (strgy_hdr->action_type == ACTION_TYPE_COMMAND) ? "CMD" : "RESP";

            printf("[STRATEGY MSG] Type=%s(%u), Action=%s(%u), MsgID=%u, SubLen=%u\n", 
                   type_str, strgy_hdr->msg_type, action_str, strgy_hdr->action_type, 
                   strgy_hdr->msg_id, strgy_hdr->payload_len);
            break;
        }

        case PROXY_MSG_TYPE_SESS: {
            if (hdr->payload_len < sizeof(SessMsgHeader)) {
                printf("[PROXY_PARSE_ERR] Session message payload too short.\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }
            const SessMsgHeader *sess_hdr = (const SessMsgHeader *)payload_start;

            if (sess_hdr->ip_version != SESS_IPV4_PROTO && sess_hdr->ip_version != SESS_IPV6_PROTO) {
                printf("[PROXY_PARSE_ERR] Invalid IP Version: %u (Must be 4 or 6).\n", sess_hdr->ip_version);
                return BACKEND_PROXY_PROCESS_ERROR;
            }

            const char *type_str = (sess_hdr->msg_type == SESS_MSG_CREATE) ? "CREATE" : "CLOSE";
            const char *action_str = (sess_hdr->action_type == ACTION_TYPE_COMMAND) ? "CMD" : "RESP";
            const char *ip_str = (sess_hdr->ip_version == SESS_IPV4_PROTO) ? "IPv4" : "IPv6";

            printf("[SESSION MSG] Type=%s(%" PRIu16 "), Action=%s(%" PRIu16 "), IP=%s(%" PRIu16 ")\n", 
                type_str, sess_hdr->msg_type, action_str, sess_hdr->action_type, ip_str, sess_hdr->ip_version);

            size_t header_consumed = sizeof(SessMsgHeader);
            if (hdr->payload_len < header_consumed) {
                return BACKEND_PROXY_PROCESS_ERROR;
            }
            
            uint16_t inner_len = hdr->payload_len - header_consumed;
            const uint8_t *inner_payload = payload_start + header_consumed;

            if (sess_hdr->msg_type == SESS_MSG_CREATE && sess_hdr->action_type == ACTION_TYPE_COMMAND) {
                if (sess_hdr->ip_version == SESS_IPV4_PROTO) {
                    if (inner_len < sizeof(SessParaIPv4)) {
                        printf("[PROXY_PARSE_ERR] IPv4 Create payload incomplete.\n");
                        return BACKEND_PROXY_PROCESS_ERROR;
                    }
                    const SessParaIPv4 *v4 = (const SessParaIPv4 *)inner_payload;
                    printf("  -> Target: DevID=%u, Proto=%u, Addr=%u.%u.%u.%u:%u\n",
                           v4->dev_id, v4->trans_proto,
                           v4->ipv4_addr.data[0], v4->ipv4_addr.data[1], 
                           v4->ipv4_addr.data[2], v4->ipv4_addr.data[3],
                           v4->port);
                } else {
                    if (inner_len < sizeof(SessParaIPv6)) {
                        printf("[PROXY_PARSE_ERR] IPv6 Create payload incomplete.\n");
                        return BACKEND_PROXY_PROCESS_ERROR;
                    }
                    const SessParaIPv6 *v6 = (const SessParaIPv6 *)inner_payload;
                    printf("  -> Target: DevID=%u, Proto=%u, Addr=IPv6(%02x%02x:...)\n",
                           v6->dev_id, v6->trans_proto,
                           v6->ipv6_addr.data[0], v6->ipv6_addr.data[1]);
                }
            } else if (sess_hdr->action_type == ACTION_TYPE_RESPONSE) {
                if (inner_len < 2) {
                    printf("[PROXY_PARSE_ERR] Session Response payload too short.\n");
                    return BACKEND_PROXY_PROCESS_ERROR;
                }
                printf("  -> Result: Success=%u, ErrCode=%u\n", inner_payload[0], inner_payload[1]);
            } else {
                printf("  -> Content: (Empty or Command Close)\n");
            }
            break;
        }

        case PROXY_MSG_TYPE_DATA: {
            printf("[DATA MSG] Raw Data Length: %u bytes\n", hdr->payload_len);
            if (hdr->payload_len > 0) {
                printf("  First 16 bytes: ");
                uint16_t print_len = (hdr->payload_len < 16) ? hdr->payload_len : 16;
                for (uint16_t i = 0; i < print_len; i++) {
                    printf("%02X ", payload_start[i]);
                }
                printf("\n");
            }
            break;
        }

        case PROXY_MSG_TYPE_IOT: {
            if (hdr->payload_len < sizeof(IotMsgHeader)) {
                printf("[PROXY_PARSE_ERR] IoT message payload too short for header.\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }
            const IotMsgHeader *iot_hdr = (const IotMsgHeader *)payload_start;

            const char *proto_str = "UNKNOWN";
            switch (iot_hdr->proto_type) {
                case IOT_PROTO_TYPE_BLUETOOTH: proto_str = "BLUETOOTH"; break;
                case IOT_PROTO_TYPE_ZIGBEE: proto_str = "ZIGBEE"; break;
                case IOT_PROTO_TYPE_CAN: proto_str = "CAN"; break;
                case IOT_PROTO_TYPE_LORA: proto_str = "LORA"; break;
                case IOT_PROTO_TYPE_POWERLINK: proto_str = "POWERLINK"; break;
                case IOT_PROTO_TYPE_MODBUSTCP: proto_str = "MODBUS_TCP"; break;
            }

            printf("[IOT MSG] Proto=%s(%u), Opcode=%u, PortID=%u, PayloadLen=%u\n",
                   proto_str, iot_hdr->proto_type, iot_hdr->opcode, 
                   iot_hdr->dev_port_id, iot_hdr->payload_len);

            size_t header_consumed = sizeof(IotMsgHeader);
            if (hdr->payload_len < header_consumed) {
                return BACKEND_PROXY_PROCESS_ERROR;
            }

            size_t addr_size = 0;
            switch (iot_hdr->proto_type) {
                case IOT_PROTO_TYPE_BLUETOOTH: addr_size = sizeof(IotBtAddr); break;
                case IOT_PROTO_TYPE_CAN: addr_size = sizeof(IotCanAddr); break;
                case IOT_PROTO_TYPE_ZIGBEE: addr_size = sizeof(IotZigbeeAddr); break;
                case IOT_PROTO_TYPE_LORA: addr_size = sizeof(IotLoraAddr); break;
                case IOT_PROTO_TYPE_POWERLINK: addr_size = sizeof(IotPowerLinkAddr); break;
                case IOT_PROTO_TYPE_MODBUSTCP: addr_size = sizeof(IotModbusTcpAddr); break;
                default:
                    printf("[PROXY_PARSE_ERR] Unknown IoT Protocol Type: %u\n", iot_hdr->proto_type);
                    return BACKEND_PROXY_PROCESS_ERROR;
            }

            /* Calculate total expected length inside payload */
            /* Note: We trust iot_hdr->payload_len here as well */
//            printf("hdr->payload_len = %d, header_consumed = %zu, addr_size = %zu, iot_hdr->payload_len = %d\n", hdr->payload_len, header_consumed, addr_size, iot_hdr->payload_len);
            if ((size_t)hdr->payload_len < header_consumed + iot_hdr->payload_len) {
                printf("[PROXY_PARSE_ERR] IoT message internal length mismatch.\n");
                return BACKEND_PROXY_PROCESS_ERROR;
            }

            const uint8_t *addr_ptr = payload_start + header_consumed;
            const uint8_t *data_ptr = addr_ptr + addr_size;

            printf("  Address Info: ");
            switch (iot_hdr->proto_type) {
                case IOT_PROTO_TYPE_BLUETOOTH: {
                    const IotBtAddr *addr = (const IotBtAddr *)addr_ptr;
                    /* Ensure null termination for safety */
                    char safe_mac[19];
                    memcpy(safe_mac, addr->mac, 18);
                    safe_mac[18] = '\0';
                    printf("MAC=%s, Port=%u\n", safe_mac, addr->port);
                    break;
                }
                case IOT_PROTO_TYPE_CAN: {
                    const IotCanAddr *addr = (const IotCanAddr *)addr_ptr;
                    printf("BusID=%u, CanID=0x%X, Port=%u\n", addr->bus_id, addr->can_id, addr->port);
                    break;
                }
                case IOT_PROTO_TYPE_ZIGBEE: {
                    const IotZigbeeAddr *addr = (const IotZigbeeAddr *)addr_ptr;
                    printf("PAN=0x%04X, EP=%u, MAC=", addr->pan_id, addr->endpoint);
                    for(int i=0; i<8; i++) printf("%02X", addr->mac[i]);
                    printf("\n");
                    break;
                }
                case IOT_PROTO_TYPE_LORA: {
                    const IotLoraAddr *addr = (const IotLoraAddr *)addr_ptr;
                    printf("DevEUI=%llu, Port=%u, Band=%u\n", (unsigned long long)addr->dev_eui, addr->port, addr->freq_band);
                    break;
                }
                case IOT_PROTO_TYPE_POWERLINK: {
                    const IotPowerLinkAddr *addr = (const IotPowerLinkAddr *)addr_ptr;
                    printf("NodeID=%u, PDO=%u, MAC=", addr->node_id, addr->pdo_id);
                    for(int i=0; i<6; i++) printf("%02X", addr->mac[i]);
                    printf("\n");
                    break;
                }
                case IOT_PROTO_TYPE_MODBUSTCP: {
                    const IotModbusTcpAddr *addr = (const IotModbusTcpAddr *)addr_ptr;
                    printf("IP=%u.%u.%u.%u, Port=%u\n", addr->ip[0], addr->ip[1], addr->ip[2], addr->ip[3], addr->port);
                    printf("    UnitID=%u, RegAddr=%hu, RegNum=%hu\n",
                           addr->unit_id, addr->reg_addr, addr->reg_num);
                    break;
                }
            }

            if (iot_hdr->payload_len > 0) {
                printf("  IoT Payload (First 16 bytes): ");
                uint16_t print_len = (iot_hdr->payload_len < 16) ? iot_hdr->payload_len : 16;
                for (uint16_t i = 0; i < print_len; i++) {
                    printf("%02X ", data_ptr[i]);
                }
                printf("\n");
            }
            break;
        }

        default:
            printf("[PROXY_PARSE_ERR] Unknown Proxy Message Type: %u\n", hdr->proxy_msg_type);
            return BACKEND_PROXY_PROCESS_ERROR;
    }

    printf("===========================\n");
    return BACKEND_PROXY_PROCESS_OK;
}


#if 0
/**
 * @brief Prints the detailed information of a GeneralProxyMsgHeader.
 * 
 * This function parses the outer header and, depending on the message type,
 * parses the specific inner header and IoT address information.
 * 
 * @param hdr Pointer to the GeneralProxyMsgHeader structure.
 */
void print_general_proxy_msg_header(const GeneralProxyMsgHeader *hdr) {
    if (hdr == NULL) {
        printf("[ERROR] Input header pointer is NULL.\n");
        return;
    }

    const ProxyMsgHeader *outer = &hdr->outer_header;
    
    printf("=== GeneralProxyMsgHeader Dump ===\n");
    printf("[Outer Header]\n");
    printf("  Version:          %u\n", outer->version);
    printf("  Msg Type:         %u ", outer->proxy_msg_type);
    
    // Print human-readable message type
    switch (outer->proxy_msg_type) {
        case PROXY_MSG_TYPE_DEV:   printf("(DEV)\n"); break;
        case PROXY_MSG_TYPE_STRGY: printf("(STRATEGY)\n"); break;
        case PROXY_MSG_TYPE_SESS:  printf("(SESSION)\n"); break;
        case PROXY_MSG_TYPE_DATA:  printf("(DATA)\n"); break;
        case PROXY_MSG_TYPE_IOT:   printf("(IOT)\n"); break;
        default:                   printf("(UNKNOWN)\n"); break;
    }

    printf("  Frontend Sess ID: %u\n", outer->frontend_sess_id);
    printf("  Backend Sess ID:  %u\n", outer->backend_sess_id);
    printf("  Payload Len:      %u\n", outer->payload_len);

    // Parse Inner Header based on message type
    switch (outer->proxy_msg_type) {
        case PROXY_MSG_TYPE_DEV: {
            const DevMsgHeader *dev = (const DevMsgHeader *)&hdr->inner_header.dev_hdr;
            printf("[Inner Header: DEV]\n");
            printf("  Version:       %u\n", dev->version);
            printf("  Msg Type:      %u\n", dev->msg_type);
            printf("  Msg ID:        %u\n", dev->msg_id);
            printf("  Action Type:   %u\n", dev->action_type);
            printf("  Payload Len:   %u\n", dev->payload_len);
            break;
        }

        case PROXY_MSG_TYPE_STRGY: {
            const StrgyMsgHeader *strgy = (const StrgyMsgHeader *)&hdr->inner_header.strgy_hdr;
            printf("[Inner Header: STRATEGY]\n");
            printf("  Version:       %u\n", strgy->version);
            printf("  Msg Type:      %u\n", strgy->msg_type);
            printf("  Msg ID:        %u\n", strgy->msg_id);
            printf("  Action Type:   %u\n", strgy->action_type);
            printf("  Payload Len:   %u\n", strgy->payload_len);
            break;
        }

        case PROXY_MSG_TYPE_SESS: {
            const SessMsgHeader *sess = (const SessMsgHeader *)&hdr->inner_header.sess_hdr;
            printf("[Inner Header: SESSION]\n");
            printf("  Version:       %u\n", sess->version);
            printf("  Msg Type:      %u\n", sess->msg_type);
            printf("  Action Type:   %u\n", sess->action_type);
            printf("  IP Version:    %u\n", sess->ip_version);
            printf("  Payload Len:   %u\n", sess->payload_len);
            break;
        }

        case PROXY_MSG_TYPE_DATA: {
            printf("[Inner Header: DATA]\n");
            printf("  (No specific inner header to parse for DATA messages)\n");
            break;
        }

        case PROXY_MSG_TYPE_IOT: {
            const IotMsgHeader *iot_hdr = (const IotMsgHeader *)&hdr->inner_header.iot_hdr;
            printf("[Inner Header: IOT]\n");
            printf("  Proto Ver:     %u\n", iot_hdr->proto_ver);
            printf("  Proto Type:    %u ", iot_hdr->proto_type);
            
            // Print human-readable protocol type
            switch (iot_hdr->proto_type) {
                case IOT_PROTO_TYPE_BLUETOOTH: printf("(BLUETOOTH)\n"); break;
                case IOT_PROTO_TYPE_ZIGBEE:    printf("(ZIGBEE)\n"); break;
                case IOT_PROTO_TYPE_CAN:       printf("(CAN)\n"); break;
                case IOT_PROTO_TYPE_LORA:      printf("(LORA)\n"); break;
                case IOT_PROTO_TYPE_POWERLINK: printf("(POWERLINK)\n"); break;
                case IOT_PROTO_TYPE_MODBUSTCP: printf("(MODBUS TCP)\n"); break;
                default:                       printf("(UNKNOWN)\n"); break;
            }

            printf("  Opcode:        %u\n", iot_hdr->opcode);
            printf("  Dev Port ID:   %u\n", iot_hdr->dev_port_id);
            printf("  Payload Len:   %u\n", iot_hdr->payload_len);
            printf("  Reserve:       %u\n", iot_hdr->reserve);

            // Parse IoT Address Information
            printf("[IoT Address Info]\n");
            printf("  Addr Len:      %u\n", hdr->iot_addr_len);
            
            if (hdr->iot_addr_len == 0) {
                printf("  (Address invalid/empty as length is 0)\n");
            } else {
                // Check consistency between header proto_type and address type
                if (hdr->iot_addr.addr_type != iot_hdr->proto_type) {
                    printf("  [WARNING] Mismatch: Header proto_type (%u) != Addr type (%u)\n", 
                           iot_hdr->proto_type, hdr->iot_addr.addr_type);
                }

                switch (hdr->iot_addr.addr_type) {
                    case IOT_PROTO_TYPE_UNKNOWN:
                        printf("  Type: UNKNOWN (No parsing performed)\n");
                        break;

                    case IOT_PROTO_TYPE_BLUETOOTH: {
                        const IotBtAddr *bt = &hdr->iot_addr.addr_info.bt_addr;
                        printf("  Type: BLUETOOTH\n");
                        // Assuming mac is a null-terminated string or fixed length
                        printf("  MAC:   %.17s\n", bt->mac); 
                        printf("  Port:  %u\n", bt->port);
                        break;
                    }

                    case IOT_PROTO_TYPE_ZIGBEE: {
                        const IotZigbeeAddr *zb = &hdr->iot_addr.addr_info.zigbee_addr;
                        printf("  Type: ZIGBEE\n");
                        printf("  MAC:     ");
                        for (int i = 0; i < 8; i++) {
                            printf("%02X", zb->mac[i]);
                            if (i < 7) printf(":");
                        }
                        printf("\n");
                        printf("  PAN ID:  0x%04X\n", zb->pan_id);
                        printf("  Endpoint:%u\n", zb->endpoint);
                        break;
                    }

                    case IOT_PROTO_TYPE_CAN: {
                        const IotCanAddr *can = &hdr->iot_addr.addr_info.can_addr;
                        printf("  Type: CAN\n");
                        printf("  Port:   %u\n", can->port);
                        printf("  CAN ID: 0x%08X\n", (unsigned int)can->can_id);
                        printf("  Bus ID: %u\n", can->bus_id);
                        break;
                    }

                    case IOT_PROTO_TYPE_LORA: {
                        const IotLoraAddr *lora = &hdr->iot_addr.addr_info.lora_addr;
                        printf("  Type: LORA\n");
                        printf("  Dev EUI:   0x%016" PRIx64 "\n", lora->dev_eui);
                        printf("  Port:      %u\n", lora->port);
                        printf("  Freq Band: %u\n", lora->freq_band);
                        break;
                    }

                    case IOT_PROTO_TYPE_POWERLINK: {
                        const IotPowerLinkAddr *pl = &hdr->iot_addr.addr_info.powerlink_addr;
                        printf("  Type: POWERLINK\n");
                        printf("  Node ID: %u\n", pl->node_id);
                        printf("  MAC:     ");
                        for (int i = 0; i < 6; i++) {
                            printf("%02X", pl->mac[i]);
                            if (i < 5) printf(":");
                        }
                        printf("\n");
                        printf("  PDO ID:  %u\n", pl->pdo_id);
                        break;
                    }

                    case IOT_PROTO_TYPE_MODBUSTCP: {
                        const IotModbusTcpAddr *mb = &hdr->iot_addr.addr_info.modbus_tcp_addr;
                        printf("  Type: MODBUS TCP\n");
                        printf("  IP:      %u.%u.%u.%u\n", mb->ip[0], mb->ip[1], mb->ip[2], mb->ip[3]);
                        printf("  Port:    %u\n", mb->port);
                        break;
                    }

                    default:
                        printf("  Type: UNSUPPORTED (%u)\n", hdr->iot_addr.addr_type);
                        // Optional: Print raw bytes for debugging
                        printf("  Raw Bytes: ");
                        for(int i=0; i<16; i++) printf("%02X ", hdr->iot_addr.addr_info.raw[i]);
                        printf("\n");
                        break;
                }
            }
            break;
        }

        default:
            printf("[Inner Header]: Unknown proxy_msg_type (%u), cannot parse inner header.\n", outer->proxy_msg_type);
            break;
    }

    PRINT_SEPARATOR();
}
#endif


/**
 * @brief Helper to print IotAddr based solely on type (no len param).
 */
void print_iot_addr_details(const IotAddr *addr) {
    if (addr == NULL) return;

    printf("  Protocol Type: %u ", addr->addr_type);
    
    switch (addr->addr_type) {
        case IOT_PROTO_TYPE_UNKNOWN:
            printf("(UNKNOWN)\n");
            return;
        case IOT_PROTO_TYPE_BLUETOOTH:
            printf("(BLUETOOTH)\n");
            {
                const IotBtAddr *bt = &addr->addr_info.bt_addr;
                printf("  MAC:   %.17s\n", bt->mac);
                printf("  Port:  %u\n", bt->port);
            }
            break;
        case IOT_PROTO_TYPE_ZIGBEE:
            printf("(ZIGBEE)\n");
            {
                const IotZigbeeAddr *zb = &addr->addr_info.zigbee_addr;
                printf("  MAC:     ");
                for (int i = 0; i < 8; i++) { printf("%02X", zb->mac[i]); if(i<7) printf(":"); }
                printf("\n  PAN ID:  0x%04X\n  Endpoint:%u\n", zb->pan_id, zb->endpoint);
            }
            break;
        case IOT_PROTO_TYPE_CAN:
            printf("(CAN)\n");
            {
                const IotCanAddr *can = &addr->addr_info.can_addr;
                printf("  Port:   %u\n  CAN ID: 0x%08X\n  Bus ID: %u\n", can->port, (unsigned int)can->can_id, can->bus_id);
            }
            break;
        case IOT_PROTO_TYPE_LORA:
            printf("(LORA)\n");
            {
                const IotLoraAddr *lora = &addr->addr_info.lora_addr;
                printf("  Dev EUI:   0x%016" PRIx64 "\n  Port: %u\n  Freq Band: %u\n", lora->dev_eui, lora->port, lora->freq_band);
            }
            break;
        case IOT_PROTO_TYPE_POWERLINK:
            printf("(POWERLINK)\n");
            {
                const IotPowerLinkAddr *pl = &addr->addr_info.powerlink_addr;
                printf("  Node ID: %u\n  MAC: ", pl->node_id);
                for (int i = 0; i < 6; i++) { printf("%02X", pl->mac[i]); if(i<5) printf(":"); }
                printf("\n  PDO ID:  %u\n", pl->pdo_id);
            }
            break;
        case IOT_PROTO_TYPE_MODBUSTCP:
            printf("(MODBUS TCP)\n");
            {
                const IotModbusTcpAddr *mb = &addr->addr_info.modbus_tcp_addr;
                printf("  IP:   %u.%u.%u.%u\n  Port: %u\n", mb->ip[0], mb->ip[1], mb->ip[2], mb->ip[3], mb->port);
                printf("    UnitID=%u, RegAddr=%hu, RegNum=%hu\n",
                           mb->unit_id, mb->reg_addr, mb->reg_num);
            }
            break;
        default:
            printf("(UNSUPPORTED)\n");
            break;
    }
}


/**
 * @brief Prints the GeneralProxyMsgHeader.
 * 
 * CORRECTION APPLIED: 
 * The length validation logic for IOT messages has been fixed.
 * Logic: hdr->payload_len MUST EQUAL (sizeof(IotMsgHeader) + iot_hdr->payload_len).
 * Where iot_hdr->payload_len includes both the Address and the Actual Data.
 */
void print_general_proxy_msg_header(const GeneralProxyMsgHeader *hdr) {
    if (hdr == NULL) {
        printf("[ERROR] Input header pointer is NULL.\n");
        return;
    }

    const ProxyMsgHeader *outer = &hdr->outer_header;
    
    printf("=== GeneralProxyMsgHeader Dump ===\n");
    printf("[Outer Header]\n");
    printf("  Version:          %u\n", outer->version);
    printf("  Msg Type:         %u ", outer->proxy_msg_type);
    
    switch (outer->proxy_msg_type) {
        case PROXY_MSG_TYPE_DEV:   printf("(DEV)\n"); break;
        case PROXY_MSG_TYPE_STRGY: printf("(STRATEGY)\n"); break;
        case PROXY_MSG_TYPE_SESS:  printf("(SESSION)\n"); break;
        case PROXY_MSG_TYPE_DATA:  printf("(DATA)\n"); break;
        case PROXY_MSG_TYPE_IOT:   printf("(IOT)\n"); break;
        default:                   printf("(UNKNOWN)\n"); break;
    }

    printf("  Frontend Sess ID: %u\n", outer->frontend_sess_id);
    printf("  Backend Sess ID:  %u\n", outer->backend_sess_id);
    printf("  Payload Len:      %u\n", outer->payload_len);

    switch (outer->proxy_msg_type) {
        case PROXY_MSG_TYPE_DEV: {
            const DevMsgHeader *dev = (const DevMsgHeader *)&hdr->inner_header.dev_hdr;
            printf("[Inner Header: DEV]\n");
            printf("  Version: %u, MsgType: %u, ID: %u, Action: %u, PayLen: %u\n",
                   dev->version, dev->msg_type, dev->msg_id, dev->action_type, dev->payload_len);
            break;
        }
        case PROXY_MSG_TYPE_STRGY: {
            const StrgyMsgHeader *s = (const StrgyMsgHeader *)&hdr->inner_header.strgy_hdr;
            printf("[Inner Header: STRATEGY]\n");
            printf("  Version: %u, MsgType: %u, ID: %u, Action: %u, PayLen: %u\n",
                   s->version, s->msg_type, s->msg_id, s->action_type, s->payload_len);
            break;
        }
        case PROXY_MSG_TYPE_SESS: {
            const SessMsgHeader *s = (const SessMsgHeader *)&hdr->inner_header.sess_hdr;
            printf("[Inner Header: SESSION]\n");
            printf("  Version: %u, MsgType: %u, Action: %u, IPVer: %u, PayLen: %u\n",
                   s->version, s->msg_type, s->action_type, s->ip_version, s->payload_len);
            break;
        }
        case PROXY_MSG_TYPE_DATA:
            printf("[Inner Header: DATA] (No specific inner header)\n");
            break;

        case PROXY_MSG_TYPE_IOT: {
            const IotMsgHeader *iot_hdr = (const IotMsgHeader *)&hdr->inner_header.iot_hdr;
            printf("[Inner Header: IOT]\n");
            printf("  Proto Ver:   %u\n", iot_hdr->proto_ver);
            printf("  Proto Type:  %u ", iot_hdr->proto_type);
            
            // Basic validation: Ensure payload_len is at least large enough for the smallest address?
            // Actually, we trust the type for printing, but we can log a warning if it looks too small.
            
            switch (iot_hdr->proto_type) {
                case IOT_PROTO_TYPE_BLUETOOTH: printf("(BLUETOOTH)\n"); break;
                case IOT_PROTO_TYPE_ZIGBEE:    printf("(ZIGBEE)\n"); break;
                case IOT_PROTO_TYPE_CAN:       printf("(CAN)\n"); break;
                case IOT_PROTO_TYPE_LORA:      printf("(LORA)\n"); break;
                case IOT_PROTO_TYPE_POWERLINK: printf("(POWERLINK)\n"); break;
                case IOT_PROTO_TYPE_MODBUSTCP: printf("(MODBUS TCP)\n"); break;
                default:                       printf("(UNKNOWN)\n"); break;
            }

            printf("  Opcode:      %u\n", iot_hdr->opcode);
            printf("  Dev Port ID: %u\n", iot_hdr->dev_port_id);
            printf("  Payload Len: %u (Includes Addr + Data)\n", iot_hdr->payload_len);
            printf("  Reserve:     %u\n", iot_hdr->reserve);

            // --- LENGTH VALIDATION FIX ---
            // The outer payload_len must cover: sizeof(IotMsgHeader) + iot_hdr->payload_len
            size_t header_consumed = sizeof(IotMsgHeader);
            
            // Correct Logic:
            // Total Available (outer) == Header Size + Declared Inner Payload (Addr + Data)
            if (outer->payload_len != (header_consumed + iot_hdr->payload_len)) {
                printf("  [WARNING] Length Mismatch!\n");
                printf("            Outer Payload (%u) != IotHeader Size (%zu) + Inner Payload (%u)\n",
                       outer->payload_len, header_consumed, iot_hdr->payload_len);
                printf("            Expected: %zu, Got: %u\n", 
                       (header_consumed + iot_hdr->payload_len), outer->payload_len);
            } else {
                printf("  [OK] Length validation passed.\n");
            }

            // Optional: Check if inner payload is at least big enough for the address type
            size_t min_addr_size = 0;
            switch (iot_hdr->proto_type) {
                case IOT_PROTO_TYPE_BLUETOOTH: min_addr_size = sizeof(IotBtAddr); break;
                case IOT_PROTO_TYPE_CAN:       min_addr_size = sizeof(IotCanAddr); break;
                case IOT_PROTO_TYPE_ZIGBEE:    min_addr_size = sizeof(IotZigbeeAddr); break;
                case IOT_PROTO_TYPE_LORA:      min_addr_size = sizeof(IotLoraAddr); break;
                case IOT_PROTO_TYPE_POWERLINK: min_addr_size = sizeof(IotPowerLinkAddr); break;
                case IOT_PROTO_TYPE_MODBUSTCP: min_addr_size = sizeof(IotModbusTcpAddr); break;
                default: min_addr_size = 0; break;
            }

            if (min_addr_size > 0 && iot_hdr->payload_len < min_addr_size) {
                printf("  [ERROR] Inner Payload (%u) is smaller than minimum Address Size (%zu) for this protocol!\n",
                       iot_hdr->payload_len, min_addr_size);
            }

            // Print Address Info
            // Note: Since this function takes a Struct Pointer, we assume 'hdr->iot_addr' 
            // has been correctly populated by the caller's parsing logic.
            printf("[IoT Address Info]\n");
            print_iot_addr_details(&hdr->iot_addr);
            
            break;
        }

        default:
            printf("[Inner Header]: Unknown type.\n");
            break;
    }

    PRINT_SEPARATOR();
}



/**
 * @brief Helper function to print IotAddr details.
 * 
 * Determines the expected size and content based solely on addr_type.
 * No external addr_len parameter is required.
 * 
 * @param addr Pointer to the IotAddr structure.
 */
void print_iot_addr(const IotAddr *addr) {
    if (addr == NULL) {
        printf("  [Address] NULL\n");
        return;
    }

    printf("  Protocol Type: %u ", addr->addr_type);
    
    // Print human-readable type name and determine expected size
    int expected_size = 0;
    switch (addr->addr_type) {
        case IOT_PROTO_TYPE_UNKNOWN:
            printf("(UNKNOWN)\n");
            printf("  (No address parsing for UNKNOWN type)\n");
            return;

        case IOT_PROTO_TYPE_BLUETOOTH:
            printf("(BLUETOOTH)\n");
            expected_size = sizeof(IotBtAddr); // 18 + 2 = 20 bytes
            break;

        case IOT_PROTO_TYPE_ZIGBEE:
            printf("(ZIGBEE)\n");
            expected_size = sizeof(IotZigbeeAddr); // 8 + 2 + 1 = 11 bytes
            break;

        case IOT_PROTO_TYPE_CAN:
            printf("(CAN)\n");
            expected_size = sizeof(IotCanAddr); // 2 + 4 + 1 = 7 bytes
            break;

        case IOT_PROTO_TYPE_LORA:
            printf("(LORA)\n");
            expected_size = sizeof(IotLoraAddr); // 8 + 2 + 1 = 11 bytes
            break;

        case IOT_PROTO_TYPE_POWERLINK:
            printf("(POWERLINK)\n");
            expected_size = sizeof(IotPowerLinkAddr); // 2 + 6 + 2 = 10 bytes
            break;

        case IOT_PROTO_TYPE_MODBUSTCP:
            printf("(MODBUS TCP)\n");
            expected_size = sizeof(IotModbusTcpAddr); // 4 + 2 = 6 bytes
            break;

        default:
            printf("(UNSUPPORTED)\n");
            printf("  Raw Bytes: ");
            for(int i=0; i<16; i++) printf("%02X ", addr->addr_info.raw[i]);
            printf("\n");
            return;
    }

    // Parse and print specific fields based on type
    switch (addr->addr_type) {
        case IOT_PROTO_TYPE_BLUETOOTH: {
            const IotBtAddr *bt = &addr->addr_info.bt_addr;
            printf("  MAC:   %.17s\n", bt->mac);
            printf("  Port:  %u\n", bt->port);
            break;
        }

        case IOT_PROTO_TYPE_ZIGBEE: {
            const IotZigbeeAddr *zb = &addr->addr_info.zigbee_addr;
            printf("  MAC:     ");
            for (int i = 0; i < 8; i++) {
                printf("%02X", zb->mac[i]);
                if (i < 7) printf(":");
            }
            printf("\n");
            printf("  PAN ID:  0x%04X\n", zb->pan_id);
            printf("  Endpoint:%u\n", zb->endpoint);
            break;
        }

        case IOT_PROTO_TYPE_CAN: {
            const IotCanAddr *can = &addr->addr_info.can_addr;
            printf("  Port:   %u\n", can->port);
            printf("  CAN ID: 0x%08X\n", (unsigned int)can->can_id);
            printf("  Bus ID: %u\n", can->bus_id);
            break;
        }

        case IOT_PROTO_TYPE_LORA: {
            const IotLoraAddr *lora = &addr->addr_info.lora_addr;
            printf("  Dev EUI:   0x%016" PRIx64 "\n", lora->dev_eui);
            printf("  Port:      %u\n", lora->port);
            printf("  Freq Band: %u\n", lora->freq_band);
            break;
        }

        case IOT_PROTO_TYPE_POWERLINK: {
            const IotPowerLinkAddr *pl = &addr->addr_info.powerlink_addr;
            printf("  Node ID: %u\n", pl->node_id);
            printf("  MAC:     ");
            for (int i = 0; i < 6; i++) {
                printf("%02X", pl->mac[i]);
                if (i < 5) printf(":");
            }
            printf("\n");
            printf("  PDO ID:  %u\n", pl->pdo_id);
            break;
        }

        case IOT_PROTO_TYPE_MODBUSTCP: {
            const IotModbusTcpAddr *mb = &addr->addr_info.modbus_tcp_addr;
            printf("  IP:      %u.%u.%u.%u\n", mb->ip[0], mb->ip[1], mb->ip[2], mb->ip[3]);
            printf("  Port:    %u\n", mb->port);
            break;
        }
        
        default:
            break;
    }
    
    // Optional: Print the calculated expected size for debugging verification
    printf("  (Expected Struct Size: %d bytes)\n", expected_size);
}


/**
 * @brief Prints the detailed information of an IotMsgBuffer.
 * 
 * Parses buffer metadata, decodes IotAddr based on type (no len param needed),
 * and prints a hex dump of the raw data. ext_info is not dereferenced.
 * 
 * @param buf Pointer to the IotMsgBuffer structure.
 */
void print_iot_msg_buffer(const IotMsgBuffer *buf) {
    if (buf == NULL) {
        printf("[ERROR] Input IotMsgBuffer pointer is NULL.\n");
        return;
    }

    printf("=== IotMsgBuffer Dump ===\n");
    
    // 1. Basic Metadata
    printf("[Metadata]\n");
    printf("  Message ID:    %" PRIu32 "\n", buf->msg_id);
    printf("  Timestamp:     %" PRIu64 " ms\n", buf->timestamp);
    printf("  Data Length:   %" PRIu32 " bytes\n", buf->len);
    printf("  Data Pointer:  %p\n", (void*)buf->data);
    
    if (buf->data == NULL && buf->len > 0) {
        printf("  [WARNING] Data length is > 0 but data pointer is NULL!\n");
    }

    // 2. Extended Info (Pointer only, not dereferenced)
    printf("[Extended Info]\n");
    if (buf->ext_info == NULL) {
        printf("  Ptr:           NULL\n");
    } else {
        printf("  Ptr:           %p (Content not parsed)\n", buf->ext_info);
    }

    // 3. Address Information (No addr_len passed)
    printf("[Address Information]\n");
    print_iot_addr(&buf->addr);

    // 4. Raw Data Hex Dump
    printf("[Raw Data Payload]\n");
    if (buf->data == NULL || buf->len == 0) {
        printf("  (No data to display)\n");
    } else {
        // Limit dump size to avoid flooding console
        uint32_t dump_len = (buf->len > 64) ? 64 : buf->len;
        
        printf("  Showing first %u of %u bytes:\n", dump_len, buf->len);
        for (uint32_t i = 0; i < dump_len; i++) {
            if (i % 16 == 0) {
                printf("  %04X: ", i);
            }
            printf("%02X ", buf->data[i]);
            
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
        if (dump_len % 16 != 0) {
            printf("\n");
        }
        
        if (buf->len > 64) {
            printf("  ... (%" PRIu32 " bytes omitted)\n", buf->len - 64);
        }
    }

    PRINT_SEPARATOR();
}