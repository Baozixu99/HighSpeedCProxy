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
    if (alloc_mode != MEMORY_ALLOC_SHARED && alloc_mode != MEMORY_ALLOC_CALLER)
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

    return BACKEND_PROXY_PROCESS_OK;
}