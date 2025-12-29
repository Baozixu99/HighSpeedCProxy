
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
    BackendEngine *eng; 

    engine_init();

    eng = get_global_backend_engine();

    test_proxy_scenario_multi_type_msg_build(eng);
    test_proxy_scenario_msg_read_from_rx_queue(eng);
    test_proxy_scenario_process_active_f2b_sess_queue(eng);

    test_proxy_scenario_msg_read_from_poller(eng);
    test_proxy_scenario_process_active_b2f_sess_queue(eng);
    
//    engine_run();

    engine_destory();
    
    return 0;
}
