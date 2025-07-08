
#include <stdio.h>

#include "netns_socket.h"
#include "dev.h"
#include "session.h"
#include "session_pool.h"
#include "channel.h"
#include "message.h"
#include "engine.h"
#include "poller.h"

extern void print_pool(struct BackendSessionPool* s_pool);
extern void high_speed_delete_all_sess(struct BackendSessionPool* s_pool);

void run(int epoll_fd)
{
    while (1)
    {
        //todo, poll/push share memory
        engine_run();

        poller_run(epoll_fd);
    }
       
}

int main(int argc, char** argv)
{
    high_speed_pool = (struct BackendSessionPool*)malloc(sizeof(struct BackendSessionPool));
    high_speed_init_pool(high_speed_pool);

    struct BackendSession* s1 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s1->backend_sess_id = 1;
    high_speed_pool->ops->insert_sess(high_speed_pool, s1);
    print_pool(high_speed_pool);

    struct BackendSession* s2 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s2->backend_sess_id = 2;
    
    struct BackendSession* s3 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s3->backend_sess_id = 3;

    struct BackendSession* s4 = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    s4->backend_sess_id = 4;

    high_speed_pool->ops->insert_sess(high_speed_pool, s1);
    high_speed_pool->ops->insert_sess(high_speed_pool, s2);
    high_speed_pool->ops->insert_sess(high_speed_pool, s3);
    high_speed_pool->ops->insert_sess(high_speed_pool, s4);

    print_pool(high_speed_pool);
    // high_speed_delete_all_sess(high_speed_pool);
   
    struct BackendSession* s5 = high_speed_pool->ops->search_sess(high_speed_pool, (uint16_t)3);
    if (s5) printf("%d\n", s5->backend_sess_id);

    high_speed_pool->ops->delete_sess(high_speed_pool, s3);

    struct BackendSession* s6 = high_speed_pool->ops->search_sess(high_speed_pool, (uint16_t)4);
    if (s6) printf("%d\n", s6->backend_sess_id);

    print_pool(high_speed_pool);
    printf("hello!\n");

    // engine_init();
    // run();
    return 0;
}