#include "senario_test.h"

/**
 * @brief Global array storing all supported return values of result_msg
 * @details Each element corresponds to a specific test scenario or functional requirement in the document,
 *          ensuring consistency between result_msg's return values and the document's test logic
 */
const ResultMsgSupportedItem g_result_msg_supported_values[] = {
    // 1. Valid address in shared memory mode
    {
        RESULT_MSG_VALID_SHARED,
        0x0000000000000001,  // Example: Non-zero address within shared memory pool (per document's "shared memory pool handle" definition)
        "Valid message address located in the shared memory FIFO ring buffer.",
        MEMORY_ALLOC_SHARED
    },

    // 2. Valid address in caller-allocated mode
    {
        RESULT_MSG_VALID_CALLER,
        0x1000000000000001,  // Example: Address of caller-preallocated buffer (per document's "caller-allocated mode" requirement)
        "Valid message address located in the caller-preallocated buffer.",
        MEMORY_ALLOC_CALLER
    },

    // 3. Invalid address (UINT64_MAX) for full shared memory queue
    {
        RESULT_MSG_INVALID_FULL,
        UINT64_MAX,           // Explicitly defined in document Test 2.1.2: "Queue full → allocation fails, addr_ptr set to UINT64_MAX"
        "Invalid address (set to UINT64_MAX) caused by full shared memory queue.",
        MEMORY_ALLOC_SHARED
    },

    // 4. Invalid address for message construction failure
    {
        RESULT_MSG_INVALID_FAILED,
        0x0000000000000000,   // NULL-like address: No valid address on construction failure (document's general exception flag)
        "Invalid address (NULL-like) caused by message construction failure.",
        MEMORY_ALLOC_SHARED | MEMORY_ALLOC_CALLER  // Applies to both modes (construction failure can occur in either)
    }
};

// Length of the global array (facilitates traversal/lookup in test code, no document dependency)
const size_t g_result_msg_supported_count = sizeof(g_result_msg_supported_values) / sizeof(ResultMsgSupportedItem);


// GeneralProxyMsgHeader  dev_enable_msg_hdr, strgy_query_msg_hdr, sess_create_msg_hdr, data_msg_hdr;


GeneralProxyMsgHeader dev_enable_msg_hdr = {
    .outer_header = {
        .version            = PROXY_PROTO_VERSION_1,                       // Protocol version, fixed to 1 as specified
        .proxy_msg_type     = PROXY_MSG_TYPE_DEV,                          // Proxy message type: Device message (0)
        .frontend_sess_id   = FRONTEND_ADMIN_SESSION_ID,                   // Frontend admin session ID, used to match frontend-backend sessions in frontend proxy
        .backend_sess_id    = BACKEND_ADMIN_SESSION_ID,                    // Backend admin session ID, used to match backend-backend sessions in frontend proxy
        .payload_len        = sizeof(DevMsgHeader) + sizeof(DevMsgMask)    // Payload length, set according to actual payload
    },
    .inner_header.dev_hdr   = {                                            // Use device message inner header
        .version            = PROXY_PROTO_DEV_VERSION_1,                   // Protocol version, fixed to 1
        .msg_type           = DEV_MSG_ENABLE,                              // Message type: Enable (1)
        .msg_id             = 0,                                           // Message ID, used for command-response matching
        .action_type        = ACTION_TYPE_COMMAND,                         // Signaling type: Command (0)
        .payload_len        = sizeof(DevMsgMask)                           // Payload length, set according to actual payload
    }
};


// Strategy set message header
GeneralProxyMsgHeader strgy_set_msg_hdr = {
    .outer_header = {
        .version            = PROXY_PROTO_VERSION_1,                                    // Protocol version, fixed to 1 as specified
        .proxy_msg_type     = PROXY_MSG_TYPE_STRGY,                                     // Proxy message type: Strategy message (1)
        .frontend_sess_id   = FRONTEND_ADMIN_SESSION_ID,                                // Frontend admin session ID, used to match frontend-backend sessions in frontend proxy
        .backend_sess_id    = BACKEND_ADMIN_SESSION_ID,                                 // Backend admin session ID, used to match backend-backend sessions in frontend proxy
        .payload_len        = sizeof(StrgyMsgHeader) + sizeof(StrgyCMDEnableMessage)    // Payload length, set according to actual payload
    },
    .inner_header.strgy_hdr = {                                                         // Use strategy message inner header
        .version            = PROXY_PROTO_STRGY_VERSION_1,                              // Protocol version, fixed to 1
        .msg_type           = STRGY_MSG_SET,                                            // Message type: Set (0)
        .msg_id             = 0,                                                        // Message ID, used for command-response matching
        .action_type        = ACTION_TYPE_COMMAND,                                      // Signaling type: Command (0)
        .payload_len        = sizeof(StrgyCMDEnableMessage)                             // Payload length, set according to actual payload
    }
};


