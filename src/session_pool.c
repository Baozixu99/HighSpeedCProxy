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
#include "engine.h"


int __high_speed_create_sess_fastpath(struct BackendSessionPool *s_pool, struct BackendSession **sess, uint16_t new_sess_id, struct SessMsgPara *para);

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
    TAILQ_INIT(&pool->b2f_queue);

    pool->htable = NULL;
    pool->ops = &high_speed_pool_ops;
    pool->engine = get_global_backend_engine();
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
 * The procedure of creating a session can be divided into two steps:
 * STEP 1. Allocate resources, including session object, backend session ID, socket, etc.
 * STEP 2. Establish a session according to the parameters provided by the front end, and create a session message to inform the front-end proxy of the result of the 
 *         creation request.
 *
 * The main body of the session creation procedure lies in the function which the create_sess pointer points to.
 */
    struct BackendSession *new_sess = NULL;
    BackendEngine *engine;
    uint16_t frontend_sess_id, new_sess_id, dev_id;
    int fd = ERROR_SOCKET_FD, domain, type, protocol, ns_id, ret;
    SessOpRespData resp_dat;

    engine = s_pool->engine;
    if(NULL == engine || NULL == engine->ops || NULL == engine->ops->choose_dev){
        error_print("high_speed_create_sess fails: the session pool does not belong to any engine, or the engine is not initialized successfully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
/*
 * STEP 1.
 * (1) Allocate memory for storing the backend session object;
 * (2) Parse session message parameters, obtain the device ID, and execute the device selection procedure if necessary;
 * (3) Determine the namespace ID by parsing the device ID of the specified high-speed network device, and create a new socket based on the session message parameters.
 */
    engine              = s_pool->engine;
    frontend_sess_id    = para->frontend_sess_id;
    new_sess            = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    if(NULL == new_sess){
        error_print("high_speed_create_sess failed: failed to allocate memory for BackendSession!");
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }

    new_sess_id = allocate_id(&s_pool->id_queue);
    if(0 == new_sess_id){
        error_print("high_speed_create_sess failed: failed to allocating session ID!");
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }

    para->backend_sess_id = new_sess_id;

/*
 * Choose the namespace of the preferred high-speed network device for creating a socket.
 */
    dev_id = para->dev_id;


/*
 * If the device ID equals 0xFF, it means the backend engine should take responsibility for choosing the most appropriate high-speed network device on which the 
 * new session is established.
 */
    if(DEV_ID_AUTO_HANDOVER == dev_id){
        if(BACKEND_PROXY_PROCESS_OK != engine->ops->choose_dev(engine, &dev_id)){
            error_print("high_speed_create_sess failed: the network device selection procedure failed!");
            resp_dat.status = SESS_OP_STATUS_FAIL;
            resp_dat.code   = SESS_OP_CODE_DEVICE_ERROR;
            goto create_sess_error;
        }
    }

/*
 * Get the namespace ID to which the selected high-speed network device is set.
 */
    ns_id = GET_NS_ID(&engine->dev_set, dev_id);
    if(ERROR_NAMESPACE_ID == ns_id){
        error_print("high_speed_create_sess failed: failed to obtain the namespace ID that the selected high-speed network device belongs to!");
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_DEVICE_ERROR;
        goto create_sess_error;
    }

/*
 * Create a socket with given parameters.
 */
    if(BACKEND_PROXY_PROCESS_OK != create_socket_netns(ns_id, para, &fd)){
        error_print("high_speed_create_sess failed: failed to create a socket with the given parameters!");
        fd = ERROR_SOCKET_FD;
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }

/*
 * STEP 2:
 * (1). Connect to the specified IP:Port tuple;
 * (2). Initialize members of the new session object;
 * (3). Create a RESPONSE message to notify the frontend proxy that the specified session is created successfully;
 * (4). Insert the session into the specified session pool.
 */

/*
 * Connect to the specified IP:Port tuple.
 */
    if(BACKEND_PROXY_PROCESS_OK != connect_socket_netns(fd, para)){
        error_print("high_speed_create_sess failed: failed to connect to the specified IP:Port!");
/*
 * The backend proxy should generate and send a create-response message to notify the frontend proxy that the create-command has failed.
 * We have not developed the failed-reason function; it is reserved for future development.
 */
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.status = SESS_OP_CODE_NETWORK_UNREACHABLE;

        goto create_sess_error;
    }

/*
 * Initialize session object.
 */    
    new_sess->frontend_sess_id  = para->frontend_sess_id;
    new_sess->backend_sess_id   = new_sess_id;
    new_sess->ip_version        = para->ip_version;
    new_sess->sock_fd           = fd;
    new_sess->eng               = engine;
    TAILQ_INIT(&new_sess->queue_f2b);
    TAILQ_INIT(&new_sess->queue_b2a);

/*
 * Generate a session create-response message, deliver it to the shared queue. The front-end will receive this message and complete the handshake procedure.
 */

    *sess = new_sess;

    resp_dat.status = SESS_OP_STATUS_SUCCESS;
    resp_dat.code   = SESS_OP_CODE_SUCCESS;

    ret = backend_proxy_send_sess_msg_to_frontend_via_shmem(new_sess, SESS_MSG_CREATE, &resp_dat);
    
    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("high_speed_create_sess failed: failed to push the session creat-response message into the tx queue!");
/*
 * The backend proxy should generate and send a create-response message to notify the frontend proxy that the create-command has failed.
 * We want the backend proxy protocol to try again by sending a message to notify that the establishment procedure has failed.
 */
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.status = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }

    return BACKEND_PROXY_PROCESS_OK;

create_sess_error:
/*
 * Response a message to notify the front-end that the session creation procedure failed.
 */
    backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem(engine, frontend_sess_id, BACKEND_HANDOVER_SESSION_ID, para->ip_version, SESS_MSG_CREATE, &resp_dat);

/*
 * Reclaim resources.
 */
    if(NULL != new_sess){
        free(new_sess);
    }

    if(0 != new_sess_id){
        release_id(&s_pool->id_queue, new_sess_id);
    }

    if(ERROR_SOCKET_FD != fd){
        close(fd);
    }

/*
 * Now connect to the remote IP:Port.
 *
 * PLEASE NOTICE
 *
 * The datagram socket can also "connect" to the specified IP:Port. However, the behavior is quite different from that of a stream socket when connecting to the remote 
 * side. We choose to call connect on a datagram socket in order to unify the procedure of the backend proxy network stack. The details are processed by the Linux kernel
 * network stack.
 */

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
/*
 * The process of deleting a session can be divided into two steps:
 * STEP 1. Release resources, including session objects, backend session IDs, sockets, etc.
 * STEP 2. Create a session message to inform the front-end proxy of the result of the 
 *         session closure request.
 */
    uint16_t frontend_sess_id, backend_sess_id;
    SessOpRespData resp_dat;


    if(NULL == s_pool || NULL == sess){
        error_print("high_speed_delete_sess failed: the session pool or the sess pointer is NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * STEP 1.
 * (1) Close the socket bound to the backend session object;
 * (2) Delete the session instance from the hash table, and detach it from the double linked lists;
 * (3) Release resources occupied by the session instance.
 */

/*
 * STEP 1(1).
 */
    close(sess->sock_fd);

    frontend_sess_id    = sess->frontend_sess_id;
    backend_sess_id     = sess->backend_sess_id;
/*
 * STEP 1(2).
 */
    HASH_DEL(s_pool->htable, sess);

    dec_sess_num(s_pool);

/*
 * STEP 1(3).
 */
    free(sess);
    return BACKEND_PROXY_PROCESS_OK;
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


int __high_speed_create_sess_fastpath(struct BackendSessionPool *s_pool, struct BackendSession **sess, uint16_t new_sess_id, struct SessMsgPara *para){
/*
 * To do.
 */
    return BACKEND_PROXY_PROCESS_OK;
}

