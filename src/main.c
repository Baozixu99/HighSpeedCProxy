
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

void run()
{
    while (1)
    {
        //todo, poll/push share memory
        engine_run();
    }
       
}

int main(int argc, char** argv)
{
    BackendEngine *eng; 

    engine_init();

    eng = get_global_backend_engine();

    test_proxy_scenario_multi_type_msg_build(eng);
    test_proxy_scenario_msg_read_from_rx_queue(eng);
    // run();
    return 0;
}