// Session create message header
GeneralProxyMsgHeader sess_create_msg_hdr = {
    .outer_header = {
        .version            = PROXY_PROTO_VERSION_1,                            // Protocol version, fixed to 1 as specified
        .proxy_msg_type     = PROXY_MSG_TYPE_SESS,                              // Proxy message type: Session message (2)
        .frontend_sess_id   = 1,                                                // Frontend admin session ID, used to match frontend-backend sessions in frontend proxy
        .backend_sess_id    = BACKEND_HANDOVER_SESSION_ID,                      // Backend admin session ID, set to BACKEND_HANDOVER_SESSION_ID to trigger the handover procedure 
        .payload_len        = sizeof(SessMsgHeader) + sizeof(SessIPv4Params)    // Payload length, set according to actual payload
    },
    .inner_header.sess_hdr  = {                                                 // Use session message inner header
        .version            = PROXY_PROTO_SESS_VERSION_1,                       // Protocol version, fixed to 1
        .msg_type           = SESS_MSG_CREATE,                                  // Message type: Create (0)
        .action_type        = ACTION_TYPE_COMMAND,                              // Signaling type: Command (0)
        .ip_version         = SESS_IPV4_PROTO,                                  // IP version: IPv4 (4), can be changed to IPv4 (6) if needed
        .payload_len        = sizeof(SessIPv4Params)                            // Payload length, set according to actual payload
    }
};


// data message header
GeneralProxyMsgHeader proxy_data_msg_hdr = {
    .outer_header = {
        .version            = PROXY_PROTO_VERSION_1,                            // Protocol version, fixed to 1 as specified
        .proxy_msg_type     = PROXY_MSG_TYPE_DATA,                              // Proxy message type: Session message (3)
        .frontend_sess_id   = 1,                                                // Frontend session ID, used to match frontend-backend sessions in frontend proxy
        .backend_sess_id    = 1,                                                // Backend  session ID, used to match backend-backend sessions in frontend proxy 
        .payload_len        = 0                                                 // Payload length, set according to actual payload
    },
};

/**
 * @brief Simulate front-end requests, construct proxy messages via build_proxy_general_message, and inject them into the shared memory RX queue of the back-end protocol stack
 * @details Implements the core responsibility of "scenario functions" in the "Backend Protocol Stack Unit Test.doc": 
 *          1. Calls the existing build_proxy_general_message to construct complete proxy messages (supports both shared memory and caller-allocated memory modes);
 *          2. Injects the constructed messages into the shared memory RX queue in compliance with FIFO read-write rules (avoids queue overflow or data overwriting);
 *          3. Returns detailed injection results to locate issues like queue initialization exceptions or message construction failures, 
 *             and triggers the back-end protocol stack's request processing logic (e.g., engine_run reads from RX queue).
 * 
 * @param[in]  engine            Pointer to a BackendEngine object containing the back-end proxy's global context (e.g., runtime configuration, memory pool handles).
 *                               Required by build_proxy_general_message for message construction (e.g., accessing memory management resources).
 *                               Must not be NULL (consistent with build_proxy_general_message's parameter constraint).
 * @param[in]  msg_header        Pointer to a GeneralProxyMsgHeader structure specifying the header of the injected message (e.g., message type, payload length).
 *                               Must not be NULL (passed to build_proxy_general_message as the "header" parameter) and comply with the protocol specification in the document.
 * @param[in]  msg_payload       Pointer to the const uint8_t buffer containing the message payload (business data like device status, strategy config).
 *                               Can be NULL only if msg_payload_len is 0 (consistent with build_proxy_general_message's payload constraint).
 * @param[in]  msg_payload_len   Length of msg_payload in bytes. Must be non-negative and match msg_header->payload_len (if the header has a payload length field)
 *                               to ensure message consistency (follows build_proxy_general_message's parameter rule).
 * @param[in]  alloc_mode        Memory allocation mode for message construction, using the same MemoryAllocMode enum as build_proxy_general_message:
 *                               - MEMORY_ALLOC_SHARED: Allocates memory in the shared memory FIFO ring buffer (rx_queue acts as ring_buf for build_proxy_general_message);
 *                               - MEMORY_ALLOC_CALLER: Requires the caller to pre-allocate memory (build_proxy_general_message populates the pre-allocated buffer).
 * @param[out] result_msg        Double pointer to receive the address of the constructed message (consistent with build_proxy_general_message's "result_msg" parameter):
 *                               - MEMORY_ALLOC_SHARED: Points to the message in the rx_queue's FIFO ring buffer (no caller deallocation needed);
 *                               - MEMORY_ALLOC_CALLER: Points to the caller's pre-allocated buffer (populated with the complete message).
 *                               Must not be NULL.
 * @param[out] result_desc       Buffer to store detailed injection results (e.g., "Request injected successfully into RX queue", "RX queue full, injection failed").
 *                               Helps locate test startup issues (document's "injection status feedback" requirement). Must not be NULL.
 * @param[in]  desc_len          Maximum length of the result_desc buffer to prevent buffer overflow (ensures safe result storage).
 * 
 * @return int                   Status code following the back-end protocol stack's unified specification (consistent with build_proxy_general_message's return type):
 *                               - BACKEND_PROXY_PROCESS_OK: Message constructed successfully and injected into RX queue;
 *                               - BACKEND_PROXY_PROCESS_ERROR: General error (e.g., build_proxy_general_message fails, rx_queue uninitialized);

 * 
 * @note 1. Depends on the existing build_proxy_general_message function (must ensure its correctness before using this injection function);
 *       2. Before injection, confirm BackendEngine is initialized (engine_init returns BACKEND_PROXY_PROCESS_OK) and rx_queue is ready (document's test prerequisite);
 *       3. For MEMORY_ALLOC_SHARED, ensure rx_queue is a valid SharedMemoryPoolQueue (used for FIFO ring buffer operations in build_proxy_general_message);
 *       4. For MEMORY_ALLOC_CALLER, the caller must guarantee the pre-allocated buffer (result_msg) is large enough to hold the complete message (header + payload)
 *          (follows build_proxy_general_message's constraint for this mode);
 *       5. Supports all message types in the document (device, strategy, session, data messages) by configuring msg_header->message_type.
 */
