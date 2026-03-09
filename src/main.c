
#include <stdio.h>

#include "netns_socket.h"
#include "dev.h"
#include "session.h"
#include "session_pool.h"
#include "channel.h"
#include "message.h"
#include "engine.h"
#include "poller.h"
#include "iniparser.h"

#include "senario_test.h"

int main(int argc, char** argv)
{
    BackendEngine   *eng;
    ProxyMsgHeader  *proxy_msg_hdr;
    int             ret, proto_msg_len, cnt = 0; 

    engine_init();
    
    sleep(2);

    eng = get_global_backend_engine();


#if 0

#if 0
    test_proxy_scenario_multi_type_msg_build(eng);
    test_proxy_scenario_msg_read_from_rx_queue(eng);
    test_proxy_scenario_process_active_f2b_sess_queue(eng);

    test_proxy_scenario_msg_read_from_poller(eng);
    test_proxy_scenario_process_active_b2f_sess_queue(eng);
#endif
//    engine_run();



    while(1){
        uint8_t msg_buf[HYPERAMP_MSG_HDR_PLUS_MAX_SIZE];
        size_t actual_len = 0;
        ret = backend_engine_hyperamp_rx_queue_get(eng, HYPERAMP_MSG_HDR_PLUS_MAX_SIZE, msg_buf, &actual_len);


        if(BACKEND_PROXY_PROCESS_OK == ret){
            utils_print("In loop after backend_engine_hyperamp_rx_queue_get, ret = %d, actual_len = %u\n", ret, actual_len);
            proxy_msg_hdr = (ProxyMsgHeader *)msg_buf;
            utils_print("version = %d, msg type = %d, frontend ID = %d, backend ID = %d, msg len = %d\n", 
                         proxy_msg_hdr->version, proxy_msg_hdr->proxy_msg_type, proxy_msg_hdr->frontend_sess_id, proxy_msg_hdr->backend_sess_id, proxy_msg_hdr->payload_len);
            
            backend_proxy_msg_process(msg_buf);
        }else if(BACKEND_PROXY_PROCESS_AGAIN == ret){
            utils_print("In backend, HyperAMP RX queue is empty!\n");
        }else{
            utils_print("Failed to get message in HyperAMP RX queue!\n");
        }
        
        sleep(2);

        cnt++;
        if(cnt > 50){
            break;
        }
    }
#endif

    engine_run_hyperamp();
    
    engine_destory();
    
    return 0;
}
