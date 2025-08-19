#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "session_pool.h"
#include "message.h"
#include "netns_socket.h"


//ops
int high_speed_create_sess(struct BackendSessionPool *s_pool, struct BackendSession **sess, struct SessMsgPara *para);
int high_speed_insert_sess(struct BackendSessionPool* s_pool, struct BackendSession *sess);
struct BackendSession* high_speed_search_sess(struct BackendSessionPool *s_pool, uint16_t id);
int high_speed_delete_sess(struct BackendSessionPool *s_pool, struct BackendSession *sess);
int high_speed_data_process(struct BackendSession *sess, uint8_t *in, 
        uint32_t in_size, uint8_t *out, uint32_t *out_size);
void high_speed_destroy_pool(struct BackendSessionPool *s_pool);


struct BackendSessionPoolOps high_speed_pool_ops = {
    .create_sess = high_speed_create_sess,
    .insert_sess = high_speed_insert_sess,
    .search_sess = high_speed_search_sess,
    .delete_sess = high_speed_delete_sess,
    .data_process = high_speed_data_process,
    .destroy_pool = high_speed_destroy_pool
};

struct BackendSessionPool *get_backend_high_speed_pool(){
    return high_speed_pool;
}

//helper func
void fill_id_queue(struct BackendSessionIDQueue *id_q)
{
    uint16_t q_num = 1024;
    for (uint16_t i = 1; i <= q_num; i++)
    {
        struct BackendSessionID* id_e = (struct BackendSessionID*)malloc(
            sizeof(struct BackendSessionID));
        if (!id_e) {
            printf("Memory allocate failed!\n");
            exit(1);
        }
        id_e->id = i;
        TAILQ_INSERT_TAIL(id_q, id_e, entry);
        // printf("push %p %d\n", id_e, id_e->id);
    }
}

uint16_t allocate_id(struct BackendSessionIDQueue *id_q)
{
    uint16_t res = 0;

    if(!TAILQ_EMPTY(id_q))
    {   
        struct BackendSessionID *id_e = TAILQ_FIRST(id_q);
        res = id_e->id;
        // printf("pop %p %d\n", id_e, res);
        TAILQ_REMOVE(id_q, id_e, entry);
        free(id_e);
    } 
    return res;
}

void release_id(struct BackendSessionIDQueue *id_q, uint16_t id)
{   
    struct BackendSessionID* id_e = (struct BackendSessionID*)malloc(
            sizeof(struct BackendSessionID));
    if (!id_e) {
        printf("Memory allocate failed!\n");
        exit(1);
    }
    id_e->id = id;
    TAILQ_INSERT_TAIL(id_q, id_e, entry);
    // printf("push %p %d\n", id_e, id_e->id);
}

int high_speed_init_pool(struct BackendSessionPool *pool)
{
    pool->pool_name = "high_speed_pool";
    pool->capacity = 1024;
    pool->sess_num = 0;

    TAILQ_INIT(&pool->id_queue);
    fill_id_queue(&pool->id_queue);

    TAILQ_INIT(&pool->act_queue);

    pool->htable = NULL;
    pool->ops = &high_speed_pool_ops;
    return 0;
}

void inc_sess_num(struct BackendSessionPool *pool)
{
    pool->sess_num++;
}

void dec_sess_num(struct BackendSessionPool *pool)
{
    pool->sess_num--;
}

void print_pool(struct BackendSessionPool *s_pool) {
    struct BackendSession *s;

    for (s = s_pool->htable; s != NULL; s = s->hh.next) {
        printf("sess id %d\n", s->backend_sess_id);
    }
}

void high_speed_delete_all_sess(struct BackendSessionPool *s_pool)
{
    struct BackendSession *current_sess, *tmp;

    HASH_ITER(hh, s_pool->htable, current_sess, tmp) {
        HASH_DEL(s_pool->htable, current_sess);  /* delete it */
        free(current_sess);                      /* free it */
    }
}

//ops
 int high_speed_create_sess(struct BackendSessionPool *s_pool, struct BackendSession **sess, struct SessMsgPara *para){
/*
 * The procedure of creating a session can be divided into three steps:
 * STEP 1. Allocate resources, including session object, backend session ID, socket, etc.
 * STEP 2. Establish a session according to the parameters provided by the front end.
 * STEP 3. Create a session message to inform the front-end proxy of the result of the creation request.
 *
 * The main body of the session creation procedure lies in the function which the create_sess pointer points to.
 */
    struct BackendSession *new_sess = NULL;
    uint16_t new_sess_id;
    int fd, type, protocol;
/*
 * STEP 1.
 */
    sess = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    if(NULL == sess){
        error_print("high_speed_create_sess returns an error because allocating memory for BackendSession failed!");
        goto create_sess_error;
    }

    new_sess_id = allocate_id(&s_pool->id_queue);
    if(0 == new_sess_id){
        error_print("high_speed_create_sess returns an error because allocating session ID failed!");
        goto create_sess_error;
    }

    if(SESS_TCP_PROTO == para->trans_proto){

    }else if (SESS_UDP_PROTO == para->trans_proto){

    }else if (SESS_FASTPATH_PROTO == para->trans_proto){

    }else{
        
    }

    return BACKEND_PROXY_PROCESS_OK;

create_sess_error:
    if(NULL != sess){
        free(sess);
    }

    return BACKEND_PROXY_PROCESS_ERROR;
 }


int high_speed_insert_sess(struct BackendSessionPool *s_pool, struct BackendSession *sess)
{
    struct BackendSession *s;
    HASH_FIND(hh, s_pool->htable, &sess->backend_sess_id, sizeof(uint16_t), s);
    if(s == NULL)
    {
        HASH_ADD(hh, s_pool->htable, backend_sess_id, sizeof(uint16_t), sess);
        inc_sess_num(s_pool);
        printf("add %d\n", sess->backend_sess_id);
    }else{
        goto insert_error;
    }
    return BACKEND_PROXY_PROCESS_OK;
insert_error:
    return BACKEND_PROXY_PROCESS_ERROR;
}

struct BackendSession *high_speed_search_sess(struct BackendSessionPool *s_pool, uint16_t id)
{
    struct BackendSession* s = NULL;
    HASH_FIND(hh, s_pool->htable, &id, sizeof(uint16_t), s);
    return s;
}

int high_speed_delete_sess(struct BackendSessionPool *s_pool, struct BackendSession *sess)
{
    HASH_DEL(s_pool->htable, sess);
    dec_sess_num(s_pool);
    free(sess);
    return 0;
}

int high_speed_data_process(struct BackendSession *sess, uint8_t *in, 
        uint32_t in_size, uint8_t *out, uint32_t *out_size)
{
    return 0;
}

void high_speed_destroy_pool(struct BackendSessionPool *s_pool)
{
    s_pool->pool_name = NULL;
    s_pool->capacity = 0;
    s_pool->sess_num = 0;
    s_pool->ops = NULL;
    high_speed_delete_all_sess(s_pool);

    //todo, clear queue
}


