#include "common_utils.h"
#include "message.h"

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