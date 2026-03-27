#ifndef SENARIO_TEST_H_
#define SENARIO_TEST_H_

#include "engine.h"
#include "backend_proto.h"
#include "message.h"
#include "shared_mem_io.h"
#include "session.h"

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

int test_proxy_scenario_process_active_f2b_sess_queue(BackendEngine *engine);

void test_proxy_scenario_process_read_from_poller(BackendEngine *engine);

int test_proxy_scenario_process_active_b2f_sess_queue(BackendEngine *engine);


int device_msg_inject_backend_hyperamp(BackendEngine *engine);


#endif