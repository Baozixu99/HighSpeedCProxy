#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include "session_pool.h"
#include "message.h"
#include "netns_socket.h"
#include "engine.h"
#include "shared_mem_io.h"
#include "common_utils.h"


int __high_speed_create_sess_fastpath(struct BackendSessionPool *s_pool, struct BackendSession **sess, uint16_t new_sess_id, struct SessMsgPara *para);

//ops
int high_speed_create_sess(struct BackendSessionPool *s_pool, struct BackendSession **sess, struct SessMsgPara *para);
int high_speed_create_sess_active(struct BackendSessionPool *s_pool, struct BackendSession **sess, struct SessMsgPara *para);
int high_speed_create_sess_passive(struct BackendSessionPool *s_pool, struct BackendSession **sess, PassiveSessParaIP *para);
int high_speed_insert_sess(struct BackendSessionPool* s_pool, struct BackendSession *sess);
struct BackendSession* high_speed_search_sess(struct BackendSessionPool *s_pool, uint16_t id);
int high_speed_delete_sess(struct BackendSessionPool *s_pool, struct BackendSession *sess);
void high_speed_destroy_pool(struct BackendSessionPool *s_pool);


struct BackendSessionPoolOps high_speed_pool_ops = {
    .create_sess            = high_speed_create_sess,
    .create_sess_active     = high_speed_create_sess_active,
    .create_sess_passive    = high_speed_create_sess_passive,
    .insert_sess            = high_speed_insert_sess,
    .search_sess            = high_speed_search_sess,
    .delete_sess            = high_speed_delete_sess,
    .data_process_f2b       = high_speed_data_process_f2b,
    .data_process_b2f       = high_speed_data_process_b2f,
    .data_process_nns       = high_speed_data_process_nns, 
    .destroy_pool           = high_speed_destroy_pool
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
    struct BackendSessionID *id_e = (struct BackendSessionID*)malloc(
            sizeof(struct BackendSessionID));
    if (!id_e) {
        printf("Memory allocate failed!\n");
        exit(1);
    }
    id_e->id = id;
    TAILQ_INSERT_TAIL(id_q, id_e, entry);
    // printf("push %p %d\n", id_e, id_e->id);
}

void release_id_queue(struct BackendSessionIDQueue *id_q){
    struct BackendSessionID *id_tmp, *id_next;

    if(NULL == id_q){
        return;
    }

    TAILQ_FOREACH_SAFE(id_tmp, id_q, entry, id_next){
        TAILQ_REMOVE(id_q, id_tmp, entry);
        free(id_tmp);
    }
}

int high_speed_init_pool(struct BackendSessionPool *pool)
{
    pool->pool_name = "high_speed_pool";
    pool->capacity = 1024;
    pool->sess_num = 0;

    TAILQ_INIT(&pool->id_queue);
    fill_id_queue(&pool->id_queue);

    TAILQ_INIT(&pool->queue_f2b);
    TAILQ_INIT(&pool->queue_b2f);

    pool->htable = NULL;
    pool->ops = &high_speed_pool_ops;
    pool->engine = get_global_backend_engine();
    return 0;
}