int scenario_msg_inject(BackendEngine *engine,
                        GeneralProxyMsgHeader *msg_header,
                        const uint8_t *msg_payload,
                        size_t msg_payload_len,
                        MemoryAllocMode alloc_mode,
                        uint8_t **result_msg,
                        char *result_desc,
                        size_t desc_len){
    int ret;
    
    // 1. Check if BackendEngine pointer is NULL (engine initialization is a prerequisite per "Backend Protocol Stack Unit Test.doc")
    if (engine == NULL)
    {
        error_print("scenario_msg_inject failed: BackendEngine pointer is NULL, engine must be initialized!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 2. Check if engine->rx_queue (shared memory RX queue) is NULL (queue initialization is required for message injection, per document)
    if (engine->rx_queue == NULL)
    {
        error_print("scenario_msg_inject failed: engine->rx_queue (SharedMemoryPoolQueue) is NULL, RX queue must be initialized via engine_init_shared_mem_queue");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 3. Check if GeneralProxyMsgHeader pointer is NULL (valid header is required for message parsing, per document's message handling rules)
    if (msg_header == NULL)
    {
        error_print("scenario_msg_inject failed: GeneralProxyMsgHeader pointer is NULL, valid message header is required");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 4. Check if result_msg double pointer is NULL (used to store constructed message address, per document's scenario function requirements)
    if (result_msg == NULL)
    {
        error_print("scenario_msg_inject failed: result_msg double pointer is NULL, cannot store address of constructed message");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 5. Check if result_desc pointer is NULL (used to feedback injection status, per document's "injection status feedback" requirement)
    if (result_desc == NULL)
    {
        error_print("scenario_msg_inject failed: result_desc pointer is NULL, cannot store injection result description");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 6. Check consistency between msg_payload and msg_payload_len (empty payload requires len=0, per document's data message integrity rules)
    if (msg_payload == NULL && msg_payload_len > 0)
    {
        error_print("scenario_msg_inject failed: msg_payload is NULL but msg_payload_len > 0, violates payload integrity rules");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 7. Check if MemoryAllocMode is valid (covers undefined modes, aligns with document's "shared memory/caller-allocated dual mode")
    if (alloc_mode != MEMORY_ALLOC_SHARED && alloc_mode != MEMORY_ALLOC_CALLER && alloc_mode != MEMORY_ALLOC_AMPQUEUE)
    {
        error_print("scenario_msg_inject failed: invalid MemoryAllocMode, only MEMORY_ALLOC_SHARED and MEMORY_ALLOC_CALLER are supported");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // 8. Check if desc_len is valid (prevents buffer overflow when writing result_desc, per safe coding practices in document context)
    if (desc_len == 0)
    {
        error_print("scenario_msg_inject failed: desc_len is 0, insufficient buffer size for result_desc");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    // ------------------------------
    // Normal logic starts here (e.g., call build_proxy_general_message, inject to engine->rx_queue)
    // ------------------------------
    // Example: Initialize result_desc with success info first (if no errors)
    // strncpy(result_desc, "scenario_msg_inject: request injection started", desc_len - 1);
    // result_desc[desc_len - 1] = '\0';

    // ... (call build_proxy_general_message, handle queue injection, etc.)

    utils_print("In %s, before enter build_proxy_general_message, build message type = %d, msg_payload_len = %d\n", __func__, msg_header->outer_header.proxy_msg_type, msg_payload_len);
    ret = build_proxy_general_message(engine, msg_header, msg_payload, msg_payload_len, result_msg, alloc_mode, engine->rx_queue);

    return ret;
}



/**
 * @brief Inject a device message into the backend engine
 * @details Handles injection of device-related messages, processing according to
 *          device management logic and returning results via output parameters.
 * 
 * @param engine Pointer to the BackendEngine instance
 * @return int Return code indicating processing result:
 *         - BACKEND_PROXY_PROCESS_OK: Message injected and processed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed to inject or process the message
 */
int device_msg_inject(BackendEngine *engine){
    GeneralProxyMsgHeader *dev_msg_hdr;
    DevMsgMask            dev_msg_mask;
    int                   ret, desc_len = 100;
    uint8_t               **res_string;
    char                  *desc_string;
    uint8_t               *res_buf[100] = {NULL};
    char                  desc_buf[100] = {0};

    dev_msg_hdr          = &dev_enable_msg_hdr;
    dev_msg_mask.data    = 0xFF;


    res_string           = res_buf;
    desc_string          = desc_buf;

    utils_print("In %s, before enter scenario_msg_inject\n", __func__);
    ret = scenario_msg_inject(engine, dev_msg_hdr, (const uint8_t *)&dev_msg_mask, sizeof(dev_msg_mask), MEMORY_ALLOC_SHARED, res_string, desc_string, desc_len);


    return ret;
}


/**
 * @brief Inject a strategy message into the backend engine
 * @details Handles injection of strategy/policy-related messages, processing according to
 *          strategy management logic and returning results via output parameters.
 * 
 * @param engine Pointer to the BackendEngine instance
 * @return int Return code indicating processing result:
 *         - BACKEND_PROXY_PROCESS_OK: Message injected and processed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed to inject or process the message
 */
int strategy_msg_inject(BackendEngine *engine){
    GeneralProxyMsgHeader  *strgy_msg_hdr;
    StrgyCMDEnableMessage  strgy;
    int                    ret, desc_len = 100;
    uint8_t               **res_string;
    char                  *desc_string;
    uint8_t               *res_buf[100] = {NULL};
    char                  desc_buf[100] = {0};


    strgy_msg_hdr          = &strgy_set_msg_hdr;
    strgy.strgy_para       = 0;

    res_string           = res_buf;
    desc_string          = desc_buf;

    ret = scenario_msg_inject(engine, strgy_msg_hdr, (const uint8_t *)&strgy, sizeof(strgy), MEMORY_ALLOC_SHARED, res_string, desc_string, desc_len);

    return ret;
}


/**
 * @brief Inject a session message into the backend engine
 * @details Handles injection of session-related messages, processing according to
 *          session management logic and returning results via output parameters.
 * 
 * @param engine Pointer to the BackendEngine instance
 * @return int Return code indicating processing result:
 *         - BACKEND_PROXY_PROCESS_OK: Message injected and processed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed to inject or process the message
 */
int session_msg_inject(BackendEngine *engine){
    GeneralProxyMsgHeader *sess_msg_hdr;
    SessIPv4Params sess_ipv4_paras;
    int ret, desc_len = 100;
    uint8_t **res_string;
    char *desc_string;
    uint8_t               *res_buf[100] = {NULL};
    char                  desc_buf[100] = {0};
    char *ip_port_string = "192.168.1.101:8888";

    sess_msg_hdr                            = &sess_create_msg_hdr;

    sess_ipv4_paras.transport_layer_proto   = SESS_UDP_PROTO;
    sess_ipv4_paras.device_selection        = 0xFF;
    IPV4_PORT_STR_TO_TUPLE(ip_port_string, sess_ipv4_paras.dest_endpoint);

    res_string           = res_buf;
    desc_string          = desc_buf;

    utils_print("In %s, version = %d, type = %d\n", __func__, sess_msg_hdr->outer_header.version, sess_msg_hdr->outer_header.proxy_msg_type);

    ret = scenario_msg_inject(engine, sess_msg_hdr, (const uint8_t *)&sess_ipv4_paras, sizeof(sess_ipv4_paras), MEMORY_ALLOC_SHARED, res_string, desc_string, desc_len);

    return ret;
}


/**
 * @brief Inject a data message into the backend engine
 * @details Handles injection of data/content-related messages, processing according to
 *          data processing logic and returning results via output parameters.
 * 
 * @param engine Pointer to the BackendEngine instance
 * @return int Return code indicating processing result:
 *         - BACKEND_PROXY_PROCESS_OK: Message injected and processed successfully
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed to inject or process the message
 */
int data_msg_inject(BackendEngine *engine){
    GeneralProxyMsgHeader *data_msg_hdr;
    uint8_t **res_string;
    char *desc_string;
    uint8_t               *res_buf[100] = {NULL};
    char                  desc_buf[100] = {0};
    char data_buf[100];
    int ret,  desc_len = 100;



    memset(data_buf, 0, sizeof(data_buf));
    snprintf(data_buf, sizeof(data_buf), "test msg");

    utils_print("strlen(test msg) = %d\n", strlen("test msg"));
    utils_print("content of data_buf = %s\n", data_buf);

    data_msg_hdr         = &proxy_data_msg_hdr;
    res_string           = res_buf;
    desc_string          = desc_buf;

    data_msg_hdr->outer_header.payload_len = strlen("test msg");

    utils_print("outer_header.payload_len = %d\n", data_msg_hdr->outer_header.payload_len);

    utils_print("In %s, version = %d, type = %d\n", __func__, data_msg_hdr->outer_header.version, data_msg_hdr->outer_header.proxy_msg_type);

    DUMP_BUFFER_CONTENT(data_buf, 8, "%c");

    ret = scenario_msg_inject(engine, data_msg_hdr, (const uint8_t *)data_buf, strlen(data_buf), MEMORY_ALLOC_SHARED, res_string, desc_string, desc_len);

    return ret;
}



/**
 * @brief Upper-layer scenario-based test function for the backend protocol stack, which uniformly constructs multi-type proxy messages and injects them into the shared memory RX queue
 * @details Designed based on the core responsibilities of "Scenario Functions", serving as a standardized entry for unit testing:
 *          1. Automatically constructs all core proxy message types, including device messages, strategy messages, session messages, and data messages;
 *          2. Calls the underlying packet-building functions (e.g., scenario_msg_inject build_proxy_general_message, build_proxy_sess_message) to complete the structured packaging of messages;
 *          3. Obtains the shared memory RX queue handle from the input BackendEngine global context (which must be initialized in advance), and injects the constructed messages into the queue following FIFO rules;
 *          4. Does not require external input of message parameters (e.g., msg_header, msg_payload). All test message parameters (such as device ID, session ID, data payload) are preset according to the test scenarios in the document 
 *             (e.g., frontend_sess_id = 1 preset for session messages, "query strategy" command preset for strategy messages) to ensure test consistency.
 * 
 * @param[in] engine Pointer to the BackendEngine global context, which must meet the requirements in the document:
 *                   - Must be successfully initialized via the engine_init function (returning BACKEND_PROXY_PROCESS_OK) to ensure that resources such as the memory pool handle and shared memory RX queue (engine->rx_queue) are ready 
 *                   - The context must contain valid runtime configurations (e.g., shared memory queue size, session pool capacity) to avoid resource unavailability errors during message construction or injection 
 *                     (refer to the parameter verification logic in "Section 1.2 Abnormal Scenarios" of the document).
 * 
 * @return int Follows the unified error code specification in the document, with return value meanings as follows:
 *             - BACKEND_PROXY_PROCESS_OK: All types of proxy messages are successfully constructed and injected into the RX queue;
 *             - BACKEND_PROXY_PROCESS_ERROR: All abnormal scenarios are covered, including uninitialized BackendEngine, NULL internal resources (e.g., rx_queue), full shared memory RX queue, and failed calls to underlying packet-building functions 
 *              
 * 
 * @note 1. Precondition: The engine must be initialized before calling this function (refer to the normal scenario process in "Test Process - Engine Initialization Test" of "Backend Protocol Stack Unit Test.doc"), otherwise a process error will be returned directly;
 *       2. Message coverage: The current version covers 4 core message types. Protocol Message Processing Module" of the document:
 *          - Device messages: Preset with "device status query" commands;
 *          - Strategy messages: Preset with "query strategy configuration" commands (corresponding to the "strategy configuration command execution" scenario;
 *          - Session messages: Preset with "create session" (valid device ID) and "close session" (valid session ID) commands (corresponding to the "session creation" scenario;
 *          - Data messages: Preset with two boundary scenarios: empty payload and maximum-length payload (4088 bytes);
 *       3. Result feedback: The function internally uses error_print to print detailed logs of message construction/injection (e.g., "Device message injected successfully", "RX queue full, data message injection failed"), 
 *          and the log format complies with the log specification for "configuration file loading failure";
 *       4. Subsequent triggering: After message injection is completed, the engine_run function (entry of "Main Loop Test Process" in the document) must be called to trigger the engine to read and process messages from the RX queue, thus completing the full test link.
 */
int test_proxy_scenario_multi_type_msg_build(BackendEngine *engine){
    device_msg_inject(engine);
    strategy_msg_inject(engine);
    session_msg_inject(engine);
    data_msg_inject(engine);

    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Upper-layer scenario-based test function for the backend protocol stack, which uniformly reads multi-type proxy messages from the shared memory RX queue 
 * and simulates the data reading process.
 * @details Designed based on the core responsibilities of "Scenario Functions", serving as a standardized entry for unit testing:
 * Automatically reads all core proxy message types that may exist in the queue, including device messages, strategy messages, session messages, and data messages 
 * (the actual types and quantities are uncertain, depending on the injected content);
 * Calls the underlying message reading functions (e.g., backend_engine_rx_queue_get, scenario_msg_read) to complete the parsing and extraction of structured messages;
 * Obtains the shared memory RX queue handle from the input BackendEngine global context (which must be initialized in advance), and reads messages from the queue following FIFO rules;
 * Does not require external input of message parameters (e.g., expected msg type, msg ID). All test verification logic (such as checking message structure validity, matching preset parameters) is 
 * based on the test scenarios (e.g., verifying that session messages contain preset session ID = 1, data messages match preset payload length) to ensure test consistency.
 * @param[in] engine Pointer to the BackendEngine global context, which must meet the requirements in the document:
 * Must be successfully initialized via the engine_init function (returning BACKEND_PROXY_PROCESS_OK) to ensure that resources such as the memory pool handle and 
 * shared memory RX queue (engine->rx_queue) are ready
 * The context must contain valid runtime configurations (e.g., shared memory queue size, message parsing rules) to avoid resource unavailability errors during message reading or parsing
 * (refer to the parameter verification logic in "Section 1.2 Abnormal Scenarios" of the document).
 * @return int Follows the unified error code specification in the document, with return value meanings as follows:
 * BACKEND_PROXY_PROCESS_OK: All existing types of proxy messages in the RX queue are successfully read and parsed;
 * BACKEND_PROXY_PROCESS_ERROR: All abnormal scenarios are covered, including uninitialized BackendEngine, NULL internal resources (e.g., rx_queue), empty shared memory RX queue (when messages 
 * are expected), and failed calls to underlying message reading/parsing functions
 * @note 1. Precondition: The engine must be initialized before calling this function, and messages must be pre-injected into the RX queue (e.g., via test_proxy_scenario_multi_type_msg_build) (refer to the normal scenario process in "Test Process - Message Reading Test" of "Backend Protocol Stack Unit Test.doc"), otherwise a process error will be returned directly;
 * Message coverage: The current version supports reading 4 core message types, which may exist in any combination (one or more types) depending on the injection scenario. For details on message 
 * structure, refer to "Section 3.1 Proxy Message Format" of the document:
 * Device messages: Verify consistency with preset "device status query" commands;
 * Strategy messages: Verify consistency with preset "query strategy configuration" commands (matching the "strategy configuration command parsing" scenario);
 * Session messages: Verify consistency with preset "create session" (valid device ID) and "close session" (valid session ID) commands (matching the "session command parsing" scenario);
 * Data messages: Verify consistency with preset boundary scenarios (empty payload and maximum-length payload (4088 bytes));
 * Result feedback: The function internally uses error_print to print detailed logs of message reading/parsing (e.g., "Device message read successfully", "Data message parsing failed: 
 * invalid payload length"),
 * and the log format complies with the log specification for "message processing failure";
 * Subsequent verification: After message reading is completed, the function may trigger internal verification logic (e.g., checking message count, comparing with injected content) to confirm 
 * the correctness of the reading process, thus completing the full test link of "inject-read-verify".
 */

int test_proxy_scenario_msg_read_from_rx_queue(BackendEngine *engine){
    struct SharedMemoryPoolQueue    *rx_queue;
//    struct BackendSessionQueue      *active_queue_f2b, *active_queue_b2f;
//    struct BackendSession           *cur_sess, *next_sess;
//    struct BackendSessionPool       *sess_pool;
//    struct BackendSessionPoolOps    *sess_pool_ops;
    uint8_t                         *proxy_msg;
    ProxyMsgHeader                  *proxy_msg_hdr;
    size_t                          msg_size;
    int                             ret;

    rx_queue = engine->rx_queue;

    if(NULL == rx_queue){
        error_print("test_proxy_scenario_msg_read_from_rx_queue failed: the rx_queue of the engine is not initialized!\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    utils_print("In %s, before enter backend_engine_rx_queue_get, proxy_msg = %p\n", __func__, proxy_msg);
    ret = backend_engine_rx_queue_get(rx_queue, (void **)&proxy_msg, PROXY_MSG_HDR_PLUS_MAX_SIZE, &msg_size);
    utils_print("In %s, after enter backend_engine_rx_queue_get\n", __func__);
    utils_print("rx queue header is %d, tail is %d, proxy_msg = %p\n", rx_queue->header, rx_queue->tail, proxy_msg);

    backend_proxy_msg_process(proxy_msg);

    ret = backend_engine_rx_queue_get(rx_queue, (void **)&proxy_msg, PROXY_MSG_HDR_PLUS_MAX_SIZE, &msg_size);
    utils_print("ret = %d, rx queue header is %d, tail is %d\n", ret, rx_queue->header, rx_queue->tail);

    backend_proxy_msg_process(proxy_msg);

    ret = backend_engine_rx_queue_get(rx_queue, (void **)&proxy_msg, PROXY_MSG_HDR_PLUS_MAX_SIZE, &msg_size);
    utils_print("ret = %d, rx queue header is %d, tail is %d\n", ret, rx_queue->header, rx_queue->tail);

    backend_proxy_msg_process(proxy_msg);

    ret = backend_engine_rx_queue_get(rx_queue, (void **)&proxy_msg, PROXY_MSG_HDR_PLUS_MAX_SIZE, &msg_size);
    utils_print("ret = %d, rx queue header is %d, tail is %d\n", ret, rx_queue->header, rx_queue->tail);

    proxy_msg_hdr = (ProxyMsgHeader *)proxy_msg;
    (void)proxy_msg_hdr;
    utils_print("in data message, version = %d, message type = %d\n", proxy_msg_hdr->version, proxy_msg_hdr->proxy_msg_type);
    backend_proxy_msg_process(proxy_msg);

    return ret;
}


/**
 * @brief Process the active Frontend-to-Backend session queue in the test proxy scenario
 * @details This function is designed to be executed after test_proxy_scenario_msg_read_from_rx_queue.
 * It accesses all active Frontend-to-Backend (f2b) sessions managed by the backend engine,
 * retrieves information from each session in sequence, and sends the information through
 * the socket maintained by the corresponding session object.
 *
 * @param[in] engine Pointer to a BackendEngine structure that manages the active f2b sessions and related resources
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful processing of all active sessions; 
 *                     BACKEND_PROXY_PROCESS_ERROR if any error occurs during processing
 */
int test_proxy_scenario_process_active_f2b_sess_queue(BackendEngine *engine){
//    struct SharedMemoryPoolQueue    *rx_queue, *tx_queue;
    struct BackendSessionQueue      *active_queue_f2b;
    struct BackendSession           *cur_sess, *next_sess;
    struct BackendSessionPool       *sess_pool;
    struct BackendSessionPoolOps    *sess_pool_ops;
//    uint8_t                         *proxy_msg;
//    size_t                          msg_size;
    int                             ret;

    utils_print("In %s, the address of the engine is %p\n", __func__, engine);

    BACKEND_ENGINE_GET_F2B_QUEUE(engine, active_queue_f2b);

#if 0
    sess_pool = engine->sess_pool;
    utils_print("The address of the session pool is %p\n", sess_pool);
    active_queue_f2b = &engine->sess_pool->queue_f2b;
    utils_print("The address of the active_queue_f2b is %p\n", active_queue_f2b);
#endif

    sess_pool       = engine->sess_pool;
    sess_pool_ops   = sess_pool->ops;

#if 1
    utils_print("In %s\n", __func__);
    TAILQ_FOREACH_SAFE(cur_sess, active_queue_f2b, entries_f2b, next_sess){
        utils_print("current sess frontend_sess_id = %d, backend_sess_id = %d\n", cur_sess->frontend_sess_id, cur_sess->backend_sess_id);

        ret = sess_pool_ops->data_process_f2b(cur_sess);

        utils_print("After call data_process_b2f, ret = %d\n", ret);

/*
 * If data_process_f2b returns BACKEND_PROXY_PROCESS_OK, this indicates all message segments in the front-to-back (F2B) message queue have been sent via the session's socket. 
 * Such sessions should be detached from the F2B active queue, and their "linked to queue" state flag should be cleared.
 */
        if(BACKEND_PROXY_PROCESS_OK == ret){
            TAILQ_REMOVE(active_queue_f2b, cur_sess, entries_f2b);
            cur_sess->state_f2b &= ~BACKEND_SESS_LINKED_TO_QUEUE;
        }
/*
 * If data_process_f2b returns BACKEND_PROXY_PROCESS_ERROR, it means an error occurs when trying to send data via the socket of the session. This type of session should not only be 
 * detached from the front-to-end active queue, but also be removed from the session pool.
 */
        if(BACKEND_PROXY_PROCESS_ERROR == ret){
            TAILQ_REMOVE(active_queue_f2b, cur_sess, entries_f2b);
            sess_pool_ops->delete_sess(sess_pool, cur_sess);
        }
 /*
  * Nothing to do when not all data has been sent and there are no errors.
  */

    }// TAILQ_FOREACH_SAFE
#endif

#if 0
    for ( (cur_sess) = (active_queue_f2b)->tqh_first, (next_sess) = (cur_sess)->entries_f2b.tqe_next; (cur_sess) != NULL; 
          (cur_sess) = (next_sess), (next_sess) = (cur_sess)->entries_f2b.tqe_next
    ) {
        utils_print("The address of the cur_sess = %p, next_sess = %p\n", cur_sess, next_sess);
        utils_print("current sess frontend_sess_id = %d, backend_sess_id = %d\n", cur_sess->frontend_sess_id, cur_sess->backend_sess_id);
    }
#endif


    return ret;
}

/**
 * @brief Upper-layer scenario-based test function for the backend protocol stack, which reads pure data from sockets monitored by the epoll instance (managed by NetPoller) via the epoll interface,
 * adds proxy data message headers to the data, sends it to the proxy frontend through the shared memory TX queue, and handles abnormal events (socket closure/error) by sending session closure commands.
 * @details Designed based on the core responsibilities of "Scenario Functions", serving as a standardized entry for unit testing:
 * 1. Event-driven data reading: Monitors sockets through the epoll instance maintained by the NetPoller member (engine->poller) of BackendEngine, waits for readable events via epoll interface,
 *    and reads pure binary data from ready sockets (no proxy message headers included initially);
 * 2. Data message encapsulation: After successfully reading pure data, automatically adds the standard proxy data message header (including message type, length, etc.) to the data,
 *    encapsulates it into a structured proxy data message that conforms to the protocol specification;
 * 3. Message forwarding: Obtains the shared memory TX queue handle (engine->tx_queue) from the BackendEngine context, and sends the encapsulated proxy data message to the proxy frontend following FIFO rules;
 * 4. Abnormal event handling: If epoll detects abnormal events (e.g., EPOLLRDHUP (socket closure), EPOLLERR (socket error)) on monitored sockets, automatically constructs a standard proxy session closure command,
 *    and sends it to the proxy frontend through the shared memory TX queue to ensure session state consistency;
 * 5. Supported message types: Focuses on proxy data messages (encapsulated from pure socket data) and session closure commands (triggered by abnormal events), with the data message content depending on the pure data sent to the sockets;
 * 6. Underlying function dependencies: Calls NetPoller's epoll operation functions , pure data reading functions,
 *    proxy data message header encapsulation functions , shared memory TX queue sending functions ,
 *    and session closure command construction functions to complete the full process;
 * 7. Resource acquisition: Obtains core resources from BackendEngine context, including NetPoller instance (engine->poller, containing epoll file descriptor epfd and monitored socket set),
 *    shared memory TX queue (engine->tx_queue), and proxy message header configuration (e.g., fixed message type for data messages), following epoll event-driven and queue sending rules.
 * @param[in] engine Pointer to the BackendEngine global context, which must meet the requirements in the document:
 * 1. Must be successfully initialized via the engine_init function (returning BACKEND_PROXY_PROCESS_OK) to ensure that core resources are ready;
 * 2. The context must contain valid configuration parameters:
 *    - Epoll configuration (via engine->poller): Wait timeout, maximum number of events per wait, socket monitoring rules;
 *    - Proxy message configuration: Data message header format (e.g., header length, field order), session closure command code and structure;
 *    - Avoid resource unavailability or configuration mismatch errors during epoll operation, data reading, message encapsulation, or queue sending.
 */
void test_proxy_scenario_msg_read_from_poller(BackendEngine *engine){
    NetPoller       *net_poller;
    net_poller      = &engine->poller;

    poller_run(engine, net_poller);
}


/**
 * @brief Process the active Backend-to-Frontend session queue in the test proxy scenario
 * @details This function is designed for processing Backend-to-Frontend (b2f) traffic.
 * It accesses all active b2f sessions managed by the backend engine,
 * calls the sess_pool_ops->data_process_b2f function in sequence for each session,
 * and writes the packets in the session's b2f message queue to the shared memory TX queue.
 *
 * @param[in] engine Pointer to a BackendEngine structure that manages the active b2f sessions and related resources (including b2f message queues, shared memory TX queue, etc.)
 * @return int Returns BACKEND_PROXY_PROCESS_OK on successful processing of all active sessions; 
 *                     BACKEND_PROXY_PROCESS_ERROR if any error occurs during processing
 */
int test_proxy_scenario_process_active_b2f_sess_queue(BackendEngine *engine){
//    struct SharedMemoryPoolQueue    *rx_queue, *tx_queue;
    struct BackendSessionQueue      *active_queue_b2f;
    struct BackendSession           *cur_sess, *next_sess;
    struct BackendSessionPool       *sess_pool;
    struct BackendSessionPoolOps    *sess_pool_ops;
//    uint8_t                         *proxy_msg;
//    size_t                          msg_size;
    int                             ret;

    utils_print("In %s, the address of the engine is %p\n", __func__, engine);

    BACKEND_ENGINE_GET_B2F_QUEUE(engine, active_queue_b2f);

    sess_pool       = engine->sess_pool;
    sess_pool_ops   = sess_pool->ops;

    utils_print("In %s\n", __func__);
    TAILQ_FOREACH_SAFE(cur_sess, active_queue_b2f, entries_b2f, next_sess){
        utils_print("current sess frontend_sess_id = %d, backend_sess_id = %d\n", cur_sess->frontend_sess_id, cur_sess->backend_sess_id);

        ret = sess_pool_ops->data_process_b2f(cur_sess);

        utils_print("After call data_process_b2f, ret = %d\n", ret);

/*
 * If data_process_b2f returns BACKEND_PROXY_PROCESS_OK, this indicates all message segments in the back-to-end (B2F) message queue have been sent via the shared memory TX queue. 
 * Such sessions should be detached from the B2F active queue, and their "linked to queue" state flag should be cleared.
 */
        if(BACKEND_PROXY_PROCESS_OK == ret){
            TAILQ_REMOVE(active_queue_b2f, cur_sess, entries_b2f);
            cur_sess->state_b2f &= ~BACKEND_SESS_LINKED_TO_QUEUE;
        }
    }// TAILQ_FOREACH_SAFE

/*
 * If data_process_f2b returns BACKEND_PROXY_PROCESS_ERROR, it means an error occurs when trying to send data via the shared memory TX queue. This type of session should not only be 
 * detached from the front-to-end active queue, but also be removed from the session pool.
 */
        if(BACKEND_PROXY_PROCESS_ERROR == ret){
            TAILQ_REMOVE(active_queue_b2f, cur_sess, entries_b2f);
            sess_pool_ops->delete_sess(sess_pool, cur_sess);
        }
 /*
  * Nothing to do when not all data has been sent and there are no errors.
  */
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Inject a device message into the backend engine via the HyperAMP queue
 *
 * @details This function injects a device-related message into the BackendEngine instance, 
 *          typically by enqueuing it into the HyperAMP RX queue (i.e., the channel where the 
 *          backend consumes messages from the frontend or external sources). 
 *          The message is expected to be formatted according to the HyperAMP protocol and will be 
 *          processed by the backend engine during its next polling or event loop cycle.
 *          This function simulates or facilitates the arrival of a message from the HyperAMP 
 *          shared-memory interface into the backend's internal processing logic.
 *
 * @param engine Pointer to the BackendEngine instance that owns or manages the HyperAMP interface
 * @return int Return code indicating the result of the injection attempt:
 *         - BACKEND_PROXY_PROCESS_OK: Message successfully injected/enqueued into the BackendEngine
 *         - BACKEND_PROXY_PROCESS_ERROR: Failed to inject the message (e.g., queue full, 
 *                                        uninitialized HyperAMP context, invalid engine state, 
 *                                        or NULL pointer provided)
 *
 * @note This function does not wait for the backend to fully process the message logic; it only 
 *       ensures the message is placed in the target queue or handoff buffer within the BackendEngine. 
 *       Processing is asynchronous.
 *       The actual message content and layout must conform to the agreed-upon HyperAMP device 
 *       message schema between the source (e.g., seL4 frontend) and Linux (backend).
 */
int device_msg_inject_backend_hyperamp(BackendEngine *engine){
    GeneralProxyMsgHeader *dev_msg_hdr;
    DevMsgReport           dev_msg_resp;
    int                   ret, desc_len = 100;
    uint8_t               **res_string;
    char                  *desc_string;
    uint8_t               *res_buf[100] = {NULL};
    char                  desc_buf[100] = {0};

    dev_msg_hdr          = &dev_enable_msg_hdr;

    dev_msg_resp.status  = SESS_OP_STATUS_SUCCESS;
    dev_msg_resp.error   = SESS_OP_CODE_SUCCESS;
    dev_msg_resp.data    = 0xFF;


    res_string           = res_buf;
    desc_string          = desc_buf;

    utils_print("In %s, before enter scenario_msg_inject\n", __func__);

    ret = scenario_msg_inject(engine, dev_msg_hdr, (const uint8_t *)&dev_msg_resp, sizeof(dev_msg_resp), MEMORY_ALLOC_AMPQUEUE, res_string, desc_string, desc_len); 
    return ret;   
}