void high_speed_deinit_pool(struct BackendSessionPool *pool){
    release_id_queue(&pool->id_queue);
    TAILQ_INIT(&pool->queue_f2b);
    TAILQ_INIT(&pool->queue_b2f);
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
    BackendEngine                   *engine;
    struct BackendSessionPoolOps    *sess_pool_ops;
    struct BackendSession           *new_sess = NULL;
    NetChannel              *net_channel;
    uint16_t frontend_sess_id, new_sess_id, dev_id;
    int fd = ERROR_SOCKET_FD, ns_id, ret;
    SessOpRespData resp_dat;

    engine          = s_pool->engine;
    sess_pool_ops   = s_pool->ops;

    utils_print("In %s\n", __func__);
    utils_print("The address of engine is %p\n", engine);
    utils_print("The address of engine ops is %p\n", engine->ops);
    utils_print("The address of session pool ops is %p\n", sess_pool_ops);
#if 0
    utils_print("In %s, the address of the engine is %p, ops is %p， hs_backend_eng_ops address is %p, and chooes_dev is %p\n", 
                __func__, engine, engine->ops, get_hs_backend_engine_ops(), engine->ops->choose_dev);
#endif

    if(NULL == engine || NULL == engine->ops || NULL == engine->ops->choose_dev){
        error_print("high_speed_create_sess fails: the session pool does not belong to any engine, or the engine is not initialized successfully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


    if(NULL == sess_pool_ops || NULL == sess_pool_ops->insert_sess){
        error_print("high_speed_create_sess fails: the session pool operation function set is not initialized correctly!");
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

    utils_print("The dev_id is %d, %s\n", dev_id, __func__);
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
    struct HighSpeedNetDevice *hs_dev;
    hs_dev = &(engine->dev_set->hs_net_dev[dev_id]);
    (void)hs_dev;
    ns_id = GET_NS_ID(engine->dev_set, dev_id);
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
 * (1). Connect to the specified IP:Port tuple using the newly created socket; if successful, add this socket to the epoll wait list;
 * (2). Initialize all members of the new session object;
 * (3). Create a RESPONSE message to notify the frontend proxy that the new session has been created successfully;
 * (4). Insert the session into the corresponding session pool.
 */

/*
 * Now connect to the remote IP:Port.
 *
 * PLEASE NOTICE
 *
 * The datagram socket can also "connect" to the specified IP:Port. However, the behavior is quite different from that of a stream socket when connecting to the remote 
 * side. We choose to call connect on a datagram socket in order to unify the procedure of the backend proxy network stack. The details are processed by the Linux kernel
 * network stack.
 */

    if(BACKEND_PROXY_PROCESS_OK != connect_socket_netns(fd, para)){
        error_print("high_speed_create_sess failed: failed to connect to the specified IP:Port!");
/*
 * The backend proxy should generate and send a create-response message to notify the frontend proxy that the create-command has failed.
 * We have not developed the failed-reason function; it is reserved for future development.
 */
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_NETWORK_UNREACHABLE;

        goto create_sess_error;
    }
/*
 * Initialize the network channel.
 */
    net_channel             = &new_sess->net_channel;
    net_channel->sock_fd    = fd;
    net_channel->arg        = &engine->poller;
    net_channel->sess       = new_sess;

/*
 * Initialize session object.
 */    
    new_sess->frontend_sess_id  = para->frontend_sess_id;
    new_sess->backend_sess_id   = new_sess_id;
    new_sess->ip_version        = para->ip_version;
    new_sess->sock_fd           = fd;
    new_sess->eng               = engine;
    new_sess->state_f2b         &= BACKEND_SESS_LINKED_TO_QUEUE;
    new_sess->state_b2f         &= BACKEND_SESS_LINKED_TO_QUEUE;
    TAILQ_INIT(&new_sess->msg_f2b);
    utils_print("In %s, TAILQ_EMPTY(&new_sess->msg_f2b) returns %d\n", __func__, TAILQ_EMPTY(&new_sess->msg_f2b));
    TAILQ_INIT(&new_sess->msg_b2f);
    utils_print("In %s, TAILQ_EMPTY(&new_sess->msg_b2f) returns %d\n", __func__, TAILQ_EMPTY(&new_sess->msg_b2f));

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
/*
 * Finally, register the socket belonging to the newly created session with the epoll instance.
 */

    BACKEND_SESS_REGISTER_EPOLL(new_sess, EPOLLIN, &ret);

    if(ret != BACKEND_PROXY_PROCESS_OK){
        error_print("high_speed_create_sess failed: failed to register the socket of the newly create session with the epoll instance!");
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.status = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }


    ret = sess_pool_ops->insert_sess(s_pool, new_sess);

    if(ret != BACKEND_PROXY_PROCESS_OK){
        error_print("high_speed_create_sess failed: failed to insert the session instance into the session pool!");
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

    return BACKEND_PROXY_PROCESS_ERROR;
 }


 int high_speed_create_sess_active(struct BackendSessionPool *s_pool, struct BackendSession **sess, struct SessMsgPara *para){
/*
 * The procedure of creating a session can be divided into two steps:
 * STEP 1. Allocate resources, including session object, backend session ID, socket, etc.
 * STEP 2. Establish a session according to the parameters provided by the front end, and create a session message to inform the front-end proxy of the result of the 
 *         creation request.
 *
 * The main body of the session creation procedure lies in the function which the create_sess_active pointer points to.
 */
    BackendEngine                   *engine;
    struct BackendSessionPoolOps    *sess_pool_ops;
    struct BackendSession           *new_sess = NULL;
    NetChannel                      *net_channel;
    uint16_t                        frontend_sess_id, new_sess_id, dev_id;
    int                             fd = ERROR_SOCKET_FD, ns_id, ret;
    SessOpRespData                  resp_dat;
    struct HighSpeedNetDevice       *hs_dev;
    struct SessionNode              *sess_node;

    engine          = s_pool->engine;
    sess_pool_ops   = s_pool->ops;

    utils_print("In %s\n", __func__);
    utils_print("The address of engine is %p\n", engine);
    utils_print("The address of engine ops is %p\n", engine->ops);
    utils_print("The address of session pool ops is %p\n", sess_pool_ops);
#if 0
    utils_print("In %s, the address of the engine is %p, ops is %p， hs_backend_eng_ops address is %p, and chooes_dev is %p\n", 
                __func__, engine, engine->ops, get_hs_backend_engine_ops(), engine->ops->choose_dev);
#endif

    if(NULL == engine || NULL == engine->ops || NULL == engine->ops->choose_dev){
        error_print("high_speed_create_sess_active fails: the session pool does not belong to any engine, or the engine is not initialized successfully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


    if(NULL == sess_pool_ops || NULL == sess_pool_ops->insert_sess){
        error_print("high_speed_create_sess_active fails: the session pool operation function set is not initialized correctly!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }
/*
 * STEP 1.
 * (1) Allocate memory for storing the backend session object;
 * (2) Parse session message parameters, obtain the device ID, and execute the device selection procedure if necessary;
 * (3) Determine the namespace ID by parsing the device ID of the specified high-speed network device, and create a new socket based on the session message parameters.
 */
    frontend_sess_id    = para->frontend_sess_id;
    new_sess            = (struct BackendSession*)malloc(sizeof(struct BackendSession));
    if(NULL == new_sess){
        error_print("high_speed_create_sess_active failed: failed to allocate memory for BackendSession!");
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }

    new_sess_id = allocate_id(&s_pool->id_queue);
    if(0 == new_sess_id){
        error_print("high_speed_create_sess_active failed: failed to allocating session ID!");
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

    utils_print("The dev_id is %d, %s\n", dev_id, __func__);
    if(DEV_ID_AUTO_HANDOVER == dev_id){
        if(BACKEND_PROXY_PROCESS_OK != engine->ops->choose_dev(engine, &dev_id)){
            error_print("high_speed_create_sess_active failed: the network device selection procedure failed!");
            resp_dat.status = SESS_OP_STATUS_FAIL;
            resp_dat.code   = SESS_OP_CODE_DEVICE_ERROR;
            goto create_sess_error;
        }
    }

/*
 * Get the namespace ID to which the selected high-speed network device is set.
 */
//    hs_dev = &engine->dev_set[dev_id];
    ns_id = GET_NS_ID(engine->dev_set, dev_id);
    if(ERROR_NAMESPACE_ID == ns_id){
        error_print("high_speed_create_sess_active failed: failed to obtain the namespace ID that the selected high-speed network device belongs to!");
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_DEVICE_ERROR;
        goto create_sess_error;
    }

/*
 * Create a socket with given parameters.
 */
    if(BACKEND_PROXY_PROCESS_OK != create_socket_netns(ns_id, para, &fd)){
        error_print("high_speed_create_sess_active failed: failed to create a socket with the given parameters!");
        fd = ERROR_SOCKET_FD;
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }

/*
 * STEP 2:
 * (1). Connect to the specified IP:Port tuple using the newly created socket; if successful, add this socket to the epoll wait list;
 * (2). Initialize all members of the new session object;
 * (3). Create a RESPONSE message to notify the frontend proxy that the new session has been created successfully;
 * (4). Insert the session into the corresponding session pool.
 */

/*
 * Now connect to the remote IP:Port.
 *
 * PLEASE NOTICE
 *
 * The datagram socket can also "connect" to the specified IP:Port. However, the behavior is quite different from that of a stream socket when connecting to the remote 
 * side. We choose to call connect on a datagram socket in order to unify the procedure of the backend proxy network stack. The details are processed by the Linux kernel
 * network stack.
 */

    if(BACKEND_PROXY_PROCESS_OK != connect_socket_netns(fd, para)){
        error_print("high_speed_create_sess_active failed: failed to connect to the specified IP:Port!");
/*
 * The backend proxy should generate and send a create-response message to notify the frontend proxy that the create-command has failed.
 * We have not developed the failed-reason function; it is reserved for future development.
 */
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.status = SESS_OP_CODE_NETWORK_UNREACHABLE;

        goto create_sess_error;
    }
/*
 * Initialize the network channel.
 */
    net_channel             = &new_sess->net_channel;
    net_channel->sock_fd    = fd;
    net_channel->arg        = &engine->poller;
    net_channel->sess       = new_sess;
    net_channel->status     = EPOLL_REG_STATUS_UNREGISTERED;

/*
 * Initialize session object.
 */    
    new_sess->frontend_sess_id  = para->frontend_sess_id;
    new_sess->backend_sess_id   = new_sess_id;
    new_sess->ip_version        = para->ip_version;
    new_sess->sock_fd           = fd;
    new_sess->eng               = engine;
    new_sess->state_f2b         &= BACKEND_SESS_LINKED_TO_QUEUE;
    new_sess->state_b2f         &= BACKEND_SESS_LINKED_TO_QUEUE;
    new_sess->establish_mode    = SESS_ESTABLISH_ACTIVE;
    new_sess->proto_type        = para->trans_proto;
    TAILQ_INIT(&new_sess->msg_f2b);
    utils_print("In %s, TAILQ_EMPTY(&new_sess->msg_f2b) returns %d\n", __func__, TAILQ_EMPTY(&new_sess->msg_f2b));
    TAILQ_INIT(&new_sess->msg_b2f);
    utils_print("In %s, TAILQ_EMPTY(&new_sess->msg_b2f) returns %d\n", __func__, TAILQ_EMPTY(&new_sess->msg_b2f));

/*
 * Allocate a SessionNode from the specified HighSpeedNetDevice. If the allocation succeeds, bind the node to either the udp_node or tcp_node queue of the device according to the proto_type.
 */
//    hs_dev      = &engine->dev_set[dev_id];
    hs_dev = &(engine->dev_set->hs_net_dev[dev_id]);
    sess_node   = HIGH_SPEED_NET_DEV_POP_FREE_SESSION_NODE(hs_dev);

    utils_print("In %s, the address of the sess_node is %p\n",  __func__, sess_node);

    if(NULL == sess_node){
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto create_sess_error;
    }

    new_sess->sess_dev_link_state   &= ~BACKEND_SESS_LINKED_TO_DEV_NODE;
    sess_node->sess                 = new_sess;
    new_sess->sess_node             = sess_node;
    ret = HIGH_SPEED_NET_DEV_INSERT_SESSION_NODE(hs_dev, sess_node, sess_node->sess->proto_type);

//    utils_print("In %s, the sess_dev_link_state is %d, ret = %d\n",  __func__, new_sess->sess_dev_link_state, ret);

//    HIGH_SPEED_NET_DEV_REMOVE_SESSION_NODE(hs_dev, sess_node, sess_node->sess->proto_type);
//    utils_print("After HIGH_SPEED_NET_DEV_REMOVE_SESSION_NODE, the sess_dev_link_state is %d\n",  new_sess->sess_dev_link_state);


    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("high_speed_create_sess_active failed: failed to insert sess node to the proper sess node queue!");
        sess_node->sess         = NULL;
        new_sess->sess_node     = NULL;
        HIGH_SPEED_NET_DEV_PUSH_FREE_SESSION_NODE(hs_dev, sess_node);
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.code   = SESS_OP_CODE_DEVICE_ERROR;
        goto create_sess_error;
    }

    utils_print("In %s, the ret of HIGH_SPEED_NET_DEV_INSERT_SESSION_NODE is %d\n", __func__, ret);


/*
 * Finally, register the socket belonging to the newly created session with the epoll instance.
 */

    BACKEND_SESS_REGISTER_EPOLL(new_sess, EPOLLIN, &ret);

    if(ret != BACKEND_PROXY_PROCESS_OK){
        error_print("high_speed_create_sess_active failed: failed to register the socket of the newly create session with the epoll instance!");
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.status = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto recycle_sess_node;
    }


    ret = sess_pool_ops->insert_sess(s_pool, new_sess);

    if(ret != BACKEND_PROXY_PROCESS_OK){
        error_print("high_speed_create_sess_active failed: failed to insert the session instance into the session pool!");
        BACKEND_SESS_UNREGISTER_EPOLL(new_sess, &ret);
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.status = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto recycle_sess_node;
    }

    utils_print("high_speed_create_sess_active returns successfully!\n");

/*
 * Generate a session create-response message, deliver it to the shared queue. The front-end will receive this message and complete the handshake procedure.
 */

    resp_dat.status = SESS_OP_STATUS_SUCCESS;
    resp_dat.code   = SESS_OP_CODE_SUCCESS;

    ret = backend_proxy_send_sess_msg_to_frontend_via_shmem(new_sess, SESS_MSG_CREATE, &resp_dat);
    
    if(BACKEND_PROXY_PROCESS_OK != ret){
        error_print("high_speed_create_sess_active failed: failed to push the session creat-response message into the tx queue!");
/*
 * The backend proxy should generate and send a create-response message to notify the frontend proxy that the create-command has failed.
 * We want the backend proxy protocol to try again by sending a message to notify that the establishment procedure has failed.
 */
        resp_dat.status = SESS_OP_STATUS_FAIL;
        resp_dat.status = SESS_OP_CODE_RESOURCE_INSUFFICIENT;
        goto recycle_sess_node;
    }

    *sess = new_sess;

    return BACKEND_PROXY_PROCESS_OK;

recycle_sess_node:
    HIGH_SPEED_NET_DEV_REMOVE_SESSION_NODE(hs_dev, sess_node, sess_node->sess->proto_type);
    sess_node->sess         = NULL;
    new_sess->sess_node     = NULL;
    HIGH_SPEED_NET_DEV_PUSH_FREE_SESSION_NODE(hs_dev, sess_node);
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

    return BACKEND_PROXY_PROCESS_ERROR;
 }

/**
 * @brief  Allocates and initializes a passive session object, then sends the session creation message to the front-end via shared memory
 * @details This function completes three core tasks in sequence: 
 *          1. Allocates a new passive session object from the specified backend session pool;
 *          2. Initializes the session object with the input PassiveSessParaIP parameters (including dev_id, IP-port tuple, etc.);
 *          3. Constructs a standard session creation message and transmits it to the front-end module through the shared memory mechanism.
 * @param  s_pool  Pointer to struct BackendSessionPool, which is the source pool for allocating session objects (input parameter).
 * @param  sess    Double pointer to struct BackendSession, used to output the allocated and initialized passive session object (output parameter).
 * @param  para    Pointer to PassiveSessParaIP, which provides the core initialization parameters for the passive session (input parameter).
 * @return Returns status code @ref BACKEND_PROXY_PROCESS_OK if all operations (allocation, initialization, message sending) are completed successfully;
 *         Returns status code @ref BACKEND_PROXY_PROCESS_ERROR if any step fails (e.g., session allocation failure, parameter invalidation, shared memory transmission failure).
 * @note   Currently, the function only returns BACKEND_PROXY_PROCESS_OK by default; extended exception handling logic can be added to return BACKEND_PROXY_PROCESS_ERROR for abnormal scenarios.
 * @fn     int high_speed_create_sess_passive(struct BackendSessionPool *s_pool, struct BackendSession **sess, PassiveSessParaIP *para)
 */
int high_speed_create_sess_passive(struct BackendSessionPool *s_pool, struct BackendSession **sess, PassiveSessParaIP *para){
/*
 * The passive session creation procedure can be divided into two steps:
 * STEP 1. Allocate resources, including the session object and backend session ID. Note that the socket is pre-created prior to the invocation of the high_speed_create_sess_passive function.
 * STEP 2. Establish a session using the parameters provided by the frontend, and construct a session creation message to inform the front-end proxy that a passive session has been established.
 *
 * The core logic of the session creation procedure resides in the function pointed to by the create_sess_passive pointer.
 */
    BackendEngine                   *engine;
    struct BackendSessionPoolOps    *sess_pool_ops;
    struct BackendSession           *new_sess = NULL;
//    NetChannel                      *net_channel;
//    uint16_t                        frontend_sess_id, backend_sess_id;
    uint16_t                        new_sess_id, dev_id;
    int                             ret;
    struct HighSpeedNetDevice       *hs_dev;
    struct SessionNode              *sess_node = NULL;
    GeneralProxyMsgHeader           msg_header;
    SessMsgHeader                   *sess_msg_hdr;
    SessIPv4Params                  sess_ipv4_paras;
    uint8_t                         *res_buf[100] = {NULL};


    engine          = s_pool->engine;
    sess_pool_ops   = s_pool->ops;

    utils_print("In %s\n", __func__);
    utils_print("The address of engine is %p\n", engine);
    utils_print("The address of engine ops is %p\n", engine->ops);
    utils_print("The address of session pool ops is %p\n", sess_pool_ops);

    if(NULL == engine || NULL == engine->ops){
        error_print("high_speed_create_sess_passive fails: the session pool does not belong to any engine, or the engine is not initialized successfully!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


    if(NULL == sess_pool_ops || NULL == sess_pool_ops->insert_sess){
        error_print("high_speed_create_sess_passive fails: the session pool operation function set is not initialized correctly!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    
    if(NULL == sess || NULL == para){
        error_print("high_speed_create_sess_passive failed: invalid input parameters (sess or para is NULL)\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    if(SESS_IPV4_PROTO != para->ip_version){
        error_print("high_speed_create_sess_passive failed: only IPv4 is supported at present (IPv6 support is planned for future extensions)\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


    if(para->dev_id >= MAX_HS_DEV_NUM){
        error_print("high_speed_create_sess_passive failed: invalid dev_id (exceeds MAX_HS_DEV_NUM)\n");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    dev_id = para->dev_id;
/*
 * STEP 1.
 * Allocate and initialize memory for the backend session object.
 * Then allocate a corresponding backend session ID and session node for it.
 */
    new_sess    = (struct BackendSession*)malloc(sizeof(struct BackendSession));

    if(NULL == new_sess){
        error_print("high_speed_create_sess_active failed: failed to allocate memory for BackendSession!");
        goto create_sess_passive_error;
    }

    new_sess_id = allocate_id(&s_pool->id_queue);

    if(0 == new_sess_id){
        error_print("high_speed_create_sess_active failed: failed to allocating session ID!");
        goto create_sess_passive_error;
    }

//    hs_dev      = &engine->dev_set[dev_id];
    hs_dev      = &(engine->dev_set->hs_net_dev[dev_id]);
    sess_node   = HIGH_SPEED_NET_DEV_POP_FREE_SESSION_NODE(hs_dev);

    if(NULL == sess_node){
        error_print("high_speed_create_sess_active failed: failed to allocating sess node!\n");
        goto create_sess_passive_error;
    }

    new_sess->backend_sess_id       = new_sess_id;
    new_sess->dev_id                = dev_id;
    new_sess->ip_version            = para->ip_version;
    new_sess->sock_fd               = para->fd;
    new_sess->eng                   = engine;
    new_sess->state_f2b             &= BACKEND_SESS_LINKED_TO_QUEUE;
    new_sess->state_b2f             &= BACKEND_SESS_LINKED_TO_QUEUE;
    new_sess->establish_mode        = SESS_ESTABLISH_PASSIVE;
    new_sess->proto_type            = para->trans_proto;


    new_sess->sess_dev_link_state   &= ~BACKEND_SESS_LINKED_TO_DEV_NODE;
    sess_node->sess                 = new_sess;
    new_sess->sess_node             = sess_node;


    ret = HIGH_SPEED_NET_DEV_INSERT_SESSION_NODE(hs_dev, sess_node, sess_node->sess->proto_type);

    if(BACKEND_PROXY_PROCESS_ERROR == ret){
        error_print("high_speed_create_sess_active failed: can not insert session node to according protocol queue!\n");
        goto create_sess_passive_error;
    }
/*
 * Fills information for generating session create-command message for passive session creation.
 * Because the session instance has not been successfully created in frontend size, the frontend_sess_id cannot be registered. 
 * The backend proxy protocol will fill the frontend session ID field with FRONTEND_HANDOVER_SESSION_ID.
 *
 * It is not necessary to fill the payload_len field.
 */
    msg_header.outer_header.version             = PROXY_PROTO_VERSION_1;
    msg_header.outer_header.proxy_msg_type      = PROXY_MSG_TYPE_SESS;
    msg_header.outer_header.frontend_sess_id    = FRONTEND_HANDOVER_SESSION_ID;
    msg_header.outer_header.backend_sess_id     = new_sess_id;

//    header.outer_header.payload_len;
    sess_msg_hdr                                = &msg_header.inner_header.sess_hdr;
    sess_msg_hdr->version                       = PROXY_PROTO_SESS_VERSION_1;
    sess_msg_hdr->msg_type                      = SESS_MSG_CREATE;
    sess_msg_hdr->action_type                   = ACTION_TYPE_COMMAND;
    sess_msg_hdr->ip_version                    = para->ip_version;
    sess_msg_hdr->payload_len                   = sizeof(SessIPv4Params);

    sess_ipv4_paras.device_selection            = new_sess->dev_id;
    sess_ipv4_paras.transport_layer_proto       = para->trans_proto;
    memcpy(&sess_ipv4_paras.dest_endpoint, &para->ip_port_tuple.ipv4_port_tuple, sizeof(IPv4PortTuple));

//    ret = build_proxy_general_message(engine, &msg_header, &sess_ipv4_paras, sizeof(sess_ipv4_paras), res_buf, MEMORY_ALLOC_SHARED, engine->tx_queue);
    ret = build_proxy_general_message(engine, &msg_header, (const uint8_t*)&sess_ipv4_paras, sizeof(sess_ipv4_paras), res_buf, MEMORY_ALLOC_AMPQUEUE, NULL);

    return ret;
create_sess_passive_error:
/*
 * Reclaim resources.
 */

    if(0 != new_sess_id){
        release_id(&s_pool->id_queue, new_sess_id);
    }

    if(NULL != new_sess){
        if(NULL != sess_node){
            if(new_sess->sess_dev_link_state & BACKEND_SESS_LINKED_TO_DEV_NODE){
                HIGH_SPEED_NET_DEV_REMOVE_SESSION_NODE(hs_dev, sess_node, sess_node->sess->proto_type);
            }

            sess_node->sess         = NULL;
            new_sess->sess_node     = NULL;
            HIGH_SPEED_NET_DEV_PUSH_FREE_SESSION_NODE(hs_dev, sess_node);
        }

        free(new_sess);
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
/*
 * The process of deleting a session can be divided into two steps:
 * STEP 1. Release resources, including session objects, backend session ID, socket, session node, etc.
 * STEP 2. Create a session message to inform the front-end proxy of the result of the 
 *         session closure request.
 */
    uint16_t                    frontend_sess_id, backend_sess_id;
    SessOpRespData              resp_dat;
    int                         ret, ip_version, dev_id;
    BackendEngine               *eng;
    struct HighSpeedNetDevice   *hs_dev;
    struct SessionNode          *sess_node;


    if(NULL == s_pool || NULL == sess || NULL == s_pool->engine){
        error_print("high_speed_delete_sess failed: session pool (s_pool), session (sess), or engine (s_pool->engine) is NULL!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

/*
 * STEP 1:
 * (1) Remove the socket bound to the backend session object from the epoll instance, close it, and record
 * the session information for building the session close response message;
 * (2) Delete the session instance from the hash table and detach it from the doubly linked lists;
 * (3) Release the resources occupied by the session instance.
 */

/*
 * STEP 1(1).
 */
    BACKEND_SESS_UNREGISTER_EPOLL(sess, &ret);

    if(BACKEND_PROXY_PROCESS_OK != ret){
/*
 * Even if the unregistration operation fails, high_speed_delete_sess should not return BACKEND_PROXY_PROCESS_ERROR immediately.
 * It should continue executing to ensure all resources occupied by the session to be freed are released.
 */
        error_print("high_speed_delete_sess: failed to unregister socket from epoll instance!");
    }

    close(sess->sock_fd);

    frontend_sess_id    = sess->frontend_sess_id;
    backend_sess_id     = sess->backend_sess_id;
    ip_version          = sess->ip_version;
    dev_id              = sess->dev_id;
/*
 * STEP 1(2).
 */
    HASH_DEL(s_pool->htable, sess);

    dec_sess_num(s_pool);

    BACKEND_SESS_UNLINK_FROM_QUEUE(sess, f2b);
    BACKEND_SESS_UNLINK_FROM_QUEUE(sess, b2f);

/*
 * STEP 1(3).
 */
    sess_msg_queue_free_all(&sess->msg_f2b);
    sess_msg_queue_free_all(&sess->msg_b2f);
    release_id(&s_pool->id_queue, backend_sess_id);

/*
 * A session may be terminated either gracefully or abnormally. The value of the sess_dev_link_state flag determines the corresponding processing behavior.
 * 
 * If the BACKEND_SESS_LINKED_TO_DEV_NODE flag is set, it means the session termination procedure is triggered by the application itself, or by the remote peer in
 * a normal manner. The session node should be reattached to the free node list.
 * 
 * Otherwise, it means the session termination procedure is triggered when the backend proxy process is terminated by an interrupt signal. In this case, the 
 * session node should NOT be reattached to the free node list. Instead, the memory occupied by the session node should be freed. This is not done in the 
 * high_speed_delete_sess function. It is the sole duty of the backend engine destruction procedure.
 */
    eng    = s_pool->engine;
    dev_id = sess->dev_id;

    if(dev_id < 0 || dev_id >= MAX_HS_DEV_NUM){
/*
 * Invalid device ID. Simply ignore the session node recycling procedure.
 */
        error_print("high_speed_delete_sess notice: the session device ID is invalid!\n");
    }else{
/*
 * Device ID is valid.
 */
        if(sess->sess_dev_link_state & BACKEND_SESS_LINKED_TO_DEV_NODE){
//            hs_dev = &eng->dev_set[dev_id];
            hs_dev = &(eng->dev_set->hs_net_dev[dev_id]);
            sess_node = sess->sess_node;
/*
 * Recycle the session node. 
 */
            HIGH_SPEED_NET_DEV_REMOVE_SESSION_NODE(hs_dev, sess_node, sess_node->sess->proto_type);
            sess_node->sess         = NULL;
            HIGH_SPEED_NET_DEV_PUSH_FREE_SESSION_NODE(hs_dev, sess_node);
        }
    }


    free(sess);

/*
 * STEP 2:
 * Create a session close-response message based on the information in the session close-command message, and send this message 
 * to the front-end proxy via shared memory.
 */

    resp_dat.status = SESS_OP_STATUS_SUCCESS;
    resp_dat.code   = SESS_OP_CODE_SUCCESS;
    ret = backend_proxy_send_sess_standalone_msg_to_frontend_via_shmem(eng, frontend_sess_id, backend_sess_id, ip_version, SESS_MSG_CLOSE, &resp_dat);


    return BACKEND_PROXY_PROCESS_OK;
}



/**
 * @brief Processes high-speed data transmission from frontend to backend (f2b) using the Linux native network stack (NNS)
 * 
 * This function handles data forwarding from the frontend-to-backend message queue (msg_f2b) of a backend session,
 * sending the queued data through the session's socket to the backend. It manages socket send buffer constraints
 * and ensures proper memory deallocation of processed message segments.
 * 
 * @detail The processing workflow follows these key steps:
 *         1. Retrieve the socket file descriptor from the backend session context (struct BackendSession).
 *         2. Use getsockopt with SO_SNDBUF to query the current size of the socket's send buffer.
 *         3. If fetching the send buffer size fails, log an error and return BACKEND_PROXY_PROCESS_ERROR.
 *         4. Safely iterate over all message segments in the frontend-to-backend queue (msg_f2b) using TAILQ_FOREACH_SAFE:
 *             a. For each segment with valid data (non-null data and positive length):
 *                i. Check if the send buffer has sufficient space for the segment's data.
 *                ii. If sufficient space exists, send the data through the socket using send().
 *                iii. If send() fails, log an error, and return BACKEND_PROXY_PROCESS_ERROR.
 *                iv. Decrement the remaining send buffer size by the number of bytes sent.
 *                v. If insufficient space exists (send buffer smaller than segment length), return 
 *                   BACKEND_PROXY_PROCESS_AGAIN to indicate a retry is needed after buffer space is freed.
 *             b. Remove the processed segment from the msg_f2b queue using TAILQ_REMOVE.
 *             c. Deallocate memory based on the segment's allocation type:
 *                - For dynamically allocated segments (SESS_MSG_SEG_DYNAMIC_ALLOC), free the data buffer.
 *                - For shared memory segments (SESS_MSG_SEG_SHARED_MEM), placeholder logic for releasing shared memory.
 *             d. Free the segment structure itself.
 *         5. Once all queued segments are processed successfully, return BACKEND_PROXY_PROCESS_OK.
 * 
 * @param[in] sess Pointer to the BackendSession structure containing session context (socket FD, msg_f2b queue, etc.)
 * 
 * @return Processing result status code:
 *         - BACKEND_PROXY_PROCESS_OK: All queued data processed and sent successfully
 *         - BACKEND_PROXY_PROCESS_AGAIN: Incomplete processing (insufficient send buffer space) requiring retry
 *         - BACKEND_PROXY_PROCESS_ERROR: Critical error occurred (e.g., failed to get send buffer size or send data)
 */
int high_speed_data_process_f2b(struct BackendSession *sess)
{
    int fd, buf_size, ret;
    socklen_t buf_len;
    struct SessMsgSeg *cur_seg, *next_seg;

    fd = sess->sock_fd;
    buf_len = sizeof(buf_size);

    printf("In %s\n", __func__);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buf_size, &buf_len) == -1) {
        error_print("high_speed_data_process failed: failed to get the size of the send buffer size!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }

    printf("socket buffer size = %d\n", buf_size);

    TAILQ_FOREACH_SAFE(cur_seg, &sess->msg_f2b, entry, next_seg) {

        /* 1. Send the data from current segment */
        printf("cur_seg->len = %d, msg = %s\n", cur_seg->len, cur_seg->data);
        if (cur_seg->data && cur_seg->len > 0) {
            if(buf_size > cur_seg->len){
                printf("seg size = %d\n", cur_seg->len);
                ret = send(fd, cur_seg->data, cur_seg->len, 0);
                printf("After send, ret = %d\n", ret);

                if(-1 == ret){
                    error_print("high_speed_data_process: Failed to send data via session socket");
                    return BACKEND_PROXY_PROCESS_ERROR;
                }

                buf_size -= ret;

            }else{
            /* Unable to send all the data in the queue because the socket's send buffer is not enough. */
                return BACKEND_PROXY_PROCESS_AGAIN;
            }
        }

        /* 2. Remove the segment from the queue */
        TAILQ_REMOVE(&sess->msg_f2b, cur_seg, entry);

        /* 3. Deallocate memory based on segment type */
        if (cur_seg->type == SESS_MSG_SEG_DYNAMIC_ALLOC) {
            // Free dynamically allocated data buffer
            free(cur_seg->data);
        } else if (cur_seg->type == SESS_MSG_SEG_SHARED_MEM) {

            // if (current_seg->mem_pool) {
            //     shared_memory_pool_release(current_seg->mem_pool, current_seg->data);
            // }
        }
        /* 4. Free the segment structure itself */
        free(cur_seg);


    }// TAILQ_FOREACH_SAFE


    return BACKEND_PROXY_PROCESS_OK;
}

/**
 * @brief Processes high-speed data transmission from backend to frontend via shared memory queue
 * 
 * This function handles data forwarding from the backend-to-frontend message queue (msg_b2f) to the shared memory 
 * TX queue. It copies message data from each segment in msg_b2f to the TX queue, then cleans up the processed 
 * message segments to free resources. This facilitates efficient data transfer between backend and frontend 
 * through shared memory.
 * 
 * @detail The processing workflow follows these key steps:
 *         1. Retrieve the backend engine context and shared memory TX queue from the session.
 *         2. Iterate over all message segments in the backend-to-frontend queue (msg_b2f) using TAILQ_FOREACH_SAFE:
 *             a. Allocate a slot in the shared memory TX queue using SHM_POOL_QUEUE_HEAD_ALLOC, storing the 
 *                virtual address in 'addr'.
 *             b. If TX queue is full (allocation returns BACKEND_PROXY_PROCESS_ERROR), log a non-fatal error and 
 *                return BACKEND_PROXY_PROCESS_AGAIN to indicate a retry is needed.
 *             c. Copy data from the current message segment (cur_seg->data) to the allocated TX queue slot (addr),
 *                using the segment's length (cur_seg->len) to determine copy size.
 *             d. Remove the processed segment from msg_b2f using TAILQ_REMOVE.
 *             e. Deallocate the segment's data buffer based on its allocation type:
 *                - For dynamically allocated segments (SESS_MSG_SEG_DYNAMIC_ALLOC), free the data buffer with free().
 *                - For shared memory segments (SESS_MSG_SEG_SHARED_MEM), reserved for future implementation (currently a no-op).
 *             f. Free the message segment structure itself with free().
 *         3. Once all segments in msg_b2f are processed, return BACKEND_PROXY_PROCESS_OK.
 * 
 * @param[in] sess Pointer to the BackendSession structure containing session context, including msg_b2f queue and engine reference
 * 
 * @return Processing result status code:
 *         - BACKEND_PROXY_PROCESS_OK: All message segments processed and transferred successfully
 *         - BACKEND_PROXY_PROCESS_AGAIN: Incomplete processing (TX queue is full) requiring retry
 *         - BACKEND_PROXY_PROCESS_ERROR: Critical error occurred (implied by allocation failure handling)
 */
int high_speed_data_process_b2f(struct BackendSession *sess){
    BackendEngine                   *eng;
    struct SharedMemoryPoolQueue    *tx_queue;
    struct SessMsgSeg               *cur_seg, *next_seg;
    int                             ret;
//    uint64_t                        addr;

    eng         = sess->eng;
    (void)tx_queue;
    tx_queue    = eng->tx_queue;

    /*
     * 1. Copy data in each message segment in the backend-to-frontend queue (msg_b2f) to the shared memory TX queue. The front-end will read these messages latter.
     */
    TAILQ_FOREACH_SAFE(cur_seg, &sess->msg_b2f, entry, next_seg){
        DUMP_BUFFER_CONTENT(cur_seg->data + sizeof(ProxyMsgHeader), 8, "%c");
//        ret = shared_mem_pool_queue_send_oc(tx_queue, cur_seg->data, cur_seg->len);
        ret = backend_engine_hyperamp_tx_queue_put(eng, cur_seg->data, cur_seg->len);
/*
 * The TX queue is full. The high_speed_data_process_b2f should return and notice that the sending procedure from backend to front end continue next time.
 */
        if(BACKEND_PROXY_PROCESS_AGAIN == ret){
            error_print("high_speed_data_process_b2f failed (may not be an error): the send procedure failed because the TX queue is full. It should retry the next time.");
            return BACKEND_PROXY_PROCESS_AGAIN;
        }

        /* 2. Remove the segment from the queue */
        TAILQ_REMOVE(&sess->msg_b2f, cur_seg, entry);

        /* 3. Deallocate memory based on segment type */
        if (cur_seg->type == SESS_MSG_SEG_DYNAMIC_ALLOC) {
            // Free dynamically allocated data buffer
            free(cur_seg->data);
        } else if (cur_seg->type == SESS_MSG_SEG_SHARED_MEM) {

            // if (current_seg->mem_pool) {
            //     shared_memory_pool_release(current_seg->mem_pool, current_seg->data);
            // }
        }
        /* 4. Free the segment structure itself */
        free(cur_seg);

    }
    return BACKEND_PROXY_PROCESS_OK;
}


/**
 * @brief Processes high-speed data transmission using the Linux native network stack (NNS)
 * 
 * This function handles data retrieval from a backend session's socket receive buffer, encapsulates the data into 
 * proxy message format, and routes it to the backend-to-frontend message queue (msg_b2f). It facilitates data 
 * forwarding from kernel space to the user-space proxy layer, ensuring efficient handling of network data.
 * 
 * @detail The processing workflow follows these key steps:
 *         1. Retrieve the socket file descriptor from the backend session context (struct BackendSession).
 *         2. Use ioctl with SIOCINQ to query the size of available data in the socket's receive buffer.
 *         3. If fetching available data size fails, log an error and return BACKEND_PROXY_PROCESS_ERROR.
 *         4. Enter a loop to process all available data in the receive buffer:
 *             a. Allocate a message segment (SessMsgSeg) with sufficient space for the proxy message header + maximum payload.
 *             b. If allocation fails (likely due to temporary memory constraints), log a non-fatal error and return 
 *                BACKEND_PROXY_PROCESS_AGAIN to indicate a retry is needed.
 *             c. Map pointers to the message header (ProxyMsgHeader) and payload data region within the allocated segment.
 *             d. Read data from the socket into the payload region, up to PROXY_MSG_MAX_SIZE bytes.
 *             e. If the read operation fails, free the allocated segment, log an error, and return 
 *                BACKEND_PROXY_PROCESS_ERROR.
 *             f. Populate the proxy message header with metadata: protocol version, frontend/backend session IDs, 
 *                message type (data), and actual payload length.
 *             g. Insert the fully constructed message segment into the backend-to-frontend queue (msg_b2f) using the 
 *                SESS_MSG_SEG_INSERT_QUEUE macro.
 *             h. Decrement the remaining available data size by the number of bytes read in the current iteration.
 *         5. Once all data is processed, return BACKEND_PROXY_PROCESS_OK.
 * 
 * @param[in] sess Pointer to the BackendSession structure containing session context (socket FD, IDs, etc.)
 * 
 * @return Processing result status code:
 *         - BACKEND_PROXY_PROCESS_OK: All data processed successfully
 *         - BACKEND_PROXY_PROCESS_AGAIN: Incomplete processing (e.g., memory allocation failure) requiring retry
 *         - BACKEND_PROXY_PROCESS_ERROR: Critical error occurred (e.g., socket read failure)
 */
int high_speed_data_process_nns(struct BackendSession *sess){
    int fd, bytes_available, ret;
    struct SessMsgSeg *cur_seg;
    uint8_t           *msg_data;
    ProxyMsgHeader    *msg_hdr;

    fd = sess->sock_fd;


    if (-1 == ioctl(fd, SIOCINQ, &bytes_available)) {
        error_print("high_speed_data_process_nns failed: failed to fetch the availble data size in socket!");
        return BACKEND_PROXY_PROCESS_ERROR;
    }


/*
 * high_speed_data_process_nns reads data from the socket's receive buffer, converts it into proxy message data, and then organizes these messages into the
 * backend-to-frontend queue (msg_b2f).
 */
    utils_print("high_speed_data_process_nns: bytes_available = %d\n", bytes_available);
    while(bytes_available > 0){
        cur_seg = sess_msg_seg_alloc(PROXY_MSG_HDR_PLUS_MAX_SIZE, SESS_MSG_SEG_DYNAMIC_ALLOC, NULL, NULL);

        if(NULL == cur_seg){
            error_print("high_speed_data_process_nns failed (may not be an error): insufficient memory for allocating message segment \
                         to hold the data from backend to frontend. Should retry next time!");
            return BACKEND_PROXY_PROCESS_AGAIN;
        }
    /*
     * Copy the data into the memory area that the message segment maintains.
     */

        msg_hdr = (ProxyMsgHeader *)cur_seg->data;
        msg_data = cur_seg->data + sizeof(ProxyMsgHeader);

        ret = read(fd, msg_data, PROXY_MSG_MAX_SIZE);

        if(-1 == ret){
            error_print("high_speed_data_process_nns failed: failed to read data from the session's socket!");
        /*
         * Only free the message segment. Freeing the message is left to the poller procedure when a data read error occurs.
         */
            sess_msg_seg_free(cur_seg);
            return BACKEND_PROXY_PROCESS_ERROR;
        }

        msg_hdr->version            = PROXY_PROTO_VERSION_1;
        msg_hdr->frontend_sess_id   = sess->frontend_sess_id;
        msg_hdr->backend_sess_id    = sess->backend_sess_id;
        msg_hdr->proxy_msg_type     = PROXY_MSG_TYPE_DATA;
        msg_hdr->payload_len        = ret;
        utils_print("high_speed_data_process_nns: read %d bytes from socket %d\n", ret, fd);
        utils_print("high_speed_data_process_nns: msg_hdr->frontend_sess_id = %d, msg_hdr->backend_sess_id = %d, msg_hdr->payload_len = %d\n",
                    msg_hdr->frontend_sess_id, msg_hdr->backend_sess_id, msg_hdr->payload_len);
        utils_print("msgdata = %s\n", msg_data);

        cur_seg->len = msg_hdr->payload_len + sizeof(ProxyMsgHeader);
        SESS_MSG_SEG_INSERT_QUEUE(sess, cur_seg, b2f);
        DUMP_BUFFER_CONTENT(cur_seg->data + sizeof(ProxyMsgHeader), 8, "%c");
        bytes_available -= ret;
    }
    BACKEND_SESS_LINK_TO_QUEUE(sess, b2f);
    return BACKEND_PROXY_PROCESS_OK;
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

